/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once

#include <cstdio>

class stdin_reader_t
{
public:
    stdin_reader_t()
    {
        // Disable buffering and replace it by own one.
        std::setvbuf(stdin, 0, _IONBF, 0);
    }

    std::size_t read(char * buffer, int size)
    {
        return std::fread(buffer, 1, size, stdin);
    }
};

#define READ_CHAR _fgetc_nolock(stdin)

static std::uint16_t read_uint16()
{
    std::uint16_t num = 0;

    auto c = READ_CHAR;
    while (c >= '0' && c <= '9')
    {
        num = 10 * num + (c - '0');
        c = READ_CHAR;
    }

    return num;
}

static void read_str(char * str)
{
    int i = 0;
    auto c = READ_CHAR;
    while (!std::isspace(c))
    {
        str[i++] = c;
        c = READ_CHAR;
    }
    str[i] = 0;
}

class SynchronousReader
{
public:
    void start_read(char * arg_buffer, int arg_desired_byte_count)
    {
        buffer = arg_buffer;
        desired_byte_count = arg_desired_byte_count;
    }

    int finish_read()
    {
        return std::fread(buffer, 1, desired_byte_count, stdin);
    }
private:
    char * buffer;
    int desired_byte_count;
};


class line_reader_t
{
public:
    line_reader_t() = default;

private:

};


class parser_t
{
public:
    parser_t() = default;

    char * read_line()
    {
        return m_input.next_line();
    }

    template <class ... Columns>
    bool parse_row(Columns& ... cols)
    {
        auto line = m_input.next_line();
        if (!line)
            return false;

        detail::parse_line<trim_policy>(line, row, col_order);
        parse_helper(0, cols...);

        return true;
    }

private:
    line_reader_t m_input;
};