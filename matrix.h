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
    constexpr matrix()
        : m_dim{0}
        , m_matrix{nullptr}
        , m_max_val{std::numeric_limits<T>::min()}
    {
    }

	matrix(unsigned int dim)
	{
        set_dim(dim);
	}

	matrix(const matrix<T> &) = delete;
	matrix<T> & operator=(const matrix<T> &) = delete;

	~matrix()
	{
		delete[] m_matrix;
	}

    void set_dim(unsigned int dim)
    {
        auto length = dim * dim * dim;
        assert(length < std::numeric_limits<decltype(m_dim)>::max());

        m_dim = dim;
        m_matrix = new T[length];
        m_max_val = std::numeric_limits<T>::min();

        std::fill_n(m_matrix, length, std::numeric_limits<T>::max());
    }

	T get_max() const noexcept
	{
		return m_max_val;
	}

	std::int32_t get(std::uint16_t x, std::uint16_t y, std::uint16_t z) const noexcept
	{
        assert(x < m_dim);
        assert(y < m_dim);
        assert(z < m_dim);
		return m_matrix[(x * m_dim * m_dim) + (y * m_dim) + (z)];
	}

	void set(std::uint16_t x, std::uint16_t y, std::uint16_t z, T value) noexcept
	{
        assert(x < m_dim);
        assert(y < m_dim);
        assert(z < m_dim);

		m_matrix[(x * m_dim * m_dim) + (y * m_dim) + (z)] = value;

		if (value > m_max_val)
			m_max_val = value;
	}

private:
	unsigned int m_dim;
	T * m_matrix;
	T   m_max_val;
};
