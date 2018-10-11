/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <random>

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

typedef std::mt19937 rnd_gen_t;
