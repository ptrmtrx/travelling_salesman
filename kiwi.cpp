/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "city.h"
#include "config.h"
#include "matrix.h"
#include "parser.h"
#include "path.h"
#include "random.h"

// Start of the program
static const auto g_start_time = std::chrono::high_resolution_clock::now();
std::atomic<bool> g_continue_run(true);

static std::thread set_time_limit(std::size_t cities_count, std::size_t areas_count)
{
    using namespace std::chrono_literals;

    auto time = 15s;
    if (areas_count <= 20 && cities_count < 50)
        time = 3s;
    else if (areas_count <= 100 && cities_count < 200)
        time = 5s;

    auto elapsed_time = std::chrono::high_resolution_clock::now() - g_start_time;
    return std::thread([=]{ std::this_thread::sleep_for(time - elapsed_time - 50ms); g_continue_run = false; });
}


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
    char * from, * to;
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

    // Generate a random path.
    areapath_t path(std::move(areas_list), &cities_indexer, &costs_matrix);

    // Print the optimized path and the cost.
    path.optimize();
    path.print(std::cout);

    timeout.join();
    return 0;
}
