#include <lib_fits.hpp>
#include <iostream>
//#include <boost/asio.hpp>

int main() {
    /*
    / Writing
    */

    // Creating example.fits with 2 HDU's of size 20*30 and 10*6*5 in examples/build
    ofits<int16_t, float> example_write{"example.fits",  {{{20, 30}, {10, 6, 5}}}};

    // Async-write data to example.fits
    std::vector<float> data = {0.1f, 2.0f, 3.0f, 4.0f, 5.0f};

    example_write.async_write_data<1>({0, 0}, data, /* your WriteToken */ [](const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (!error) {
            std::cout << "Data written successfully!" << std::endl;
        } else {
            std::cerr << "Error writing data: " << error.message() << std::endl;
        } 
    });

    // Run context
    example_write.run();
    
    // Add header
    example_write.value_as<0>("EXAMPLE", "2024-04-13");

    /*
    / Reading
    */

    // Open example.fits
    ifits example_read("example.fits");

    // Get value from header EXAMPLE
    auto hdu_0 = example_read.get_hdu<0>().value_as<std::string>("EXAMPLE");

    std::cout << hdu_0 << std::endl;

    return 0;
}