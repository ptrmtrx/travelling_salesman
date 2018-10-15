/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <string>
#include <vector>

#include "city.h"

class area_t
{
public:
    area_t(const char * name)
        : m_name(name)
    {
    }

    void add_city(std::uint16_t city_idx)
    {
        m_cities.push_back(city_idx);
    }

    void add_cities(std::vector<std::uint16_t> && cities_idx)
    {
        m_cities = std::move(cities_idx);
    }

    const std::string & get_name() const { return m_name; }

private:
    std::string m_name;
    std::vector<std::uint16_t> m_cities;
};

class areas_map_t
{
public:
    areas_map_t() = default;

    void reserve(std::uint16_t count)
    {
    }
};
