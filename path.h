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

#include "area.h"
#include "city.h"
#include "random.h"

//extern config g_config;
extern std::atomic<bool> g_continue_run;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class path_base_t
{
public:
    virtual void optimize() = 0;
    virtual void print(std::ostream & out) = 0;

    virtual ~path_base_t() = default;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static constexpr double get_last_t(std::size_t cities)
{
    if (cities < 55)
        return 0.005;

    if (cities < 105)
        return 0.002;

    return 0.0005;
}


class path_t
{
public:
	path_t(const cities_map_t & cities_indexer, const matrix<std::uint16_t> & costs_matrix, std::uint16_t start_city)
        : m_cities_indexer{cities_indexer}
        , m_costs{costs_matrix}
	{
        auto cities = cities_indexer.count();
		m_path.resize(cities + 1);

		// Fill with the increasing sequence and set the first and last city.
		std::iota(m_path.begin(), m_path.end(), std::uint16_t(0));
		m_path[0] = m_path[cities] = start_city;
		m_path[start_city] = 0;

		// Create a random permutation.
		std::random_shuffle(m_path.begin() + 1, m_path.begin() + cities);
	}

    void optimize()
    {
        rnd_gen_t rng;

        auto min_path = m_path;
        auto min_cost = cost();

        auto actual_cost = min_cost;

        // Create uniform random generator for generating inner indexes in the path.
        std::uniform_int_distribution<std::uint16_t> gen_idx(1, static_cast<std::uint16_t>(cities_in_path() - 2));

        // Create uniform random generator for generating random numbers on std::uint32_t.
        std::uniform_int_distribution<std::uint32_t> gen_uint32(0, std::numeric_limits<std::uint32_t>::max());

        // Some constants for temperature computing.
        auto Tn = /*g_config.iterations*/ 110 * 1000 * 1000;
        auto exp_base = std::log(get_last_t(cities_in_path()));

        double actual_T = 1.0;

        unsigned int iter = 0;
        while (g_continue_run)
        {
            ++iter;

            if (iter % 512 /*g_config.recomp_T*/ == 1)
                actual_T = std::exp(exp_base * std::pow((double) iter / (double) Tn, 0.3));

            // Randomly choose two indexes.
            auto i = gen_idx(rng);
            auto j = gen_idx(rng);

            // Compute the best price.
            enum method_t { SWAP, REVERSE, INSERT } method;
            auto cost_diff = std::numeric_limits<std::int32_t>::max();

            //if (g_config.use_swap)
            {
                method = SWAP;
                cost_diff = swap_cost_diff(i, j);
            }

            //if (g_config.use_reverse)
            {
                auto price = reverse_cost_diff(i, j);
                if (price < cost_diff)
                {
                    cost_diff = price;
                    method = REVERSE;
                }
            }

            //if (g_config.use_insert)
            {
                auto price = insert_cost_diff(i, j);
                if (price < cost_diff)
                {
                    cost_diff = price;
                    method = INSERT;
                }
            }

            // Accept? Better ways accept every time || worse only with some probability.
            bool accept = true;
            if (cost_diff > 0)
            {
                auto rnd = gen_uint32(rng);
                auto log_max_int = std::log(std::numeric_limits<std::uint32_t>::max());

                auto right = (-cost_diff / (actual_T * m_costs.get_max())) + log_max_int;
                auto left = std::log(rnd);

                if (left > right)
                {
                    accept = false;
                }
            }

            // If the new path should be accepted, save it.
            if (accept)
            {
                switch (method)
                {
                case SWAP:    swap(i, j); break;
                case REVERSE: reverse(i, j); break;
                case INSERT:  insert(i, j); break;
                }

                actual_cost += cost_diff;

                // If the actual path cost is the best one, save it.
                if (actual_cost < min_cost)
                {
                    min_path = m_path;
                    min_cost = actual_cost;
                }
            }
        }

        m_path = min_path;
    }

    void print(std::ostream & out) const
    {
        // Print the cost.
        out << cost() << std::endl;

        // Print the path.
        for (size_t i = 0; i < m_path.size() - 1; ++i)
        {
            auto src_idx = m_path[i];
            auto dst_idx = m_path[i + 1];

            out << m_cities_indexer.get_city_object(src_idx);
            out << ' ';
            out << m_cities_indexer.get_city_object(dst_idx);
            out << ' ';
            out << i;
            out << ' ';
            out << m_costs.get(src_idx, dst_idx, (std::uint16_t)i);
            out << std::endl;
        }
    }

private:
    std::size_t cities_in_path() const noexcept
    {
        return m_path.size();
    }

    std::uint32_t cost() const noexcept
    {
        std::uint32_t sum = 0;
        for (size_t i = 0; i < m_path.size() - 1; ++i)
        {
            sum += m_costs.get(m_path[i], m_path[i + 1], (std::uint16_t)i);
        }
        return sum;
    }

    std::int32_t swap_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        std::int32_t before;
        std::int32_t after;

        auto pim1 = m_path[i - 1];
        auto pi = m_path[i];
        auto pip1 = m_path[i + 1];

        auto pjm1 = m_path[j - 1];
        auto pj = m_path[j];
        auto pjp1 = m_path[j + 1];

        if (std::abs(i - j) > 1)
        {
            before = m_costs.get(pim1, pi, i - 1) + m_costs.get(pi, pip1, i)
                   + m_costs.get(pjm1, pj, j - 1) + m_costs.get(pj, pjp1, j);
            after  = m_costs.get(pim1, pj, i - 1) + m_costs.get(pj, pip1, i)
                   + m_costs.get(pjm1, pi, j - 1) + m_costs.get(pi, pjp1, j);
        }
        else if (i + 1 == j)
        {
            before = m_costs.get(pim1, pi, i - 1) + m_costs.get(pi, pj, i) + m_costs.get(pj, pjp1, j);
            after  = m_costs.get(pim1, pj, i - 1) + m_costs.get(pj, pi, i) + m_costs.get(pi, pjp1, j);
        }
        else if (i - 1 == j)
        {
            before = m_costs.get(pjm1, pj, j - 1) + m_costs.get(pj, pi, j) + m_costs.get(pi, pip1, i);
            after  = m_costs.get(pjm1, pi, j - 1) + m_costs.get(pi, pj, j) + m_costs.get(pj, pip1, i);
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

        auto before = m_costs.get(m_path[k - 1], m_path[k], k - 1) + m_costs.get(m_path[l], m_path[l + 1], l);
        auto after = m_costs.get(m_path[k - 1], m_path[l], k - 1) + m_costs.get(m_path[k], m_path[l + 1], l);

        auto end = l - k;
        for (std::uint16_t idx = 0; idx < end; ++idx)
        {
            before += m_costs.get(m_path[k + idx], m_path[k + idx + 1], k + idx);
            after += m_costs.get(m_path[l - idx], m_path[l - idx - 1], k + idx);
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

            before = m_costs.get(m_path[i - 1], m_path[i], i - 1)
                + m_costs.get(m_path[j - 1], m_path[j], j - 1)
                + m_costs.get(m_path[j], m_path[j + 1], j);

            after = m_costs.get(m_path[i - 1], m_path[i + 1], i - 1)
                + m_costs.get(m_path[j], m_path[i], j - 1)
                + m_costs.get(m_path[i], m_path[j + 1], j);

            for (std::uint16_t k = i; k < j - 1; ++k)
            {
                before += m_costs.get(m_path[k], m_path[k + 1], k);
                after += m_costs.get(m_path[k + 1], m_path[k + 2], k);
            }
        }
        else if (j < i)
        {
            if (i - j > 30)
                return std::numeric_limits<std::int32_t>::max();

            before = m_costs.get(m_path[j - 1], m_path[j], j - 1)
                + m_costs.get(m_path[j], m_path[j + 1], j)
                + m_costs.get(m_path[i], m_path[i + 1], i);

            after = m_costs.get(m_path[j - 1], m_path[i], j - 1)
                + m_costs.get(m_path[i], m_path[j], j)
                + m_costs.get(m_path[i - 1], m_path[i + 1], i);

            for (std::uint16_t k = j + 1; k < i; ++k)
            {
                before += m_costs.get(m_path[k], m_path[k + 1], k);
                after += m_costs.get(m_path[k - 1], m_path[k], k);
            }
        }
        else
        {
            before = 0;
            after = 0;
        }

        return after - before;
    }

    void swap(std::uint16_t i, std::uint16_t j) noexcept
    {
        std::swap(m_path[i], m_path[j]);
    }

    void reverse(std::uint16_t i, std::uint16_t j) noexcept
    {
        auto k = std::min(i, j);
        auto l = std::max(i, j);

        auto end = ((k + l) / 2);
        for (int idx = k; idx <= end; ++idx)
            std::swap(m_path[idx], m_path[k + l - idx]);
    }

    void insert(std::uint16_t i, std::uint16_t j) noexcept
    {
        auto beg = m_path.begin();

        if (i < j)
            std::rotate(beg + i, beg + i + 1, beg + j + 1);
        else
            std::rotate(beg + j, beg + i, beg + i + 1);
    }

	std::vector<std::uint16_t> m_path;

    const cities_map_t & m_cities_indexer;
    const matrix<std::uint16_t> & m_costs;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class areapath_t
{
public:
    areapath_t(std::vector<area_t> && areas_list, cities_map_t & cities_indexer, matrix<std::uint16_t> & costs_matrix)
        : m_path{std::move(areas_list)}
        , m_cities_indexer{cities_indexer}
        , m_costs{costs_matrix}
    {
        // Set tha last area same as the first.
        m_path.push_back(m_path[0]);
    }

    void optimize()
    {
        rnd_gen_t rng;

        auto min_path = m_path;
        auto min_cost = cost();

        auto actual_cost = min_cost;

        // Create uniform random generator for generating inner indexes in the path.
//        std::uniform_int_distribution<std::uint16_t> gen_idx(1, static_cast<std::uint16_t>(cities_in_path() - 2));

        // Create uniform random generator for generating random numbers on std::uint32_t.
        std::uniform_int_distribution<std::uint32_t> gen_uint32(0, std::numeric_limits<std::uint32_t>::max());

        // Some constants for temperature computing.
//        auto Tn = /*g_config.iterations*/ 110 * 1000 * 1000;
//        auto exp_base = std::log(get_last_t(cities_in_path()));

        double actual_T = 1.0;

        unsigned int iter = 0;
        while (g_continue_run)
        {
            ++iter;

//            if (iter % 512 /*g_config.recomp_T*/ == 1)
//                actual_T = std::exp(exp_base * std::pow((double) iter / (double) Tn, 0.3));

            // Randomly choose two indexes.
//            auto i = gen_idx(rng);
//            auto j = gen_idx(rng);

            // Compute the best price.
            enum method_t { SWAP, REVERSE, INSERT } method;
            auto cost_diff = std::numeric_limits<std::int32_t>::max();

            //if (g_config.use_swap)
            {
                method = SWAP;
                //cost_diff = swap_cost_diff(i, j);
            }

            // Accept? Better ways accept every time || worse only with some probability.
            bool accept = true;
            if (cost_diff > 0)
            {
                auto rnd = gen_uint32(rng);
                auto log_max_int = std::log(std::numeric_limits<std::uint32_t>::max());

                auto right = (-cost_diff / (actual_T * m_costs.get_max())) + log_max_int;
                auto left = std::log(rnd);

                if (left > right)
                {
                    accept = false;
                }
            }

            // If the new path should be accepted, save it.
            if (accept)
            {
                //switch (method)
                //{
                //case SWAP:    swap(i, j); break;
                //case REVERSE: reverse(i, j); break;
                //case INSERT:  insert(i, j); break;
                //}

                actual_cost += cost_diff;

                // If the actual path cost is the best one, save it.
                if (actual_cost < min_cost)
                {
                    min_path = m_path;
                    min_cost = actual_cost;
                }
            }
        }

        m_path = min_path;
    }

    void print(std::ostream & out) const
    {
        // Print the cost.
        out << cost() << std::endl;

        // Print the path.
        for (size_t i = 0; i < m_path.size() - 1; ++i)
        {
            auto src_idx = m_path[i];
            auto dst_idx = m_path[i + 1];

        //    out << map.get_city_object(src_idx);
            out << ' ';
        //    out << map.get_city_object(dst_idx);
            out << ' ';
            out << i;
            out << ' ';
        //    out << m_costs.get(src_idx, dst_idx, (std::uint16_t)i);
            out << std::endl;
        }
    }

private:
    std::uint32_t cost() const noexcept
    {
        std::uint32_t sum = 0;
        std::uint16_t from = 0; // start_city (always zero by design)
        for (std::size_t i = 0; i < m_path.size() - 1; ++i)
        {
            auto to = m_path[i].get_selected_city();
            sum += m_costs.get(from, to, static_cast<std::uint16_t>(i));
            from = to;
        }
        return sum;
    }

    std::int32_t swap_areas_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        return 0;
    }

    void swap_areas(std::uint16_t i, std::uint16_t j) noexcept
    {
        
    }

    std::int32_t select_city_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        return 0;
    }

    void select_city(std::uint16_t i, std::uint16_t j) noexcept
    {
    }

    std::vector<area_t> m_path;

    // A sources of data.
    const cities_map_t & m_cities_indexer;
    const matrix<std::uint16_t> & m_costs;
};
