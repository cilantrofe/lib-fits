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

    double fits_total_time = 0.0;
    double cfitsio_total_time = 0.0;
    int num_iterations = 10; // Количество итераций

    for (int iteration = 0; iteration < num_iterations; ++iteration)
    {
        // FITS-cpp-library
        {
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
            fits_total_time += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }

        // CFITSIO
        {
            int status = 0;
            fitsfile *fptr;
            long naxes[3] = {6000, 492, 658};

            fits_create_file(&fptr, "cfitsio.fits", &status);
            auto start = std::chrono::steady_clock::now();

            fits_create_img(fptr, FLOAT_IMG, 3, naxes, &status);

            for (size_t i = 0; i < 6000; ++i)
            {
                fits_write_img(fptr, TFLOAT, 1 + i * 492 * 658, 492 * 658, data.data(), &status);
            }

            fits_close_file(fptr, &status);

            auto end = std::chrono::steady_clock::now();
            cfitsio_total_time += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        }
    }

    // Вычисление среднего времени выполнения
    double fits_average_time = fits_total_time / num_iterations;
    double cfitsio_average_time = cfitsio_total_time / num_iterations;

    // Вывод результатов
    std::cout << "Average time for FITS-cpp-library: " << fits_average_time << " ms" << std::endl;
    std::cout << "Average time for CFITSIO: " << cfitsio_average_time << " ms" << std::endl;

    return 0;
}
