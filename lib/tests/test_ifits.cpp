#include <gtest/gtest.h>
#include "ifits.hpp"

#define DATA_ROOT PROJECT_ROOT "/tests/data"

TEST(test_ifits, print_headers)
{
    boost::asio::io_context io_context;

    std::filesystem::path filename = DATA_ROOT "/movie-64.fits";

    ifits movie64_fits(io_context, filename);

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

    filename = DATA_ROOT "/gradient.fits";

    ifits gradient_fits(io_context, filename);

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

TEST(test_ifits, check_not_existing_header)
{
    boost::asio::io_context io_context;

    std::filesystem::path filename = DATA_ROOT "/movie-64.fits";

    ifits movie64_fits(io_context, filename);

    try
    {
        std::string value = movie64_fits.get_hdus().front().value_as<std::string>("NON_EXISTING_KEY");
    }
    catch (const std::out_of_range &e)
    {
        EXPECT_STREQ(e.what(), "Not found");
    }

    filename = DATA_ROOT "/gradient.fits";

    ifits gradient_fits(io_context, filename);

    try
    {
        std::string value = gradient_fits.get_hdus().front().value_as<std::string>("NON_EXISTING_KEY");
    }
    catch (const std::out_of_range &e)
    {
        EXPECT_STREQ(e.what(), "Not found");
    }
}

TEST(test_ifits, check_values)
{
    boost::asio::io_context io_context;

    std::filesystem::path filename = DATA_ROOT "/movie-64.fits";

    ifits movie64_fits(io_context, filename);

    for (const auto &hdu : movie64_fits.get_hdus())
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as<std::string>(key));
        }
    }

    filename = DATA_ROOT "/gradient.fits";

    ifits gradient_fits(io_context, filename);

    for (const auto &hdu : gradient_fits.get_hdus())
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as<std::string>(key));
        }
    }
}

TEST(test_ifits, check_value_as_optional)
{
    boost::asio::io_context io_context;

    std::filesystem::path filename = DATA_ROOT "/movie-64.fits";

    ifits movie64_fits(io_context, filename);

    for (const auto &hdu : movie64_fits.get_hdus())
    {
        for (const auto &[key, value] : hdu.get_headers())
        {
            EXPECT_EQ(value, hdu.value_as_optional<std::string>(key));
        }
    }
}

TEST(test_ifits, check_not_existing_header_optional)
{
    boost::asio::io_context io_context;

    std::filesystem::path filename = DATA_ROOT "/movie-64.fits";

    ifits movie64_fits(io_context, filename);

    std::optional<std::string> value = movie64_fits.get_hdus().front().value_as_optional<std::string>("NON_EXISTING_KEY");
    EXPECT_EQ(value, std::nullopt);
}