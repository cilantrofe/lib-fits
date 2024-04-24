#include <lib_fits.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <boost/asio.hpp>

int main()
{
    std::vector<float> data(492 * 658, 1676.0f);

    ofits<float> fits{"lib_write.fits", {{{6000, 492, 658}}}};

    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < 6000; ++i)
    {
        fits.async_write_data<0>({i}, boost::asio::buffer(data), [&](const boost::system::error_code &error, std::size_t bytes_transferred)
        {});
    }

    fits.run();

    auto end = std::chrono::steady_clock::now();

    std::cout << "FITS-cpp-library (write): " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

    return 0;
}
