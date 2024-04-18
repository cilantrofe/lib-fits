/**
 * @file search.hpp
 * @author Alina Gubeeva
 * @brief Structs and functions for case-insensitive search
 * @version 0.1
 * @date 2024-04-10
 *
 * @copyright Copyright (c) 2024
 *
 */

// STL
#include <string>

/**
 * @brief Hash function for case-insensitive strings.
 *
 * This hash function is similar to the one used in the standard library, but
 * performs a case-insensitive comparison. The function is based on
 * Daniel J. Bernstein's hash function.
 */
struct CaseInsensitiveHash
{
    /**
     * @brief Compute the hash value of the given string.
     *
     * This function computes the hash value of the given string by converting
     * each character to lowercase and using the same algorithm as used by the
     * standard library's `std::hash<std::string>` specialization.
     *
     * @param key The string whose hash value is to be computed.
     * @return The hash value of @p key.
     */
    std::size_t operator()(const std::string_view &key) const
    {
        // Similar to std::hash<std::string>, but case-insensitive
        std::size_t hash = 0;
        for (char ch : key)
        {
            // Convert each character to lowercase and combine the hash value
            hash ^= (std::tolower(ch) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
        }
        return hash;
    }
};

/**
 * @brief Compares two strings case-insensitively.
 *
 * This function compares two strings case-insensitively by converting both
 * strings to lowercase and comparing the resulting strings. The comparison
 * is done using the standard library's `std::equal` function, which returns
 * `true` if the two ranges are equal and `false` otherwise.
 */
struct CaseInsensitiveEqual
{
    /**
     * @brief Compares two strings case-insensitively.
     *
     * This function compares two strings case-insensitively by converting both
     * strings to lowercase and comparing the resulting strings. The comparison
     * is done using the standard library's `std::equal` function, which returns
     * `true` if the two ranges are equal and `false` otherwise.
     *
     * @param lhs The first string to compare.
     * @param rhs The second string to compare.
     * @return `true` if @p lhs is equal to @p rhs case-insensitively, and
     *         `false` otherwise.
     */
    bool operator()(const std::string_view &lhs, const std::string_view &rhs) const
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                          [](char a, char b)
                          {
                              // Convert both characters to lowercase and compare
                              return std::tolower(a) == std::tolower(b);
                          });
    }
};
