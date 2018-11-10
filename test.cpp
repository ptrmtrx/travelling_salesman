/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>


class city_t
{
public:
    explicit constexpr city_t(const char * str)
        : m_code{ str[0], str[1], str[2] }
    {
    }

    constexpr bool operator<(const city_t & other) const noexcept
    {
        return m_code[0] < other.m_code[0]
            || m_code[1] < other.m_code[1]
            || m_code[2] < other.m_code[2];
    }

    constexpr bool operator==(const city_t & other) const noexcept
    {
        return m_code[0] == other.m_code[0]
            && m_code[1] == other.m_code[1]
            && m_code[2] == other.m_code[2];

        //return std::equal(m_code, m_code + sizeof(m_code), other.m_code);
    }

    constexpr std::size_t hash() const noexcept
    {
        return m_code[0]
            + (m_code[1] << 8)
            + (m_code[2] << 16);
    }

    friend std::ostream & operator<<(std::ostream & out, const city_t & rec)
    {
        out << rec.m_code[0] << rec.m_code[1] << rec.m_code[2];
        return out;
    }

private:
    char m_code[3]; // IATA codes - 3 English alphabet uppercase letters A-Z
};


struct city_hasher
{
    std::size_t operator()(const city_t & c) const noexcept
    {
        return c.hash();
    }
};


class cities_map_t
{
public:
    cities_map_t()
    {
        m_map.reserve(300);
    }

    std::uint16_t get_city_index(const city_t & city)
    {
        if (m_map.find(city) != m_map.end())
            return m_map[city];

        return m_map[city] = m_last_idx++;
    }

    city_t get_city_object(std::uint16_t idx) const
    {
        for (const auto & pair : m_map)
        {
            if (pair.second == idx)
                return pair.first;
        }

        throw std::runtime_error("get_city_object");
    }

    std::size_t count() const
    {
        return m_map.size();
    }

private:
    std::unordered_map<city_t, std::uint16_t, city_hasher> m_map;
    std::uint16_t m_last_idx = 0;
};


typedef std::vector<std::uint16_t> area_t;


template <typename T>
class matrix
{
public:
    constexpr matrix()
        : m_dim{ 0 }
        , m_matrix{ nullptr }
        , m_max_val{ std::numeric_limits<T>::min() }
    {
    }

    matrix(unsigned int dim)
    {
        set_dim(dim);
    }

    matrix(const matrix<T> &) = delete;
    matrix<T> & operator=(const matrix<T> &) = delete;

    ~matrix()
    {
        delete[] m_matrix;
    }

    void set_dim(unsigned int dim)
    {
        auto length = dim * dim * dim;

        m_dim = dim;
        m_matrix = new T[length];
        m_max_val = std::numeric_limits<T>::min();

        std::fill_n(m_matrix, length, std::numeric_limits<T>::max());
    }

    T get_max() const noexcept
    {
        return m_max_val;
    }

    std::int32_t get(std::uint16_t x, std::uint16_t y, std::uint16_t z) const noexcept
    {
        return m_matrix[(x * m_dim * m_dim) + (y * m_dim) + (z)];
    }

    void set(std::uint16_t x, std::uint16_t y, std::uint16_t z, T value) noexcept
    {
        if (value < get(x, y, z))
        {
            m_matrix[(x * m_dim * m_dim) + (y * m_dim) + (z)] = value;

            if (value > m_max_val)
                m_max_val = value;
        }
    }

private:
    unsigned int m_dim;
    T * m_matrix;
    T   m_max_val;
};


class parser_t
{
public:
    parser_t() = default;

    // Returns whole line, nullptr on EOF.
    char * read_line()
    {
        return next_line();
    }

    void parse_line(std::uint16_t & num, char *& str)
    {
        auto line = next_line();
        read_uint16(line, num);
        read_str(line, str);
    }

    bool parse_line(char *& from, char *& to, std::uint16_t & day, std::uint16_t & price)
    {
        auto line = next_line();
        if (!line)
            return false;

        read_str(line, from);
        read_str(line, to);
        read_uint16(line, day);
        read_uint16(line, price);
        return true;
    }

private:
    static void read_str(char *& line, char *& str)
    {
        str = line;

        int i = 0;
        char c = *line++;
        while (c != ' ' && c != '\n')
        {
            ++i;
            c = *line++;
        }
        str[i] = '\0';
    }

    static void read_uint16(char *& line, std::uint16_t & num)
    {
        num = 0;

        char c = *line++;
        while (c != ' ' && c != '\n')
        {
            num = 10 * num + (c - '0');
            c = *line++;
        }
    }

    // Returns whole line from stdin, nullptr on EOF.
    char * next_line()
    {
        return std::fgets(m_buffer, sizeof(m_buffer), stdin);
    }

    char m_buffer[2048];
};

class xoroshiro128plus
{
public:
    typedef std::uint64_t result_type;

    xoroshiro128plus(std::uint64_t seed = 0)
    {
        m_state[0] = (seed + 249863319);
        m_state[1] = (seed + 249863319);
    }

    xoroshiro128plus(xoroshiro128plus &&) noexcept = default;
    xoroshiro128plus & operator=(xoroshiro128plus &&) noexcept = default;

    xoroshiro128plus(xoroshiro128plus &) = default;
    xoroshiro128plus & operator=(xoroshiro128plus &) = default;

    result_type operator()() noexcept
    {
        auto const s0 = m_state[0];
        auto s1 = m_state[1];
        auto const result = s0 + s1;

        s1 ^= s0;
        m_state[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16);
        m_state[1] = rotl(s1, 37);

        return result;
    }

    static result_type min() noexcept { return 0; }
    static result_type max() noexcept { return std::numeric_limits<result_type>::max(); }

private:
    static inline std::uint64_t rotl(std::uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    std::uint64_t m_state[2];
};

//typedef std::mt19937     rnd_gen_t;
//typedef std::mt19937_64  rnd_gen_t;
//typedef xorshift128plus  rnd_gen_t;
typedef xoroshiro128plus rnd_gen_t;


 // Start of the program
static const auto g_start_time = std::chrono::high_resolution_clock::now();
static std::atomic<bool> g_continue_run(true);

static std::thread set_time_limit(std::size_t cities_count, std::size_t areas_count)
{
    using namespace std::chrono_literals;

    auto time = 15s;
    if (areas_count <= 20 && cities_count < 50)
        time = 3s;
    else if (areas_count <= 100 && cities_count < 200)
        time = 5s;

    auto elapsed_time = std::chrono::high_resolution_clock::now() - g_start_time;
    return std::thread([=] { std::this_thread::sleep_for(time - elapsed_time - 100ms); g_continue_run = false; });
}

static constexpr double get_last_t(std::size_t /*cities*/)
{
    //if (cities < 55)
    //    return 0.005;

    //if (cities < 105)
    //    return 0.002;

    return 0.005;
}

static constexpr std::uint32_t get_iter_count(std::size_t /*cities*/)
{
    //if (cities < 55)
    //    return 0.005;

    //if (cities < 105)
    //    return 40'000'000;

    return 100'000'000;
}

static constexpr std::uint16_t bound_value(std::uint16_t rnd, std::uint16_t range)
{
    std::uint32_t x = static_cast<std::uint32_t>(rnd) * static_cast<std::uint32_t>(range);
    return x >> 16;
}


class path_t
{
public:
    path_t(const cities_map_t * cities_indexer, const matrix<std::uint16_t> * costs_matrix, std::uint16_t start_city)
        : m_cities_indexer{ cities_indexer }
        , m_costs{ costs_matrix }
    {
        auto cities = cities_indexer->count();
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
        rnd_gen_t rng(std::chrono::system_clock::now().time_since_epoch().count());

        auto min_path = m_path;
        auto min_cost = cost();

        auto actual_cost = min_cost;

        // Some constants for temperature computing.
        auto Tn = get_iter_count(cities_in_path());
        auto exp_base = std::log(get_last_t(cities_in_path()));

        double actual_T = 1.0;

        unsigned int iter = 0;
        while (g_continue_run)
        {
            if (iter++ % 512 /*g_config.recomp_T*/ == 0)
                actual_T = std::exp(exp_base * std::pow(iter / (double) Tn, 0.3));

            // Randomly choose two indexes.
            auto xrnd = rng();
            static_assert(sizeof xrnd == 8, "Bad random number generator!");

            auto i = static_cast<std::uint16_t>(xrnd >> 32);
            auto j = static_cast<std::uint16_t>(xrnd >> 48);

            i = bound_value(i, static_cast<std::uint16_t>(m_path.size()) - 2) + 1;
            j = bound_value(j, static_cast<std::uint16_t>(m_path.size()) - 2) + 1;

            // Compute the best price.
            enum method_t { SWAP, REVERSE, INSERT } method;
            auto cost_diff = std::numeric_limits<std::int32_t>::max();

            {
                method = SWAP;
                cost_diff = swap_cost_diff(i, j);
            }
            {
                auto price = reverse_cost_diff(i, j);
                if (price < cost_diff)
                {
                    cost_diff = price;
                    method = REVERSE;
                }
            }
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
                auto rnd = static_cast<std::uint32_t>(xrnd);
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
        auto src_idx = m_path[0];
        auto src_city = m_cities_indexer->get_city_object(src_idx);

        for (std::uint16_t i = 0; i < m_path.size() - 1; ++i)
        {
            auto dst_idx = m_path[i + 1];
            auto dst_city = m_cities_indexer->get_city_object(dst_idx);

            out << src_city;
            out << ' ';
            out << dst_city;
            out << ' ';
            out << i + 1;
            out << ' ';
            out << m_costs->get(src_idx, dst_idx, i);
            out << std::endl;

            src_idx = dst_idx;
            src_city = dst_city;
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
        for (std::uint16_t i = 0; i < m_path.size() - 1; ++i)
        {
            sum += m_costs->get(m_path[i], m_path[i + 1], i);
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
            before = m_costs->get(pim1, pi, i - 1) + m_costs->get(pi, pip1, i)
                + m_costs->get(pjm1, pj, j - 1) + m_costs->get(pj, pjp1, j);
            after = m_costs->get(pim1, pj, i - 1) + m_costs->get(pj, pip1, i)
                + m_costs->get(pjm1, pi, j - 1) + m_costs->get(pi, pjp1, j);
        }
        else if (i + 1 == j)
        {
            before = m_costs->get(pim1, pi, i - 1) + m_costs->get(pi, pj, i) + m_costs->get(pj, pjp1, j);
            after = m_costs->get(pim1, pj, i - 1) + m_costs->get(pj, pi, i) + m_costs->get(pi, pjp1, j);
        }
        else if (i - 1 == j)
        {
            before = m_costs->get(pjm1, pj, j - 1) + m_costs->get(pj, pi, j) + m_costs->get(pi, pip1, i);
            after = m_costs->get(pjm1, pi, j - 1) + m_costs->get(pi, pj, j) + m_costs->get(pj, pip1, i);
        }
        else
        {
            before = 0;
            after = 0;
        }

        return after - before;
    }

    std::int32_t reverse_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
    {
        auto k = std::min(i, j);
        auto l = std::max(i, j);

        if (l - k > 30)
            return std::numeric_limits<std::int32_t>::max();

        auto before = m_costs->get(m_path[k - 1], m_path[k], k - 1) + m_costs->get(m_path[l], m_path[l + 1], l);
        auto after = m_costs->get(m_path[k - 1], m_path[l], k - 1) + m_costs->get(m_path[k], m_path[l + 1], l);

        auto end = l - k;
        for (std::uint16_t idx = 0; idx < end; ++idx)
        {
            before += m_costs->get(m_path[k + idx], m_path[k + idx + 1], k + idx);
            after += m_costs->get(m_path[l - idx], m_path[l - idx - 1], k + idx);
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

            before = m_costs->get(m_path[i - 1], m_path[i], i - 1)
                + m_costs->get(m_path[j - 1], m_path[j], j - 1)
                + m_costs->get(m_path[j], m_path[j + 1], j);

            after = m_costs->get(m_path[i - 1], m_path[i + 1], i - 1)
                + m_costs->get(m_path[j], m_path[i], j - 1)
                + m_costs->get(m_path[i], m_path[j + 1], j);

            for (std::uint16_t k = i; k < j - 1; ++k)
            {
                before += m_costs->get(m_path[k], m_path[k + 1], k);
                after += m_costs->get(m_path[k + 1], m_path[k + 2], k);
            }
        }
        else if (j < i)
        {
            if (i - j > 30)
                return std::numeric_limits<std::int32_t>::max();

            before = m_costs->get(m_path[j - 1], m_path[j], j - 1)
                + m_costs->get(m_path[j], m_path[j + 1], j)
                + m_costs->get(m_path[i], m_path[i + 1], i);

            after = m_costs->get(m_path[j - 1], m_path[i], j - 1)
                + m_costs->get(m_path[i], m_path[j], j)
                + m_costs->get(m_path[i - 1], m_path[i + 1], i);

            for (std::uint16_t k = j + 1; k < i; ++k)
            {
                before += m_costs->get(m_path[k], m_path[k + 1], k);
                after += m_costs->get(m_path[k - 1], m_path[k], k);
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

    const cities_map_t * m_cities_indexer;
    const matrix<std::uint16_t> * m_costs;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class areapath_t
{
public:
    areapath_t(std::vector<area_t> && areas_list, const cities_map_t * cities_indexer, const matrix<std::uint16_t> * costs_matrix)
        : m_path{ std::move(areas_list) }
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
                m_cities_choises.push_back({ i, j });
        m_cities_choises.shrink_to_fit();
    }

    void optimize()
    {
        rnd_gen_t rng(std::chrono::system_clock::now().time_since_epoch().count());

        auto min_path = *this;
        auto min_cost = cost();

        auto actual_cost = min_cost;

        // Some constants for temperature computing.
        auto Tn = get_iter_count(m_path.size());
        auto exp_base = std::log(get_last_t(m_path.size()));

        double actual_T = 1.0;

        unsigned int iter = 0;
        while (g_continue_run)
        {
            if (iter++ % 512 /*g_config.recomp_T*/ == 0)
                actual_T = std::exp(exp_base * std::pow(iter / (double) Tn, 0.3));

            // Randomly choose two indexes.
            std::uint16_t i, j;

            // Compute the best price.
            enum method_t { SWAP_AREAS, REVERSE_AREAS, INSERT_AREA, SELECT_CITY } method;
            auto cost_diff = std::numeric_limits<std::int32_t>::max();

            auto xrnd = rng();
            static_assert(sizeof xrnd == 8, "Bad random number generator!");

            {
                i = static_cast<std::uint16_t>(xrnd >> 32);
                j = static_cast<std::uint16_t>(xrnd >> 48);

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

            //if (m_cities_choises.size())
            {
                auto x = static_cast<std::uint16_t>(xrnd >> 32);
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
                auto rnd = static_cast<std::uint32_t>(xrnd);
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
        *this = min_path;
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
        auto from = static_cast<std::uint16_t>(0);// always zero: m_path[0].get_selected_city();
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
        auto pi = city(i);
        auto pip1 = city(i + 1);

        auto pjm1 = city(j - 1);
        auto pj = city(j);
        auto pjp1 = city(j + 1);

        if (std::abs(i - j) > 1)
        {
            before = m_costs->get(pim1, pi, i - 1) + m_costs->get(pi, pip1, i)
                + m_costs->get(pjm1, pj, j - 1) + m_costs->get(pj, pjp1, j);
            after = m_costs->get(pim1, pj, i - 1) + m_costs->get(pj, pip1, i)
                + m_costs->get(pjm1, pi, j - 1) + m_costs->get(pi, pjp1, j);
        }
        else if (i + 1 == j)
        {
            before = m_costs->get(pim1, pi, i - 1) + m_costs->get(pi, pj, i) + m_costs->get(pj, pjp1, j);
            after = m_costs->get(pim1, pj, i - 1) + m_costs->get(pj, pi, i) + m_costs->get(pi, pjp1, j);
        }
        else if (i - 1 == j)
        {
            before = m_costs->get(pjm1, pj, j - 1) + m_costs->get(pj, pi, j) + m_costs->get(pi, pip1, i);
            after = m_costs->get(pjm1, pi, j - 1) + m_costs->get(pi, pj, j) + m_costs->get(pj, pip1, i);
        }
        else
        {
            before = 0;
            after = 0;
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
        auto after = m_costs->get(city(k - 1), city(l), k - 1) + m_costs->get(city(k), city(l + 1), l);

        auto end = l - k;
        for (std::uint16_t idx = 0; idx < end; ++idx)
        {
            before += m_costs->get(city(k + idx), city(k + idx + 1), k + idx);
            after += m_costs->get(city(l - idx), city(l - idx - 1), k + idx);
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
                after += m_costs->get(city(k + 1), city(k + 2), k);
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
                after += m_costs->get(city(k - 1), city(k), k);
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
        auto day = m_area_to_day[zone_idx];
        auto city_before_idx = city(day - 1);

        std::int32_t before = m_costs->get(city_before_idx, m_path[zone_idx][0], day - 1);
        std::int32_t after = m_costs->get(city_before_idx, m_path[zone_idx][new_city_pos], day - 1);

        if (day < m_path.size() - 1)
        {
            auto city_after_idx = city(day + 1);
            before += m_costs->get(m_path[zone_idx][0], city_after_idx, day);
            after += m_costs->get(m_path[zone_idx][new_city_pos], city_after_idx, day);
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

    struct zone_city_t
    {
        std::uint16_t zone_idx;
        std::uint16_t city_pos;
    };

    std::vector<area_t> m_path;
    std::vector<std::uint16_t> m_day_to_area;
    std::vector<std::uint16_t> m_area_to_day;

    std::vector<zone_city_t> m_cities_choises;

    // A sources of data.
    const cities_map_t * m_cities_indexer;
    const matrix<std::uint16_t> * m_costs;
};


//// Global config data.
//config g_config;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static std::vector<std::uint16_t> cities_names_to_cities_idx(const char * city_names, cities_map_t & cities_indexer)
{
    // There must be at least one city.
    int count = 1;
    auto begin = city_names;

    // Just an alloc optimization.
    while (city_names[3] == ' ')
    {
        ++count;
        city_names += 4;
    }

    std::vector<std::uint16_t> ret;
    ret.reserve(count);

    // Add the cities.
    city_names = begin;
    for (int i = 0; i < count; ++i)
    {
        auto idx = cities_indexer.get_city_index(city_t(city_names));
        ret.push_back(idx);
        city_names += 4;
    }
    return ret;
}

static void parse_input_data(cities_map_t & cities_indexer, std::vector<area_t> & areas_list, matrix<std::uint16_t> & costs_matrix)
{
    parser_t parser;

    // Read number of locations and start city.
    std::uint16_t areas_count;
    char * tmp_str;
    parser.parse_line(areas_count, tmp_str);

    /*auto idx_start = */cities_indexer.get_city_index(city_t(tmp_str));

    // Load areas.
    areas_list.clear();
    areas_list.reserve(areas_count + 1);
    areas_list.push_back(area_t(std::vector<std::uint16_t>())); // dummy area
    for (int i = 0; i < areas_count; ++i)
    {
        parser.read_line();
        auto cities = cities_names_to_cities_idx(parser.read_line(), cities_indexer);

        // Check if the area contains start_city.
        const auto it = std::find(cities.cbegin(), cities.cend(), /*idx_start*/0);
        if (it != cities.cend())
        {
            auto pos = std::distance(it, cities.cbegin());
            // Replace dummy area.
            areas_list[0] = area_t(std::move(cities));
            std::swap(areas_list[0], areas_list[pos]);
        }
        else
            areas_list.push_back(area_t(/*std::move(area_name),*/ std::move(cities)));
    }

    // Save all flights to the matrix.
    costs_matrix.set_dim(static_cast<unsigned int>(cities_indexer.count()));
    char * from, *to;
    std::uint16_t day, price;
    while (parser.parse_line(from, to, day, price))
    {
        auto idx_src = cities_indexer.get_city_index(city_t(from));
        auto idx_dst = cities_indexer.get_city_index(city_t(to));

        if (day)
            costs_matrix.set(idx_src, idx_dst, day - 1, price);
        else
        {
            auto count = cities_indexer.count();
            for (std::uint16_t j = 0; j < count; ++j)
                costs_matrix.set(idx_src, idx_dst, j, price);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main()
{
    // Create the holders of cities and areas [name <-> index] and price matrix.
    cities_map_t cities_indexer;
    std::vector<area_t> areas_list;
    matrix<std::uint16_t> costs_matrix;

    parse_input_data(cities_indexer, areas_list, costs_matrix);

    // Set timer to the end.
    auto timeout = set_time_limit(cities_indexer.count(), areas_list.size());

    // Generate the path between N cities.
    // Use the optimised solver in case there is only one city in each area.
    if (areas_list.size() == cities_indexer.count())
    {
        // Generate a random path.
        path_t path(&cities_indexer, &costs_matrix, 0);

        // Print the optimized path and the cost.
        path.optimize();
        path.print(std::cout);
    }
    else
    {
        // Generate a random path.
        areapath_t path(std::move(areas_list), &cities_indexer, &costs_matrix);

        // Print the optimized path and the cost.
        path.optimize();
        path.print(std::cout);
    }

    timeout.join();
    return 0;
}
