/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#include <atomic>
#include <cctype>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "area.h"
#include "city.h"
#include "config.h"
#include "csv.h"
#include "matrix.h"
#include "parser.h"
#include "path.h"
#include "random.h"


// Simulation of: /usr/bin/timeout --signal=SIGTERM --kill-after=1s 30s /app/run < data_300.txt
//static std::thread timeout([]
//{
//	std::this_thread::sleep_for(std::chrono::seconds(30));
//	std::raise(SIGTERM);
//});



//// Global config data.
//config g_config;
static std::atomic<bool> g_continue_run(true);

static void signal_handler(int signum)
{
	if (signum == SIGTERM)
		g_continue_run = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static inline double get_last_t(std::size_t cities)
{
	if (cities < 55)
		return 0.005;

	if (cities < 105)
		return 0.002;

	return 0.0005;
}

static path_t solver(path_t path)
{
    rnd_gen_t rng;

	auto min_path = path;
	auto min_cost = path.cost();

	auto actual_cost = min_cost;

	// Create uniform random generator for generating inner indexes in the path.
	std::uniform_int_distribution<std::uint16_t> gen_idx(1, static_cast<std::uint16_t>(path.cities_count() - 2));

	// Create uniform random generator for generating random numbers on std::uint32_t.
	std::uniform_int_distribution<std::uint32_t> gen_uint32(0, std::numeric_limits<std::uint32_t>::max());

	// Some constants for temperature computing.
	auto Tn = /*g_config.iterations*/ 110 * 1000 * 1000;
	auto exp_base = std::log(get_last_t(path.cities_count()));

	double actual_T = 1.0;

	unsigned int iter = 0;
	while (g_continue_run)
	{
		++iter;

		if (iter % 512 /*g_config.recomp_T*/ == 1)
			actual_T = std::exp(exp_base * std::pow((double)iter / (double)Tn, 0.3));

		// Randomly choose two indexes.
		auto i = gen_idx(rng);
		auto j = gen_idx(rng);

		// Compute the best price.
        enum method_t { SWAP, REVERSE, INSERT } method;
		auto cost_diff = std::numeric_limits<std::int32_t>::max();

		//if (g_config.use_swap)
		{
			method = SWAP;
			cost_diff = path.swap_cost_diff(i, j);
		}

		//if (g_config.use_reverse)
		{
			auto price = path.reverse_cost_diff(i, j);
			if (price < cost_diff)
			{
				cost_diff = price;
				method = REVERSE;
			}
		}

		//if (g_config.use_insert)
		{
			auto price = path.insert_cost_diff(i, j);
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

			auto right = (-cost_diff / (actual_T * path_t::s_costs.get_max())) + log_max_int;
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
			case SWAP:    path.swap(i, j); break;
			case REVERSE: path.reverse(i, j); break;
			case INSERT:  path.insert(i, j); break;
			}

			actual_cost += cost_diff;

			// If the actual path cost is the best one, save it.
			if (actual_cost < min_cost)
			{
				min_path = path;
				min_cost = actual_cost;
			}
		}
	}

	return min_path;
}

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
    parser.parse_row(areas_count, tmp_str);

    auto idx_start = cities_indexer.get_city_index(city_t(tmp_str));

    // Load areas.
    areas_indexer.reserve(areas_count);
    for (int i = 0; i < areas_count; ++i)
    {
        auto area = area_t(parser.read_line());
        areas_indexer.add_area(area);
        area.add_cities(cities_names_to_cities_idx(parser.read_line(), cities_indexer));
    }

    // Save all flights to the matrix.
    char * from, * to;
    std::uint16_t day, price;

    while (parser.parse_row(from, to, day, price))
    {
        auto idx_src = cities_indexer.get_city_index(city_t(from));
        auto idx_dst = cities_indexer.get_city_index(city_t(to));

        costs_matrix.set(idx_src, idx_dst, day, price);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main()
{
	// Set SIGTERM handler.
	std::signal(SIGTERM, signal_handler);

	// Create the holders of cities and areas [name <-> index] and price matrix.
	cities_map_t cities_indexer;
    areas_map_t  areas_indexer;
    matrix<std::uint16_t> costs_matrix(300);

    parse_input_data(cities_indexer, areas_indexer, costs_matrix);

	// Generate the path between N cities.
    std::uint16_t start_city = 0; // temporary
	path_t init_path(cities_indexer.count(), start_city);

	// Optimize the path.
	auto path = solver(init_path);

	// Print the path and the cost.
	path.print(std::cout, cities_indexer);

//	timeout.join();
    return 0;
}

