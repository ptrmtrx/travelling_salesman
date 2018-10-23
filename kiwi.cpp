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

#include "area.h"
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
    else if (areas_count <= 50 && cities_count < 200)
        time = 5s;

    auto elapsed_time = std::chrono::high_resolution_clock::now() - g_start_time;
    std::cout << "parsovani az do nastaveni casu je: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() << std::endl;

    return std::thread([=]{ std::this_thread::sleep_for(time - elapsed_time - 100ms); g_continue_run = false; });
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

static void parse_input_data(cities_map_t & cities_indexer, areas_map_t & areas_indexer, matrix<std::uint16_t> & costs_matrix)
{
    parser_t parser;

    // Read number of locations and start city.
    std::uint16_t areas_count;
    char * tmp_str;
    parser.parse_line(areas_count, tmp_str);

    auto idx_start = cities_indexer.get_city_index(city_t(tmp_str));

    // Load areas.
    areas_indexer.reserve(areas_count);
    for (int i = 0; i < areas_count; ++i)
    {
        auto area_name = std::string(parser.read_line());
        auto cities = cities_names_to_cities_idx(parser.read_line(), cities_indexer);

        // Check if the area contains start_city.
        auto it = std::find(cities.cbegin(), cities.cend(), idx_start);
        if (it != cities.cend())
        {
        }

        /*auto area_idx =*/ areas_indexer.add_area(area_t(std::move(area_name), std::move(cities)));
    }

    // Save all flights to the matrix.
    costs_matrix.set_dim(cities_indexer.count());
    char * from, * to;
    std::uint16_t day, price;
    while (parser.parse_line(from, to, day, price))
    {
        auto idx_src = cities_indexer.get_city_index(city_t(from));
        auto idx_dst = cities_indexer.get_city_index(city_t(to));

        if (day)
            costs_matrix.set(idx_src, idx_dst, day - 1, price);
        else
            assert(!"zero day");
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main()
{
    // Create the holders of cities and areas [name <-> index] and price matrix.
    cities_map_t cities_indexer;
    areas_map_t  areas_indexer;
    matrix<std::uint16_t> costs_matrix;

    parse_input_data(cities_indexer, areas_indexer, costs_matrix);

    // Set timer to the end.
    auto timeout = set_time_limit(cities_indexer.count(), areas_indexer.count());

    // Generate the path between N cities.
    // Use the optimised solver in case there is only one city in each area.
    if (areas_indexer.count() == cities_indexer.count())
    {
        // Generate a random path.
        path_t path(cities_indexer, costs_matrix, 0);

        // Print the optimized path and the cost.
        path.optimize();
        path.print(std::cout);
    }
    else
    {
        // Generate a random path.
        areapath_t path(areas_indexer, cities_indexer, costs_matrix, 0);

        // Print the optimized path and the cost.
        path.optimize();
        path.print(std::cout);
    }

    timeout.join();

    std::cout << "execution time is: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - g_start_time).count() << std::endl;

    return 0;
}
