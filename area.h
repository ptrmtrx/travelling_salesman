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
    area_t(std::string && name, std::vector<std::uint16_t> && cities_idx)
        : m_selected_city_pos{0}
        , m_cities{std::move(cities_idx)}
        , m_name{std::move(name)}
    {
    }

    const std::string & get_name()    const noexcept { return m_name; }
    std::uint16_t get_selected_city() const noexcept { return m_cities[m_selected_city_pos]; }

private:
    unsigned int m_selected_city_pos;
    std::vector<std::uint16_t> m_cities;
    std::string m_name;
};

class areas_map_t
{
public:
    areas_map_t() = default;

    void reserve(std::uint16_t count)
    {
        (void)count;
    }

    // Returns index of added area.
    std::uint16_t add_area(area_t && area)
    {
        (void)area;
        return 0;
    }

    std::size_t count() const
    {
        return 0;//m_map.size();
    }
private:

};
