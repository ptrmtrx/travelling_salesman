#pragma once

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>


namespace io
{
        class LineReader
        {
        public:
            LineReader() = delete;
            LineReader(const LineReader &) = delete;
            LineReader & operator=(const LineReader&) = delete;

            LineReader(FILE *)
            {
                init();
            }

            char * next_line()
            {
                if (data_begin == data_end)
                    return nullptr;

                if (data_begin >= block_len)
                {
                    std::memcpy(buffer.get(), buffer.get() + block_len, block_len);
                    data_begin -= block_len;
                    data_end -= block_len;

                    data_end += reader.finish_read();
                    std::memcpy(buffer.get() + block_len, buffer.get() + 2 * block_len, block_len);
                    reader.start_read(buffer.get() + 2 * block_len, block_len);
                }

                int line_end = data_begin;
                while (buffer[line_end] != '\n' && line_end != data_end)
                {
                    ++line_end;
                }

                if (buffer[line_end] == '\n' && line_end != data_end)
                {
                    buffer[line_end] = '\0';
                }
                else
                {
                    // some files are missing the newline at the end of the last line
                    ++data_end;
                    buffer[line_end] = '\0';
                }

                // handle windows \r\n-line breaks
                if (line_end != data_begin && buffer[line_end - 1] == '\r')
                    buffer[line_end - 1] = '\0';

                char * ret = buffer.get() + data_begin;
                data_begin = line_end + 1;
                return ret;
            }

        private:
            static constexpr int block_len = 1 << 24;
            std::unique_ptr<char[]> buffer; // must be constructed before (and thus destructed after) the reader!

            SynchronousReader reader;

            int data_begin;
            int data_end;

            void init()
            {
                buffer = std::make_unique<char[]>(3 * block_len);
                data_begin = 0;
                data_end = byte_source->read(buffer.get(), 2 * block_len);

                if (data_end == 2 * block_len)
                {
                    reader.init(std::move(byte_source));
                    reader.start_read(buffer.get() + 2 * block_len, block_len);
                }
            }
        };

        ////////////////////////////////////////////////////////////////////////////
        //                                 CSV                                    //
        ////////////////////////////////////////////////////////////////////////////

        template<char ... trim_char_list>
        struct trim_chars
        {
        private:
            constexpr static bool is_trim_char(char)
            {
                return false;
            }

            template<class ... OtherTrimChars>
            constexpr static bool is_trim_char(char c, char trim_char, OtherTrimChars...other_trim_chars)
            {
                return c == trim_char || is_trim_char(c, other_trim_chars...);
            }

        public:
            static void trim(char *& str_begin, char *& str_end)
            {
                while (str_begin != str_end && is_trim_char(*str_begin, trim_char_list...))
                    ++str_begin;
                while (str_begin != str_end && is_trim_char(*(str_end - 1), trim_char_list...))
                    --str_end;
                *str_end = '\0';
            }
        };

        template<char ... comment_start_char_list>
        struct single_line_comment
        {
        private:
                constexpr static bool is_comment_start_char(char)
                {
                        return false;
                }
       
                template<class ...OtherCommentStartChars>
                constexpr static bool is_comment_start_char(char c, char comment_start_char, OtherCommentStartChars...other_comment_start_chars)
                {
                        return c == comment_start_char || is_comment_start_char(c, other_comment_start_chars...);
                }

        public:

                static bool is_comment(const char*line)
                {
                        return is_comment_start_char(*line, comment_start_char_list...);
                }
        };

        namespace detail
        {
            static const char * find_next_column_end(const char * col_begin, char sep)
            {
                while (*col_begin != sep && *col_begin != '\0')
                    ++col_begin;
                return col_begin;
            }

            void chop_next_column(char *& line, char *& col_begin, char *& col_end)
            {
                col_begin = line;
                // the col_begin + (... - col_begin) removes the constness
                col_end = col_begin + find_next_column_end(col_begin, ' ') - col_begin);

                if (*col_end == '\0')
                {
                    line = nullptr;
                }
                else
                {
                    *col_end = '\0';
                    line = col_end + 1;
                }
            }

                template<class trim_policy>
                void parse_line(char * line, char ** sorted_col, const std::vector<int> & col_order)
                {
                    for (std::size_t i = 0; i < col_order.size(); ++i)
                    {
                        char*col_begin, *col_end;
                        chop_next_column(line, col_begin, col_end);

                        if (col_order[i] != -1)
                        {
                            trim_policy::trim(col_begin, col_end);
                            sorted_col[col_order[i]] = col_begin;
                        }
                    }
                }

                void parse(char*col, char &x)
                {
                    x = *col;
                    ++col;
                }
               
                void parse(char*col, const char*&x)
                {
                    x = col;
                }

                void parse(char*col, char*&x)
                {
                    x = col;
                }

                template<class T>
                void parse_unsigned_integer(const char*col, T&x)
                {
                    x = 0;
                    while (*col != '\0')
                    {
                        {
                            T y = *col - '0';
                            x = 10 * x + y;
                        }
                        ++col;
                    }
                }

                void parse(char*col, unsigned char &x)
                {
                    parse_unsigned_integer(col, x);
                }
                void parse(char*col, unsigned short &x)
                {
                    parse_unsigned_integer(col, x);
                }
               
                template<class T>
                void parse_signed_integer(const char*col, T&x)
                {
                    parse_unsigned_integer(col, x);
                }      

                void parse(char*col, signed char &x)
                {
                    parse_signed_integer(col, x);
                }
        }

        template<unsigned column_count, class trim_policy = trim_chars<' ', '\t'>>
        class CSVReader
        {
        private:
            LineReader in;

            char*row[column_count];
            std::string column_names[column_count];

            std::vector<int> col_order;

        public:
            CSVReader() = delete;
            CSVReader(const CSVReader&) = delete;
            CSVReader&operator=(const CSVReader&) = delete;

            template<class ...Args>
            explicit CSVReader(Args&&...args) : in(std::forward<Args>(args)...)
            {
                std::fill(row, row + column_count, nullptr);
                col_order.resize(column_count);
                for (unsigned i = 0; i < column_count; ++i)
                    col_order[i] = i;
                for (unsigned i = 1; i <= column_count; ++i)
                    column_names[i - 1] = "col" + std::to_string(i);
            }

            template<class ...ColType>
            bool read_row(ColType& ...cols)
            {
                auto line = in.next_line();
                if (!line)
                    return false;

                detail::parse_line<trim_policy>(line, row, col_order);
                parse_helper(0, cols...);

                return true;
            }

        private:
            void parse_helper(std::size_t) { }

            template<class T, class ...ColType>
            void parse_helper(std::size_t r, T&t, ColType&...cols)
            {
                if (row[r])
                {
                    ::io::detail::parse(row[r], t);
                }
                parse_helper(r + 1, cols...);
            }
        };
}
