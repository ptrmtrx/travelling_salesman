/**
 * @author Petr Lavicka
 * @copyright
 * @file
 */

#pragma once
#include <cstdio>

class parser_t
{
public:
    parser_t() = default;

    // Returns whole line, nullptr on EOF.
    char * read_line()
    {
        return next_line();
    }

    void parse_line(std::uint16_t & num, char *& str)
    {
        auto line = next_line();
        read_uint16(line, num);
        read_str(line, str);
    }

    bool parse_line(char *& from, char *& to, std::uint16_t & day, std::uint16_t & price)
    {
        auto line = next_line();
        if (!line)
            return false;

        read_str(line, from);
        read_str(line, to);
        read_uint16(line, day);
        read_uint16(line, price);
        return true;
    }

private:
    static void read_str(char *& line, char *& str)
    {
        str = line;

        int i = 0;
        char c = *line++;
        while (c != ' ' && c != '\n')
        {
            ++i;
            c = *line++;
        }
        str[i] = '\0';
    }

    static void read_uint16(char *& line, std::uint16_t & num)
    {
        num = 0;

        char c = *line++;
        while (c != ' ' && c != '\n')
        {
            num = 10 * num + (c - '0');
            c = *line++;
        }
    }

    // Returns whole line from stdin, nullptr on EOF.
    char * next_line()
    {
        return std::fgets(m_buffer, sizeof(m_buffer), stdin);
    }

    char m_buffer[2048];
};
