#pragma once

#include <string>

namespace
{
    template <typename char_type>
    static inline auto is_space(char_type ch) noexcept
    {
        return ch == ' ';
    }

    template <typename char_type>
    static inline auto is_digit(char_type ch) noexcept
    {
        return '0' <= ch && ch <= '9';
    }

    template <typename char_type>
    static inline auto is_lower(char_type ch) noexcept
    {
        return 'a' <= ch && ch <= 'z';
    }

    template <typename char_type>
    static inline char_type to_upper(char_type ch) noexcept
    {
        return is_lower(ch) ? ch - 'a' + 'A' : ch;
    }

    template <typename char_type>
    static inline int chrcmp(char_type lch, char_type rch) noexcept
    {
        if (lch < rch)
            return -1;
        if (lch > rch)
            return +1;
        return 0;
    }

    template <typename char_type>
    static inline int chrcasecmp(char_type lch, char_type rch) noexcept
    {
        return chrcmp(to_upper(lch), to_upper(rch));
    }

    template <
        typename iterator_type,
        int (*value_cmp)(typename iterator_type::value_type, typename iterator_type::value_type)
    >
    static int iterator_natcmp(iterator_type lbegin, iterator_type lend, iterator_type rbegin, iterator_type rend)
    {
        for (auto leading = true; ; ++lbegin, ++rbegin) {
            if (lbegin >= lend && rbegin >= rend)
                return 0;
            if (lbegin >= lend)
                return -1;
            if (rbegin >= rend)
                return +1;

            while (leading && *lbegin == '0' && (lbegin + 1 < lend) && is_digit(*(lbegin + 1)))
                ++lbegin;
            while (leading && *rbegin == '0' && (rbegin + 1 < rend) && is_digit(*(rbegin + 1)))
                ++rbegin;
            leading = false;

            while (lbegin < lend && is_space(*lbegin))
                ++lbegin;
            while (rbegin < rend && is_space(*rbegin))
                ++rbegin;

            if (lbegin >= lend && rbegin >= rend)
                return 0;
            if (lbegin >= lend)
                return -1;
            if (rbegin >= rend)
                return +1;

            if (is_digit(*lbegin) && is_digit(*rbegin)) {
                if (*lbegin == '0' || *rbegin == '0') {
                    for (;; ++lbegin, ++rbegin) {
                        if ((lbegin >= lend || !is_digit(*lbegin)) &&
                            (rbegin >= rend || !is_digit(*rbegin)))
                            break;
                        if (lbegin >= lend || !is_digit(*lbegin))
                            return -1;
                        if (rbegin >= rend || !is_digit(*rbegin))
                            return +1;
                        if (auto diff = chrcmp(*lbegin, *rbegin))
                            return diff;
                    }
                } else {
                    int bias = 0;
                    for (;; ++lbegin, ++rbegin) {
                        if ((lbegin >= lend || !is_digit(*lbegin)) &&
                            (rbegin >= rend || !is_digit(*rbegin)))
                            break;
                        if (lbegin >= lend || !is_digit(*lbegin))
                            return -1;
                        if (rbegin >= rend || !is_digit(*rbegin))
                            return +1;
                        if (auto diff = chrcmp(*lbegin, *rbegin)) {
                            if (bias == 0)
                                bias = diff;
                        }
                    }
                    if (bias)
                        return bias;
                }
                if (lbegin >= lend && rbegin >= rend)
                    return 0;
                if (lbegin >= lend)
                    return -1;
                if (rbegin >= rend)
                    return +1;
            }

            if (auto diff = value_cmp(*lbegin, *rbegin))
                return diff;
        }
    }
}

template <typename iterator_type>
inline int strnatcmp(iterator_type lbegin, iterator_type lend, iterator_type rbegin, iterator_type rend) noexcept
{
    return iterator_natcmp<iterator_type, chrcmp>(lbegin, lend, rbegin, rend);
}

template <typename iterator_type>
inline int strnatcasecmp(iterator_type lbegin, iterator_type lend, iterator_type rbegin, iterator_type rend) noexcept
{
    return iterator_natcmp<iterator_type, chrcasecmp>(lbegin, lend, rbegin, rend);
}

template <typename char_type>
inline int strnatcmp(const std::basic_string<char_type> &lhs, const std::basic_string<char_type> &rhs) noexcept
{
    return strnapcmp(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
}

template <typename char_type>
inline int strnatcasecmp(const std::basic_string<char_type> &lhs, const std::basic_string<char_type> &rhs) noexcept
{
    return strnatcasecmp(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
}
