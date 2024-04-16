// Unit tests for ifits class

#include <gtest/gtest.h>
#include "ifits.hpp"

// Path to the data used in the unit tests
#define DATA_ROOT PROJECT_ROOT "/tests/data"

// Test printing the headers of a FITS file
TEST(test_ifits, print_headers)
{
    // Movie-64 fits file
    std::filesystem::path movie64_filename = DATA_ROOT "/movie-64.fits";

    // Read FITS file
    ifits movie64_fits(movie64_filename);

    std::cout << "Movie-64 fits file" << '\n';

    std::cout << "Headers:\n";
    for (const auto &hdu : movie64_fits.get_hdus())
    {
        std::cout << "---New HDU---" << '\n';
        for (const auto &[key, value] : hdu.get_headers())
        {
            std::cout << key << ": " << value << '\n';
        }
    }

    // Gradient fits file
    std::filesystem::path gradient_filename = DATA_ROOT "/gradient.fits";

    // Read FITS file
    ifits gradient_fits(gradient_filename);

    std::cout << "Gradient fits file" << '\n';

    std::cout << "Headers:\n";
    for (const auto &hdu : gradient_fits.get_hdus())
    {
        std::cout << "---New HDU---" << '\n';
        for (const auto &[key, value] : hdu.get_headers())
        {
            std::cout << key << ": " << value << '\n';
        }
    }
}

// Test throwing an exception when a header keyword is not found
TEST(test_ifits, check_not_existing_header)
{
    // Movie-64 fits file
    std::filesystem::path movie64_filename = DATA_ROOT "/movie-64.fits";

    // Read FITS file
    ifits movie64_fits(movie64_filename);

    // Try to get a non-existing header keyword
    try
    {
        std::string value = movie64_fits.get_hdus().front().value_as<std::string>("NON_EXISTING_KEY");
    }
    catch (const std::out_of_range &e)
    {
        EXPECT_STREQ(e.what(), "Header keyword not found");
    }

    // Gradient fits file
    std::filesystem::path gradient_filename = DATA_ROOT "/gradient.fits";

    // Read FITS file
    ifits gradient_fits(gradient_filename);

    // Try to get a non-existing header keyword
    try
    {
        std::string value = gradient_fits.get_hdus().front().value_as<std::string>("NON_EXISTING_KEY");
    }
    catch (const std::out_of_range &e)
    {
        EXPECT_STREQ(e.what(), "Header keyword not found");
    }
}

// Test getting values from the headers
TEST(test_ifits, check_values)
{
    // Movie-64 fits file
    std::filesystem::path movie64_filename = DATA_ROOT "/movie-64.fits";

    // Read FITS file
    ifits movie64_fits(movie64_filename);

    // Iterate over the HDUs and check if the values are the same as the original ones
    for (const auto &hdu : movie64_fits.get_hdus())
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as<std::string>(key));
        }
    }

    // Gradient fits file
    std::filesystem::path gradient_filename = DATA_ROOT "/gradient.fits";

    // Read FITS file
    ifits gradient_fits(gradient_filename);

    // Iterate over the HDUs and check if the values are the same as the original ones
    for (const auto &hdu : gradient_fits.get_hdus())
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as<std::string>(key));
        }
    }
}

// Test getting values from the headers using optional
TEST(test_ifits, check_value_as_optional)
{
    // Movie-64 fits file
    std::filesystem::path movie64_filename = DATA_ROOT "/movie-64.fits";

    // Read FITS file
    ifits movie64_fits(movie64_filename);

    // Iterate over the HDUs and check if the values are the same as the original ones
    for (const auto &hdu : movie64_fits.get_hdus())
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as_optional<std::string>(key));
        }
    }
}

// Test getting a non-existing header using optional
TEST(test_ifits, check_not_existing_header_optional)
{
    // Movie-64 fits file
    std::filesystem::path movie64_filename = DATA_ROOT "/movie-64.fits";

    // Read FITS file
    ifits movie64_fits(movie64_filename);

    // Try to get a non-existing header using optional
    std::optional<std::string> value = movie64_fits.get_hdus().front().value_as_optional<std::string>("NON_EXISTING_KEY");
    EXPECT_EQ(value, std::nullopt);
}

// Test reading a file with double HDU
TEST(test_ifits, check_double_hdu)
{
    // Double HDU fits file
    std::filesystem::path double_hdu_filename = DATA_ROOT "/double_hdu_read.fits";

    // Read FITS file
    ifits double_hdu_fits(double_hdu_filename);

    // Iterate over the HDUs and check if the values are the same as the original ones
    auto &hdus = double_hdu_fits.get_hdus();
    for (const auto &hdu : hdus)
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as<std::string>(key));
        }
    }
}
