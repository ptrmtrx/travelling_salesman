/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "city.h"
#include "config.h"
#include "csv.h"
#include "matrix.h"
#include "path.h"


// Simulation of: /usr/bin/timeout --signal=SIGTERM --kill-after=1s 30s /app/run < data_300.txt
//std::thread timeout([]
//{
//	std::this_thread::sleep_for(std::chrono::seconds(30));
//	std::raise(SIGTERM);
//});

//// Global config data.
//config g_config;
matrix<std::uint16_t> path_t::s_costs(300);
std::atomic<bool> g_continue_run(true);

void signal_handler(int signum)
{
	if (signum == SIGTERM)
		g_continue_run = false;
}

//
//class xorshift
//{
//public:
//	typedef std::uint64_t result_type;
//
//	xorshift(std::uint32_t seed)
//		: m_state(seed + 249863319)
//		, m_state1(seed + 2498633199)
//	{
//	}
//
//	xorshift(xorshift &&) = default;
//	xorshift & operator=(xorshift &&) = default;
//
//	xorshift(xorshift &) = default;
//	xorshift & operator=(xorshift &) = default;
//
//	result_type operator()()
//	{
//		// varianta 1 z wiki
//		//std::uint32_t x = m_state;
//		//x ^= x << 13;
//		//x ^= x >> 17;
//		//x ^= x << 5;
//		//m_state = x;
//		//return x;
//
//		// varianta 2 z wiki
//		//uint64_t x = m_state;
//		//x ^= x >> 12; // a
//		//x ^= x << 25; // b
//		//x ^= x >> 27; // c
//		//m_state = x;
//		//return x * 0x2545F4914F6CDD1D;
//
//		// varianta 3 z wiki
//		std::uint64_t x = m_state;
//		std::uint64_t const y = m_state1;
//		m_state = y;
//		x ^= x << 23; // a
//		m_state1 = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
//		return m_state1 + y;
//	}
//
//	static result_type min() noexcept
//	{
//		return 0;
//	}
//
//	static result_type max() noexcept
//	{
//		return std::numeric_limits<result_type>::max();
//	}
//
//private:
//	std::uint64_t m_state;
//	std::uint64_t m_state1;
//};



enum method_t
{
	SWAP,
	REVERSE,
	INSERT
};

inline double get_last_t(std::size_t cities)
{
	if (cities < 55)
		return 0.005;

	if (cities < 105)
		return 0.002;

	return 0.0005;
}

path_t solver(path_t path, std::mt19937 rng, path_t * result)
//path_t solver(path_t path, xorshift rng, path_t * result)
{
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
		method_t method;
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

	*result = min_path;
	return min_path;
}


class task
{
public:
	task(unsigned int idx, int cities, std::uint16_t start_city)
		: m_idx(idx)
		, m_rng(idx + 60 /*g_config.seed*/)
		, m_path(cities, start_city)
	{
	}

	void start()
	{
		m_worker = std::thread(solver, m_path, m_rng, &m_path);
	}

	void wait()
	{
		if (m_worker.joinable())
			m_worker.join();
	}

	const path_t * get_path_ptr() const
	{
		return & m_path;
	}

private:
	unsigned int m_idx;
	std::thread  m_worker;
	std::mt19937 m_rng;
	//xorshift     m_rng;
	path_t       m_path;
};


int main()
{
	// Set SIGTERM handler.
	std::signal(SIGTERM, signal_handler);

	// Helper variables for line parsing.
	char * from, * to;
	std::uint16_t day, price;

	// Create the holder of cities map [name <-> index].
	cities_map cities;

	// Create the stdin data reader.
	io::CSVReader<4, io::trim_chars<>, io::no_quote_escape<' '>> in("x:\\kiwi\\data_300.txt", stdin);

	// Save the start/end city.
	in.read_row(from, to, day, price);
	auto start_city = cities.get_city_index(from);

	// Save all flights to the matrix.
	matrix<std::uint16_t> & costs_matrix = path_t::s_costs;
	while (in.read_row(from, to, day, price))
	{
		auto idx_src = cities.get_city_index(from);
		auto idx_dst = cities.get_city_index(to);

		costs_matrix.set(idx_src, idx_dst, day, price);
	}

	//// Create random generator.
	//std::mt19937 rng;

	//// Generate the path between N cities.
	//path_t init_path(cities.count(), start_city);

	//// Optimize the path.
	//auto path = solver(init_path, rng);

	//// Print the path and the cost.
	//path.print(std::cout, cities);

	const int TASKS_NUM = 4;

	task * tasks[TASKS_NUM];
	for (unsigned int i = 0; i < TASKS_NUM; ++i)
	{
		tasks[i] = new task(i, (int)cities.count(), start_city);
		tasks[i]->start();

		//std::cout << "poustim: " << i << std::endl;
	}

	for (int i = 0; i < TASKS_NUM; ++i)
	{
		tasks[i]->wait();

		//std::cout << "koncim: " << i << std::endl;
		//std::cout << "cena:   " << tasks[i]->get_path_ptr()->cost() << std::endl;
	}

	const path_t * final_path = tasks[0]->get_path_ptr();
	for (int i = 1; i < TASKS_NUM; ++i)
	{
		if (tasks[i]->get_path_ptr()->cost() < final_path->cost())
			final_path = tasks[i]->get_path_ptr();
	}

	final_path->print(std::cout, cities);

//	timeout.join();
    return 0;
}

