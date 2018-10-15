/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>


template <typename T>
class matrix
{
public:
	matrix(unsigned int dim)
		: m_dim(dim)
		, m_length(dim * dim * dim)
		, m_matrix(new T[m_length])
		, m_max(std::numeric_limits<T>::min())
	{
        assert(dim * dim * dim < std::numeric_limits<decltype(m_dim)>::max());
		std::fill_n(m_matrix, m_length, std::numeric_limits<T>::max());
	}

	matrix(const matrix<T> &) = delete;
	matrix<T> & operator=(const matrix<T> &) = delete;

	~matrix()
	{
		delete[] m_matrix;
	}

	T get_max() const noexcept
	{
		return m_max;
	}

	std::int32_t get(std::uint16_t x, std::uint16_t y, std::uint16_t z) const noexcept
	{
		return m_matrix[(x * m_dim * m_dim) + (y * m_dim) + (z)];
	}

	void set(std::uint16_t x, std::uint16_t y, std::uint16_t z, T value) noexcept
	{
		m_matrix[(x * m_dim * m_dim) + (y * m_dim) + (z)] = value;

		if (value > m_max)
			m_max = value;
	}

private:
	unsigned int m_dim;
	unsigned int m_length;
	T * m_matrix;
	T   m_max;
};
