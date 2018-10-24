/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <cassert>
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

    void set_selected_city(std::uint16_t pos)
    {
        assert(pos < m_cities.size());
        m_selected_city_pos = pos;
    }

private:
    unsigned int m_selected_city_pos;
    std::vector<std::uint16_t> m_cities;
    std::string m_name;
};
