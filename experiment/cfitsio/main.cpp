#include <fitsio.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <fitsio.h>

int main()
{
    std::vector<float> data(492 * 658, 1676.0f);

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

    fits_close_file(fptr, &status);
    if (status != 0)
    {
        fits_report_error(stderr, status);
        return 1;
    }

    auto end1 = std::chrono::steady_clock::now();

    std::cout << "CFITSIO: " << std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count() << " ms" << std::endl;

    return 0;
}