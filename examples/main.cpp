#include <lib_fits.hpp>
#include <iostream>
#include <boost/asio.hpp>
#include <memory>

int main()
{
    /*
    / Writing
    */

    // Creating example.fits with 2 HDU's of size 20*30 and 10*6*5 in examples/build
    ofits<int16_t, float> example_write{"example.fits", {{{20, 30}, {10, 6, 5}}}};
    std::vector<int16_t> data_1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Write data to first HDU
    // {1, 2} - index --> offset for data = (1 × NAXIS2 + 2) × sizeof(int16_t) = 32 × 2 = 64 relative to current HDU in data block
    example_write.async_write_data<0>({1, 2}, boost::asio::buffer(data_1), [](const boost::system::error_code &error, std::size_t bytes_transferred)
                                      {
        if (!error) {
            std::cout << "Data written successfully!" << std::endl;
        } else {
            std::cerr << "Error writing data: " << error.message() << std::endl;
        } });

    // Run context
    example_write.run();

    // Add header to second HDU
    example_write.value_as<1>("EXAMPLE", "2024-04-13");

    /*
    / Reading
    */

    // Open example.fits
    ifits example_read("example.fits");

    // Get value from header EXAMPLE
    auto hdu_1 = example_read.get_hdu<1>().value_as<std::string>("EXAMPLE");

    std::cout << hdu_1 << std::endl;

    auto hdu_0 = example_read.get_hdu<0>();

    // Apply a function to the current HDU, based on its BITPIX value
    // Async read data from first HDU
    // Pattern Visitor
    hdu_0.apply([](auto x)
                {
        // Using shared_ptr to manage buffer memory
        auto buffer = std::make_shared<std::vector<int16_t>>(10);
        x.async_read_data({1, 2}, boost::asio::buffer(*buffer), [buffer](const boost::system::error_code &error, std::size_t bytes_transferred)
                          {
            if (!error) {
                std::cout << "Data read successfully!" << std::endl;
                std::cout << "Bytes read: " << bytes_transferred << std::endl;
                for (std::size_t i = 0; i < 10; ++i) {
                    std::cout << (*buffer)[i] << " ";
                }
                std::cout << std::endl;
            } else {
                std::cerr << "Error reading data: " << error.message() << std::endl;
            }
        }); });

    // Run the IO context for asynchronous operations
    example_read.run();

    return 0;
}