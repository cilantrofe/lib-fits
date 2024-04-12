#include <lib_fits.hpp>
#include <iostream>

int main() {
    // Creating example.fits with 2 HDU's of size 20*30 and 10*6*5 in examples/build
    ofits<int16_t, float> example_file{"example.fits",  {{{20, 30}, {10, 6, 5}}}};

    // Async-write data to example.fits
    std::vector<float> data = {0.1f, 2.0f, 3.0f, 4.0f, 5.0f};
    auto token = boost::asio::use_future;
    example_file.async_write_data<1>({1, 2}, data, token);
    
}