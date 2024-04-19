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
    std::vector<float> data(8000 * 1000 * 100, 1676.0f);

    ofits<float> fits{"my.fits", {{{100, 8000, 1000}}}};

    auto start = std::chrono::steady_clock::now();

    fits.async_write_data<0>({0, 0}, boost::asio::buffer(data), [&](const boost::system::error_code &error, std::size_t bytes_transferred)
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

    fits.run();

    auto end = std::chrono::steady_clock::now();

    std::cout << "FITS-cpp-library: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;


    int status = 0;
    fitsfile *fptr;
    long naxes[3] = {100, 8000, 1000};

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

    long fpixel = 1;
    long nelements = data.size();
    fits_write_img(fptr, TFLOAT, fpixel, nelements, data.data(), &status);
    if (status != 0)
    {
        fits_report_error(stderr, status);
        fits_close_file(fptr, &status);
        return 1;
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
