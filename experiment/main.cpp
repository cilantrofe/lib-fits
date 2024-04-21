/**
 * @file main.cpp
 * @author Alina Gubeeva
 * @brief Speed comparison of FITS-cpp-library and CFITSIO
 * @version 0.1
 * @date 2024-04-19
 *
 * @copyright Copyright (c) 2024
 *
 */

#include <lib_fits.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <fitsio.h>
#include <boost/asio.hpp>

int main()
{
    std::vector<float> data(492 * 658, 1676.0f);

    ofits<float> fits{"my.fits", {{{6000, 492, 658}}}};

    auto start = std::chrono::steady_clock::now();

    for (size_t i = 0; i < 6000; ++i)
    {
        fits.async_write_data<0>({i}, boost::asio::buffer(data), [&](const boost::system::error_code &error, std::size_t bytes_transferred)
                                                  {
                                    if (!error)
                                    {
                                        std::cout << "Data written successfully!" << std::endl;
                                        fits.stop();

                                    }
                                    else
                                    {
                                        std::cerr << "Error writing data: " << error.message() << std::endl;
                                    } });
    }

    fits.run();

    auto end = std::chrono::steady_clock::now();

    std::cout << "FITS-cpp-library: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

    int status = 0;
    fitsfile *fptr;
    long naxes[3] = {6000, 492, 658};

    fits_create_file(&fptr, "cfitsio.fits", &status);
    if (status != 0)
    {
        fits_report_error(stderr, status);
        return 1;
    }

    auto start1 = std::chrono::steady_clock::now();

    fits_create_img(fptr, FLOAT_IMG, 3, naxes, &status);
    if (status != 0)
    {
        fits_report_error(stderr, status);
        fits_close_file(fptr, &status);
        return 1;
    }

    for (size_t i = 0; i < 6000; ++i) {
        fits_write_img(fptr, TFLOAT, 1 + i * 492 * 658, 492 * 658, data.data(), &status);
    }

    auto end1 = std::chrono::steady_clock::now();

    fits_close_file(fptr, &status);
    if (status != 0)
    {
        fits_report_error(stderr, status);
        return 1;
    }

    std::cout << "CFITSIO: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << " ms" << std::endl;

    return 0;
}

/**
 * For 7.24 GB data
 * 
 *
 * 1) Output: 
 * FITS-cpp-library: 85537 ms
 * CFITSIO: 133741 ms
 * 
 * 2) Output: 
 * FITS-cpp-library: 73586 ms
 * CFITSIO: 136252 ms
 * 
 * 3) Output:
 * FITS-cpp-library:  84256 ms
 * CFITSIO: 138383 ms
 * 
 * 4) Output:
 * FITS-cpp-library: 85573 ms
 * CFITSIO: 141459 ms
 * 
 * Average:
 * FITS-cpp-library:  82238 ms
 * СFITSIO: 137459 ms
 * 
 * Conclusion: FITS-cpp-library is 1,7 times faster
 */
