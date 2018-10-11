/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <cstdint>
#include <limits>
#include <random>

 
class xorshift128plus
{
public:
    typedef std::uint64_t result_type;
 
    xorshift128plus(std::uint32_t seed)
    {
        m_state[0] = (seed + 249863319);
        m_state[1] = (seed + 249863319);
    }
 
    xorshift128plus(xorshift128plus &&) noexcept = default;
    xorshift128plus & operator=(xorshift128plus &&) noexcept = default;
 
    xorshift128plus(xorshift128plus &) = default;
    xorshift128plus & operator=(xorshift128plus &) = default;
 
    result_type operator()() noexcept
    {
 	    std::uint64_t x = m_state[0];
 	    std::uint64_t const y = m_state[1];
 	    m_state[0] = y;
 	    x ^= x << 23; // a
 	    m_state[1] = x ^ y ^ (x >> 17) ^ (y >> 26); // b, c
 	    return m_state[1] + y;
    }
 
    static result_type min() noexcept
    {
 	    return 0;
    }
 
    static result_type max() noexcept
    {
 	    return std::numeric_limits<result_type>::max();
    }
 
   private:
    std::uint64_t m_state[2];
};


class xoroshiro128plus
{
public:
    typedef std::uint64_t result_type;

    xoroshiro128plus(std::uint32_t seed)
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

    static result_type min() noexcept
    {
        return 0;
    }

    static result_type max() noexcept
    {
        return std::numeric_limits<result_type>::max();
    }

private:
    static inline std::uint64_t rotl(std::uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    std::uint64_t m_state[2];
};

typedef std::mt19937     rnd_gen_t;
//typedef xorshift128plus  rnd_gen_t;
//typedef xoroshiro128plus rnd_gen_t;
