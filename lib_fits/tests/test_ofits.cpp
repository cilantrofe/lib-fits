#include <gtest/gtest.h>
#include "ofits.hpp"

#define DATA_ROOT PROJECT_ROOT "/tests/data"

TEST(ofits_test, check_single_hdu)
{
    ofits<std::uint8_t> single_hdu_file{DATA_ROOT "/single_hdu.fits", {{{200, 300}}}};

    auto& hdu_0 = single_hdu_file.get_hdu<0>();

    EXPECT_EQ(hdu_0.get_headers_written(), 6);

    single_hdu_file.value_as<0>("XTENSION", "TABLE ");
    
    EXPECT_EQ(hdu_0.get_headers_written(), 7);


}

TEST(ofits_test, check_double_hdu)
{
    ofits<std::uint8_t, float> double_hdu_file{DATA_ROOT "/double_hdu.fits", {{{200, 300}, {100, 50, 50}}}};

    auto& hdu_0 = double_hdu_file.get_hdu<0>();

    auto& hdu_1 = double_hdu_file.get_hdu<1>();

    EXPECT_EQ(hdu_0.get_headers_written(), 6);

    EXPECT_EQ(hdu_1.get_headers_written(), 7);

    double_hdu_file.value_as<0>("DATE-OBS", "1970-01-01");

    EXPECT_EQ(hdu_0.get_headers_written(), 7);

    double_hdu_file.value_as<1>("DATE-OBS", "1991-12-26");

    EXPECT_EQ(hdu_1.get_headers_written(), 8);
}

TEST(ofits_test, check_data)
{
    ofits<std::uint8_t, float> double_hdu_data_file{DATA_ROOT "/double_hdu_data.fits", {{{200, 300}, {100, 50, 50}}}};

    std::vector<std::uint8_t> data_uint8_t = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    double_hdu_data_file.write_data<0>({1, 2}, data_uint8_t);
    
    std::vector<float> data_float = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    double_hdu_data_file.write_data<1>({3, 2, 1}, data_float);

    ofits<std::uint8_t, float, double> three_hdu_file{DATA_ROOT "/three_hdu_data.fits", {{{20, 30}, {10, 5}, {25, 4}}}};
}

