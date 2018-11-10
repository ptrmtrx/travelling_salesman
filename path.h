/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

#include "city.h"
#include "random.h"

//extern config g_config;
extern std::atomic<bool> g_continue_run;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static constexpr double get_last_t(std::size_t cities)
{
    if (cities < 55)
        return 0.005;

    if (cities < 105)
        return 0.002;

    return 0.001;
}

static constexpr std::uint16_t bound_value(std::uint16_t rnd, std::uint16_t range)
{
    std::uint32_t x = static_cast<std::uint32_t>(rnd) * static_cast<std::uint32_t>(range);
    return x >> 16;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class areapath_t
{
public:
    areapath_t(std::vector<area_t> && areas_list, const cities_map_t * cities_indexer, const matrix<std::uint16_t> * costs_matrix)
        : m_path{std::move(areas_list)}
        , m_area_to_day(m_path.size() + 1)
        , m_day_to_area(m_path.size() + 1)
        , m_cities_indexer{cities_indexer}
        , m_costs{costs_matrix}
    {
        // Set tha last area same as the first.
        m_path.push_back(m_path[0]);

        // Init supported structures.
        std::iota(m_day_to_area.begin(), m_day_to_area.end(), static_cast<std::uint16_t>(0));
        std::random_shuffle(m_day_to_area.begin() + 1, m_day_to_area.begin() + m_path.size() - 1);
        for (std::uint16_t i = 0; i < m_day_to_area.size(); ++i)
            m_area_to_day[m_day_to_area[i]] = i;

        // Generate array with zones with swithable cities.
        // Don't count the first one but count the last one.
        m_cities_choises.reserve(200);
        for (std::uint16_t i = 1; i < m_path.size(); ++i)
            for (std::uint16_t j = 1; j < m_path[i].size(); ++j)
                m_cities_choises.push_back({i, j});
        m_cities_choises.shrink_to_fit();
    }

    void optimize()
    {
        rnd_gen_t rng(std::chrono::system_clock::now().time_since_epoch().count());

        auto min_path = *this;
        auto min_cost = cost();

        auto actual_cost = min_cost;

        // Some constants for temperature computing.
        auto Tn = /*g_config.iterations*/ 80'000'000;
        auto exp_base = std::log(get_last_t(m_path.size()));

        double actual_T = 1.0;

        unsigned int iter = 0;
        while (g_continue_run)
        {
            if (iter++ % 512 /*g_config.recomp_T*/ == 0)
                actual_T = std::exp(exp_base * std::pow(iter / (double)Tn, 0.3));

            // Randomly choose two indexes.
            std::uint16_t i, j;

            // Compute the best price.
            enum method_t { SWAP_AREAS, REVERSE_AREAS, INSERT_AREA, SELECT_CITY } method;
            auto cost_diff = std::numeric_limits<std::int32_t>::max();

            auto xrnd = rng();
            static_assert(sizeof xrnd == 8, "Bad random number generator!");

            {
                i = static_cast<std::uint16_t>(xrnd);
                j = static_cast<std::uint16_t>(xrnd >> 16);

                // generate indexes 1..m_path.size()
                i = bound_value(i, static_cast<std::uint16_t>(m_path.size()) - 2) + 1;
                j = bound_value(j, static_cast<std::uint16_t>(m_path.size()) - 2) + 1;

                method = SWAP_AREAS;
                cost_diff = swap_areas_cost_diff(i, j);
            }
            {
                auto price = reverse_cost_diff(i, j);
                if (price < cost_diff)
                {
                    cost_diff = price;
                    method = REVERSE_AREAS;
                }
            }
            {
                auto price = insert_cost_diff(i, j);
                if (price < cost_diff)
                {
                    cost_diff = price;
                    method = INSERT_AREA;
                }
            }

            if (m_cities_choises.size())
            {
                auto x = static_cast<std::uint16_t>(xrnd);
                x = bound_value(x, static_cast<std::uint16_t>(m_cities_choises.size()) - 1);
                auto xi = m_cities_choises[x].zone_idx;
                auto xj = m_cities_choises[x].city_pos;

                auto price = select_city_cost_diff(xi, xj);
                if (price < cost_diff)
                {
                    method = SELECT_CITY;
                    cost_diff = select_city_cost_diff(xi, xj);
                    i = xi;
                    j = xj;
                }
            }

            // Accept? Better ways accept every time || worse only with some probability.
            bool accept = true;
            if (cost_diff > 0)
            {
                auto rnd = static_cast<std::uint32_t>(xrnd >> 32);
                auto log_max_int = std::log(std::numeric_limits<std::uint32_t>::max());

                auto right = (-cost_diff / (actual_T * m_costs->get_max())) + log_max_int;
                auto left = std::log(rnd);

                accept = (left <= right);
            }

            // If the new path should be accepted, save it.
            if (accept)
            {
                switch (method)
                {
                case SWAP_AREAS:    swap_areas(i, j);    break;
                case REVERSE_AREAS: reverse_areas(i, j); break;
                case INSERT_AREA:   insert_areas(i, j);  break;
                case SELECT_CITY:   select_city(i, j);   break;
                }

                actual_cost += cost_diff;

                // If the actual path cost is the best one, save it.
                if (actual_cost < min_cost)
                {
                    min_path = *this;
                    min_cost = actual_cost;
                }
            }
        }
        //std::cout << "pocet iteraci (new): " << iter << std::endl;
        *this = min_path;
        assert(min_cost == cost());
    }

    void print(std::ostream & out) const
    {
        // Print the cost.
        out << cost() << std::endl;

        // Print the path.
        auto src_idx = static_cast<std::uint16_t>(0);
        for (std::uint16_t i = 1; i < m_path.size(); ++i)
        {
            auto dst_idx = city(i);

            out << m_cities_indexer->get_city_object(src_idx);
            out << ' ';
            out << m_cities_indexer->get_city_object(dst_idx);
            out << ' ';
            out << i;
            out << ' ';
            out << m_costs->get(src_idx, dst_idx, i - 1);
            out << std::endl;

            src_idx = dst_idx;
        }
    }

private:
    std::uint16_t city(std::uint16_t day) const noexcept
    {
        return m_path[m_day_to_area[day]][0];
    }

    std::uint32_t cost() const noexcept
    {
        std::uint32_t sum = 0;
        auto from = static_cast<std::uint16_t>(0);// always zero: m_path[0][0];
        for (std::uint16_t i = 1; i < m_path.size(); ++i)
        {
            auto to = city(i);
            sum += m_costs->get(from, to, i - 1);
            from = to;
        }
        return sum;
    }

    std::int32_t swap_areas_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        std::int32_t before;
        std::int32_t after;

        auto pim1 = city(i - 1);
        auto pi   = city(i);
        auto pip1 = city(i + 1);

        auto pjm1 = city(j - 1);
        auto pj   = city(j);
        auto pjp1 = city(j + 1);

        if (std::abs(i - j) > 1)
        {
            before = m_costs->get(pim1, pi, i - 1) + m_costs->get(pi, pip1, i)
                   + m_costs->get(pjm1, pj, j - 1) + m_costs->get(pj, pjp1, j);
            after  = m_costs->get(pim1, pj, i - 1) + m_costs->get(pj, pip1, i)
                   + m_costs->get(pjm1, pi, j - 1) + m_costs->get(pi, pjp1, j);
        }
        else if (i + 1 == j)
        {
            before = m_costs->get(pim1, pi, i - 1) + m_costs->get(pi, pj, i) + m_costs->get(pj, pjp1, j);
            after  = m_costs->get(pim1, pj, i - 1) + m_costs->get(pj, pi, i) + m_costs->get(pi, pjp1, j);
        }
        else if (i - 1 == j)
        {
            before = m_costs->get(pjm1, pj, j - 1) + m_costs->get(pj, pi, j) + m_costs->get(pi, pip1, i);
            after  = m_costs->get(pjm1, pi, j - 1) + m_costs->get(pi, pj, j) + m_costs->get(pj, pip1, i);
        }
        else
        {
            before = 0;
            after  = 0;
        }

        return after - before;
    }

    std::int32_t reverse_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        auto k = std::min(i, j);
        auto l = std::max(i, j);

        if (l - k > 30)
            return std::numeric_limits<std::int32_t>::max();

        auto before = m_costs->get(city(k - 1), city(k), k - 1) + m_costs->get(city(l), city(l + 1), l);
        auto after  = m_costs->get(city(k - 1), city(l), k - 1) + m_costs->get(city(k), city(l + 1), l);

        auto end = l - k;
        for (std::uint16_t idx = 0; idx < end; ++idx)
        {
            before += m_costs->get(city(k + idx), city(k + idx + 1), k + idx);
            after  += m_costs->get(city(l - idx), city(l - idx - 1), k + idx);
        }

        return after - before;
    }

    std::int32_t insert_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        std::int32_t before;
        std::int32_t after;

        if (i < j)
        {
            if (j - i > 30)
                return std::numeric_limits<std::int32_t>::max();

            before = m_costs->get(city(i - 1), city(i), i - 1)
                   + m_costs->get(city(j - 1), city(j), j - 1)
                   + m_costs->get(city(j), city(j + 1), j);

            after = m_costs->get(city(i - 1), city(i + 1), i - 1)
                  + m_costs->get(city(j), city(i), j - 1)
                  + m_costs->get(city(i), city(j + 1), j);

            for (std::uint16_t k = i; k < j - 1; ++k)
            {
                before += m_costs->get(city(k), city(k + 1), k);
                after  += m_costs->get(city(k + 1), city(k + 2), k);
            }
        }
        else if (j < i)
        {
            if (i - j > 30)
                return std::numeric_limits<std::int32_t>::max();

            before = m_costs->get(city(j - 1), city(j), j - 1)
                   + m_costs->get(city(j), city(j + 1), j)
                   + m_costs->get(city(i), city(i + 1), i);

            after = m_costs->get(city(j - 1), city(i), j - 1)
                  + m_costs->get(city(i), city(j), j)
                  + m_costs->get(city(i - 1), city(i + 1), i);

            for (std::uint16_t k = j + 1; k < i; ++k)
            {
                before += m_costs->get(city(k), city(k + 1), k);
                after  += m_costs->get(city(k - 1), city(k), k);
            }
        }
        else
        {
            before = 0;
            after = 0;
        }

        return after - before;
    }

    std::int32_t select_city_cost_diff(std::uint16_t zone_idx, std::uint16_t new_city_pos) const noexcept
    {
        // zone_idx will never be index of zone on day zero, because on day zero there is always one city
        // and it is ensured that we generate only zone_idx of zones with more than one city.
        assert(zone_idx > 0);
        auto day = m_area_to_day[zone_idx];
        auto city_before_idx = city(day - 1);

        std::int32_t before = m_costs->get(city_before_idx, m_path[zone_idx][0], day - 1);
        std::int32_t after  = m_costs->get(city_before_idx, m_path[zone_idx][new_city_pos], day - 1);

        if (day < m_path.size() - 1)
        {
            auto city_after_idx = city(day + 1);
            before += m_costs->get(m_path[zone_idx][0], city_after_idx, day);
            after  += m_costs->get(m_path[zone_idx][new_city_pos], city_after_idx, day);
        }

        return after - before;
    }

    void swap_areas(std::uint16_t i, std::uint16_t j) noexcept
    {
        std::swap(m_day_to_area[i], m_day_to_area[j]);
        m_area_to_day[m_day_to_area[i]] = i;
        m_area_to_day[m_day_to_area[j]] = j;
    }

    void reverse_areas(std::uint16_t i, std::uint16_t j) noexcept
    {
        auto k = std::min(i, j);
        auto l = std::max(i, j);

        auto end = ((k + l) / 2);
        for (auto idx = k; idx <= end; ++idx)
        {
            std::swap(m_day_to_area[idx], m_day_to_area[k + l - idx]);
            m_area_to_day[m_day_to_area[idx]] = idx;
            m_area_to_day[m_day_to_area[k + l - idx]] = k + l - idx;
        }
    }

    void insert_areas(std::uint16_t i, std::uint16_t j) noexcept
    {
        std::uint16_t k, l;

        auto tmp = m_day_to_area[i];
        if (i < j)
        {
            for (auto m = i; m < j; ++m)
                m_day_to_area[m] = m_day_to_area[m + 1];

            k = i;
            l = j;
        }
        else
        {
            for (auto m = i; m > j; --m)
                m_day_to_area[m] = m_day_to_area[m - 1];

            k = j;
            l = i;
        }
        m_day_to_area[j] = tmp;

        for (auto m = k; m < l + 1; ++m)
            m_area_to_day[m_day_to_area[m]] = m;
    }

    void select_city(std::uint16_t zone_idx, std::uint16_t new_city_pos) noexcept
    {
        std::swap(m_path[zone_idx][0], m_path[zone_idx][new_city_pos]);
    }

    struct area_city_t
    {
        std::uint16_t zone_idx;
        std::uint16_t city_pos;
    };

    // The path!
    std::vector<area_t> m_path;
    // Supported structures (permutation & inverze permutation) to be able to find
    // effectively areas before and after a chosen area at a day.
    std::vector<std::uint16_t> m_day_to_area;
    std::vector<std::uint16_t> m_area_to_day;

    // The vector of pairs (area idx, city position in area) for areas where is more
    // than one city. To be able to switch cities in a areas.
    std::vector<area_city_t> m_cities_choises;

    // A sources of data.
    const cities_map_t * m_cities_indexer;
    const matrix<std::uint16_t> * m_costs;
};
