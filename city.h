/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>


class city_t
{
public:
	explicit constexpr city_t(const char * str)
        : m_code{str[0], str[1], str[2]}
	{
	}

	constexpr bool operator<(const city_t & other) const
	{
		return m_code[0] < other.m_code[0]
			|| m_code[1] < other.m_code[1]
			|| m_code[2] < other.m_code[2];
	}

	constexpr bool operator==(const city_t & other) const
	{
		return m_code[0] == other.m_code[0]
			&& m_code[1] == other.m_code[1]
			&& m_code[2] == other.m_code[2];

		//return std::equal(m_code, m_code + sizeof(m_code), other.m_code);
	}

	constexpr std::size_t hash() const
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
	std::size_t operator()(const city_t & c) const
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

//class cities_map_t
//{
//public:
//    cities_map_t()
//    {
//        m_map.reserve(300);
//    }
//
//    std::uint16_t get_city_index(const city_t & city)
//    {
//        auto it = std::find_if(m_map.cbegin(), m_map.cend(), [&](auto & c) { return c.first == city; });
//        if (it != m_map.end())
//            return it->second;
//
//        m_map.emplace_back(city, m_last_idx);
//        return m_last_idx++;
//    }
//
//    city_t get_city_object(std::uint16_t idx) const
//    {
//        for (const auto & pair : m_map)
//        {
//            if (pair.second == idx)
//                return pair.first;
//        }
//
//        throw std::runtime_error("get_city_object");
//    }
//
//    std::size_t count() const
//	{
//		return m_map.size();
//	}
//
//private:
//    std::vector<std::pair<city_t, std::uint16_t>> m_map;
//    std::uint16_t m_last_idx = 0;
//};
