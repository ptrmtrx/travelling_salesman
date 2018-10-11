/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

//extern config g_config;

class path_t
{
public:
	path_t(std::size_t cities, std::uint16_t start_city)
	{
		m_path.resize(cities + 1);

		// Fill with the increasing sequence and set the first and last city.
		std::iota(m_path.begin(), m_path.end(), std::uint16_t(0));
		m_path[0] = m_path[cities] = start_city;
		m_path[start_city] = 0;

		// Create a random permutation.
		std::random_shuffle(m_path.begin() + 1, m_path.begin() + cities);
	}

	path_t & operator=(const path_t & other)
	{
		m_path = other.m_path;
		return *this;
	}

	std::size_t cities_count() const noexcept
	{
		return m_path.size();
	}

	auto cost() const noexcept
	{
		std::uint32_t sum = 0;
		for (size_t i = 0; i < m_path.size() - 1; ++i)
		{
			sum += s_costs.get(m_path[i], m_path[i+1], (std::uint16_t)i);
		}
		return sum;
	}

	auto swap_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
	{
		std::int32_t before;
		std::int32_t after;

		auto pim1 = m_path[i - 1];
		auto pi   = m_path[i];
		auto pip1 = m_path[i + 1];

		auto pjm1 = m_path[j - 1];
		auto pj   = m_path[j];
		auto pjp1 = m_path[j + 1];

		if (std::abs(i - j) > 1)
		{
			before = s_costs.get(pim1, pi, i - 1) + s_costs.get(pi, pip1, i)
			       + s_costs.get(pjm1, pj, j - 1) + s_costs.get(pj, pjp1, j);
			after  = s_costs.get(pim1, pj, i - 1) + s_costs.get(pj, pip1, i)
			       + s_costs.get(pjm1, pi, j - 1) + s_costs.get(pi, pjp1, j);
		}
		else if (i + 1 == j)
		{
			before = s_costs.get(pim1, pi, i - 1) + s_costs.get(pi, pj, i) + s_costs.get(pj, pjp1, j);
			after  = s_costs.get(pim1, pj, i - 1) + s_costs.get(pj, pi, i) + s_costs.get(pi, pjp1, j);
		}
		else if (i - 1 == j)
		{
			before = s_costs.get(pjm1, pj, j - 1) + s_costs.get(pj, pi, j) + s_costs.get(pi, pip1, i);
			after  = s_costs.get(pjm1, pi, j - 1) + s_costs.get(pi, pj, j) + s_costs.get(pj, pip1, i);
		}
		else
		{
			before = 0;
			after = 0;
		}

		return after - before;
	}

	auto reverse_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
	{
		auto k = std::min(i, j);
		auto l = std::max(i, j);

		if (l - k > 30)
			return std::numeric_limits<std::int32_t>::max();

		auto before = s_costs.get(m_path[k - 1], m_path[k], k - 1) + s_costs.get(m_path[l], m_path[l + 1], l);
		auto after  = s_costs.get(m_path[k - 1], m_path[l], k - 1) + s_costs.get(m_path[k], m_path[l + 1], l);

		auto end = l - k;
		for (std::uint16_t idx = 0; idx < end; ++idx)
		{
			before += s_costs.get(m_path[k + idx], m_path[k + idx + 1], k + idx);
			after  += s_costs.get(m_path[l - idx], m_path[l - idx - 1], k + idx);
		}

		return after - before;
	}

	auto insert_cost_diff(std::uint16_t i, std::uint16_t j) const noexcept
	{
		std::int32_t before;
		std::int32_t after;

		if (i < j)
		{
			if (j - i > 30)
				return std::numeric_limits<std::int32_t>::max();

			before = s_costs.get(m_path[i - 1], m_path[i], i - 1)
			       + s_costs.get(m_path[j - 1], m_path[j], j - 1)
			       + s_costs.get(m_path[j], m_path[j + 1], j);

			after  = s_costs.get(m_path[i - 1], m_path[i + 1], i - 1)
			       + s_costs.get(m_path[j], m_path[i], j - 1)
			       + s_costs.get(m_path[i], m_path[j + 1], j);

			for (std::uint16_t k = i; k < j-1; ++k)
			{
				before += s_costs.get(m_path[k], m_path[k + 1], k);
				after  += s_costs.get(m_path[k + 1], m_path[k + 2], k);
			}
		}
		else if (j < i)
		{
			if (i - j > 30)
				return std::numeric_limits<std::int32_t>::max();

			before = s_costs.get(m_path[j - 1], m_path[j], j - 1)
			       + s_costs.get(m_path[j], m_path[j + 1], j)
			       + s_costs.get(m_path[i], m_path[i + 1], i);

			after  = s_costs.get(m_path[j - 1], m_path[i], j - 1)
			       + s_costs.get(m_path[i], m_path[j], j)
			       + s_costs.get(m_path[i - 1], m_path[i + 1], i);

			for (std::uint16_t k = j + 1; k < i; ++k)
			{
				before += s_costs.get(m_path[k], m_path[k + 1], k);
				after  += s_costs.get(m_path[k - 1], m_path[k], k);
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

	void print(std::ostream & out, const cities_map & map) const
	{
		// Print the cost.
		out << cost() << std::endl;

		// Print the path.
		for (size_t i = 0; i < m_path.size() - 1; ++i)
		{
			auto src_idx = m_path[i];
			auto dst_idx = m_path[i+1];

			out << map.get_city_object(src_idx);
			out << ' ';
			out << map.get_city_object(dst_idx);
			out << ' ';
			out << i;
			out << ' ';
			out << s_costs.get(src_idx, dst_idx, (std::uint16_t)i);
			out << std::endl;
		}
	}

	static matrix<std::uint16_t> s_costs;

private:
	std::vector<std::uint16_t> m_path;
};
