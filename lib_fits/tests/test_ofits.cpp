// Unit tests for ofits class

#include <gtest/gtest.h>
#include "ofits.hpp"

// Path to the data used in the unit tests
#define DATA_ROOT PROJECT_ROOT "/tests/data"

// Test reading a file with a single HDU
TEST(ofits_test, check_single_hdu)
{
    // Single HDU fits file
    ofits<std::uint8_t> single_hdu_file{DATA_ROOT "/single_hdu.fits", {{{200, 300}}}};

    // Get the first HDU
    auto& hdu_0 = single_hdu_file.get_hdu<0>();

    // Check the number of headers written is correct
    EXPECT_EQ(hdu_0.get_headers_written(), 6) << "The number of headers written to the first HDU should be 6";

    // Modify a header keyword value
    single_hdu_file.value_as<0>("XTENSION", "TABLE ");

    // Check the number of headers written is correct
    EXPECT_EQ(hdu_0.get_headers_written(), 7) << "The number of headers written to the first HDU should be 7";
}

// Test reading a file with double HDU
TEST(ofits_test, check_double_hdu)
{
    // Double HDU fits file
    ofits<std::uint8_t, float> double_hdu_file{DATA_ROOT "/double_hdu.fits", {{{200, 300}, {100, 50, 50}}}};

    // Get the first HDU
    auto& hdu_0 = double_hdu_file.get_hdu<0>();

    // Get the second HDU
    auto& hdu_1 = double_hdu_file.get_hdu<1>();

    // Check the number of headers written is correct
    EXPECT_EQ(hdu_0.get_headers_written(), 6) << "The number of headers written to the first HDU should be 6";

    // Check the number of headers written is correct
    EXPECT_EQ(hdu_1.get_headers_written(), 7) << "The number of headers written to the second HDU should be 7";

    // Modify a header keyword value
    double_hdu_file.value_as<0>("DATE-OBS", "1970-01-01");

    // Check the number of headers written is correct
    EXPECT_EQ(hdu_0.get_headers_written(), 7) << "The number of headers written to the first HDU should be 7";

    // Modify a header keyword value
    double_hdu_file.value_as<1>("DATE-OBS", "1991-12-26");

    // Check the number of headers written is correct
    EXPECT_EQ(hdu_1.get_headers_written(), 8) << "The number of headers written to the second HDU should be 8";
}

// Test reading and writing data to a file with double HDU
TEST(ofits_test, check_data)
{
    // Double HDU fits file
    ofits<std::uint8_t, float> double_hdu_data_file{DATA_ROOT "/double_hdu_data.fits", {{{200, 300}, {100, 50, 50}}}};

    // Data for the first HDU
    std::vector<std::uint8_t> data_uint8_t = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

    // Write data to the first HDU
    double_hdu_data_file.write_data<0>({1, 2}, data_uint8_t);

    // Data for the second HDU
    std::vector<float> data_float = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};

    // Write data to the second HDU
    double_hdu_data_file.write_data<1>({3, 2, 1}, data_float);
}

// Test reading a file with three HDU
TEST(ofits_test, check_three_hdu)
{
    // Three HDU fits file
    ofits<std::uint8_t, float, double> three_hdu_file{DATA_ROOT "/three_hdu_data.fits", {{{20, 30}, {10, 5}, {25, 4}}}};
}


