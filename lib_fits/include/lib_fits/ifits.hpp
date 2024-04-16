/**
 * @file ifits.hpp
 * @author Alina Gubeeva
 * @brief Declaration of ifits class for reading FITS files.
 * @version 0.1
 * @date 2024-04-10
 *
 * @copyright Copyright (c) 2024
 *
 */

// STL
#include <string>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <sstream>
#include <string_view>
#include <map>
#include <list>
#include <filesystem>

// Boost
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/erase.hpp>

#include "details/search.hpp" // CaseInsensitiveHash, CaseInsensitiveEqual

// Check if BOOST_ASIO_HAS_FILE is defined
#if !defined(BOOST_ASIO_HAS_FILE)
#error "BOOST_ASIO_HAS_FILE not defined"
#endif

/**
 * @brief Class for reading FITS files.
 *
 */
class ifits
{
public:
    ifits() = default;

    ifits(const ifits &) = delete;
    ifits(ifits &&) = delete;

    ifits &operator=(const ifits &) = delete;
    ifits &operator=(ifits &) = delete;

    /**
     * @brief Constructor
     *
     * This constructor opens the FITS file at the given path and extracts the headers and data from the individual HDUs.
     *
     * @param filename The path to the FITS file
     */
    explicit ifits(const std::filesystem::path &filename)
        : io_context_(),
          file_(io_context_, filename, boost::asio::random_access_file::read_only)
    {
        std::uint64_t next_hdu_offset = 0; // The offset of the next HDU

        try
        {
            // Loop until we reach the end of the file
            while (true)
            {
                // Extract the next HDU and its offset
                auto res = hdu(*this).extract_next_HDU(next_hdu_offset);

                auto new_hdu = res.first; // The extracted HDU

                hdus_.push_back(new_hdu); // Add the HDU to the list of HDUs

                next_hdu_offset = res.second;                           // Get the offset of the next HDU
                next_hdu_offset += new_hdu.calculate_data_block_size(); // Increment the offset to the next HDU

                if (file_.size() <= next_hdu_offset) // If we've reached the end of the file
                {
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error while reading FITS file: " + filename.string() + " - " + e.what());
        }
    }

    /**
     * @brief Run the io_context.
     *
     * This function runs the io_context, which is necessary to read any data from the file asynchronously.
     * The function blocks until the io_context is stopped.
     */
    void run() noexcept
    {
        // Run the io_context to allow asynchronous reads from the file
        io_context_.run();
    }

    /**
     * @brief Stop the io_context.
     *
     * This function stops the io_context. The function is usually called after
     * all HDUs are read from the file and the io_context is no longer needed.
     */
    void stop() noexcept
    {
        // Stop the io_context
        io_context_.stop();
    }

    /**
     * @brief Class for reading one HDU (header data unit) from a FITS file.
     *
     * Each FITS file contains multiple HDUs. HDUs are separated by a special
     * block of zeros in the file. The class reads one HDU from the file and
     * provides access to its headers.
     *
     */
    class hdu
    {
        /**
         * @brief Container for FITS header. Contains multiple values for each keyword. Case-resistant.
         *
         */
        using header_container_t = std::unordered_multimap<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;

        /**
         * @brief Size of a FITS header block in bytes.
         *
         * According to the FITS standard, the header block size is always 2880 bytes.
         */
        static constexpr std::size_t kSizeHeaderBlock = 2880;

    public:
        /**
         * @brief Construct a new HDU object
         *
         * Construct a new HDU object by reading one HDU from the parent IFITS
         * object. The HDU is read from the current position in the file.
         *
         * @param parent_ifits Parent IFITS object
         */
        hdu(ifits &parent_ifits) noexcept
            : parent_ifits_(parent_ifits)
        {
        }

        /**
         * @brief Calculate the offset in the HDU data block
         *
         * Calculates the offset in the HDU data block based on the indices
         * specified in the initializer list. The indices specify the index
         * of the element in the corresponding dimension of the HDU.
         *
         * @param index List of indices specifying the location in the HDU
         * @return Offset in the HDU data block in bytes
         */
        std::size_t calculate_offset(const std::initializer_list<std::size_t> &index) const
        {
            std::size_t offset = 0;

            int naxis_size = (*this).get_NAXIS(); // Get the number of axes

            if (index.size() > naxis_size) // Check if the size of the index is valid
            {
                throw std::runtime_error("Index size is greater than NAXIS size");
            }

            auto it = index.begin();
            int i = naxis_size;
            for (; it != index.end(); ++it, --i)
            {
                std::size_t product = *it; // Get the value of the current element

                for (auto j = i; j != 1; --j) // Iterate over the axes backwards from the current one
                {
                    auto naxis_j = headers_.find("NAXIS" + std::to_string(j)); // Find the header keyword for the current axis
                    if (naxis_j == headers_.end())
                    {
                        throw std::out_of_range("NAXIS" + std::to_string(j) + " not found");
                    }
                    product *= std::stoi(naxis_j->second); // Multiply the product by the size of the current axis
                }

                offset += product;
            }

            return offset;
        }

        /**
         * @brief Extract the next HDU from FITS file
         *
         * This function reads the header of the next HDU from the FITS file, starting
         * at the given offset. The function will stop reading when it finds the "END"
         * keyword in the header. The function returns a pair containing the extracted
         * HDU and the offset of the next HDU.
         *
         * @param offset The offset into the FITS file from which to start reading
         * @return A pair containing the extracted HDU and the offset of the next HDU
         */
        std::pair<hdu, std::uint64_t> extract_next_HDU(std::uint64_t offset) noexcept
        {
            char buffer[81]; // Buffer to read header into

            // Read the header until we find the "END" keyword
            while (true)
            {
                boost::asio::read_at(parent_ifits_.file_, offset, boost::asio::buffer(buffer, 80));
                buffer[80] = '\0'; // Null-terminate the buffer

                std::string key = std::string(buffer, 8); // Extract the 8-character key from the buffer
                boost::algorithm::trim(key);              // Remove leading and trailing whitespace

                std::string value = std::string(buffer + 8, 30); // Extract the 30-character value from the buffer
                std::size_t found = value.find("/");
                if (found != std::string::npos) // If "/" is present, extract the value up to that point, because further comment
                {
                    value = std::string(buffer + 8, found);
                }

                boost::algorithm::erase_all(key, " "); // Remove whitespace
                boost::algorithm::erase_all(key, "="); // Remove "="

                boost::algorithm::erase_all(value, " "); // Remove whitespace
                boost::algorithm::erase_all(value, "="); // Remove "="

                if (key == "END") // If we found the "END" keyword, stop
                {
                    break;
                }

                headers_.emplace(key, value); // Insert the key-value pair into the header container

                offset += 80; // Increment the offset to the next 80-byte block
            }

            offset_ = round_offset(offset); // Set the current HDU's offset

            return std::make_pair(hdu(*this), round_offset(offset_));
        }

        /**
         * @brief Calculates the size of the data block of the HDU
         *
         * The calculation is based on the product of the sizes of all axes
         * multiplied by the number of bytes per pixel, plus the size of the
         * header block (2880 bytes). The result is rounded up to the next
         * multiple of 2880 bytes.
         *
         * @return Size of the data block of the HDU, in bytes
         */
        std::size_t calculate_data_block_size() const noexcept
        {
            const std::size_t data_block_size = round_offset((get_NAXIS_product() * std::abs(get_BITPIX()) / 8));

            return data_block_size;
        }

        /**
         * @brief Get the number of axes of the current HDU
         *
         * This function retrieves the value of the "NAXIS" header keyword.
         *
         * @return Number of axes of the current HDU
         */
        int get_NAXIS() const
        {
            auto it = headers_.find("NAXIS");
            if (it == headers_.end())
            {
                throw std::out_of_range("NAXIS not found");
            }
            return std::stoi(it->second); // Convert the string to int
        }

    private:
        /**
         * @brief Round up the offset to the nearest multiple of the size of the header block.
         *
         * @param offset The offset to be rounded
         * @return The rounded offset
         */
        static std::size_t round_offset(const std::size_t &offset) noexcept
        {
            return (offset % kSizeHeaderBlock == 0) ? offset : (offset / kSizeHeaderBlock + 1) * kSizeHeaderBlock;
        }

        /**
         * @brief Calculates the product of sizes of all axes of the current HDU
         *
         * The calculation is based on the values of keywords "NAXIS" and "NAXISi"
         * for all i from 1 to the value of "NAXIS".
         *
         * @return The product of sizes of all axes of the current HDU
         */
        std::size_t get_NAXIS_product() const
        {
            // Get the number of axes
            auto it = headers_.find("NAXIS");
            if (it == headers_.end())
            {
                throw std::out_of_range("NAXIS not found");
            }

            int NAXIS = std::stoi(it->second);

            // Calculate the product of sizes of all axes
            int product = 1;
            for (int i = 1; i <= NAXIS; i++)
            {
                // Get the size of the axis
                std::string key = "NAXIS" + std::to_string(i);
                auto it = headers_.find(key);
                if (it == headers_.end())
                {
                    throw std::out_of_range(key + " not found");
                }
                product *= std::stoi(it->second);
            }

            return product;
        }

        /**
         * @brief Get the number of bits per pixel for the current HDU
         *
         * This function retrieves the value of the "BITPIX" header keyword.
         * The value is returned as an integer.
         *
         * @return Number of bits per pixel for the current HDU
         */
        int get_BITPIX() const
        {
            auto it = headers_.find("BITPIX");
            if (it == headers_.end())
            {
                throw std::out_of_range("BITPIX not found");
            }

            return std::stoi(it->second); // Convert the string to int
        }

        header_container_t::const_iterator cbegin() const
        {
            return headers_.cbegin();
        }

        header_container_t::const_iterator begin() const
        {
            return headers_.begin();
        }

        header_container_t::const_iterator cend() const
        {
            return headers_.cend();
        }

        header_container_t::const_iterator end() const
        {
            return headers_.end();
        }

    public:
        const header_container_t &get_headers() const noexcept
        {
            return headers_;
        }

        /**
         * @brief Get the value of a header keyword
         *
         * This function retrieves the value of the specified header keyword
         * from the current HDU. The value is returned as the type T,
         * which must be convertible from std::string.
         *
         * @tparam T The type of the return value
         * @param key The name of the header keyword
         * @return The value of the header keyword
         */
        template <class T>
        T value_as(std::string_view key) const
        {
            auto it = headers_.find(std::string(key));
            if (it == headers_.end())
            {
                throw std::out_of_range("Header keyword not found");
            }

            std::istringstream iss(it->second);
            T value;
            if (!(iss >> value))
            {
                throw std::runtime_error("Failed to convert value of " +
                                         std::string(key));
            }

            return value;
        }

        /**
         * @brief Get the value of a header keyword (optional)
         *
         * This function retrieves the value of the specified header keyword
         * from the current HDU. If the header keyword is not found,
         * std::nullopt is returned. Otherwise, the value is returned as the type
         * T, which must be convertible from std::string.
         *
         * @tparam T The type of the return value
         * @param key The name of the header keyword
         * @return The value of the header keyword, or std::nullopt if not found
         */
        template <class T>
        std::optional<T> value_as_optional(std::string_view key) const
        {
            auto it = headers_.find(std::string(key));
            if (it != headers_.end())
            {
                std::istringstream iss(it->second);
                T value;
                if (!(iss >> value))
                {
                    throw std::runtime_error("Failed to convert value of " +
                                             std::string(key));
                }
                return value;
            }
            return std::nullopt;
        }

        /**
         * @brief Apply a function to the current HDU, based on its BITPIX value
         *
         * This function applies the function @p f to the current HDU, using the
         * appropriate specialization of image_hdu. The return value of @p f is
         * returned by this function.
         *
         * @tparam Functor The type of the function to apply
         * @param f The function to apply
         * @return The return value of @p f
         */
        template <typename Functor>
        auto apply(Functor f)
        {
            switch (get_BITPIX())
            {
            case 8:
                return f(image_hdu<std::uint8_t>(*this));
            case 16:
                return f(image_hdu<std::int16_t>(*this));
            case 32:
                return f(image_hdu<std::int32_t>(*this));
            case 64:
                return f(image_hdu<std::int64_t>(*this));
            case -32:
                return f(image_hdu<float>(*this));
            case -64:
                return f(image_hdu<double>(*this));
            default:
                throw std::runtime_error("Unsupported BITPIX value");
            }
        }

        /**
         * @brief Image HDU class
         *
         * This class represents an image HDU in a FITS file. The template parameter T specifies the type of the image
         * data. The class provides functions to read and write image data at specific indices.
         */
        template <class T>
        class image_hdu
        {
        public:
            /**
             * @brief Constructor
             *
             * @param parent_hdu The parent HDU
             */
            image_hdu(hdu &parent_hdu)
                : parent_hdu_(parent_hdu)
            {
            }

            /**
             * @brief Asynchronously read image data at a specific index
             *
             * This function asynchronously reads image data from a specific index.
             * The function returns a `boost::asio::async_result<ReadToken, std::size_t>`
             * object representing the result of the asynchronous operation.
             *
             * @param index The initial position for reading data
             * @param buffers A sequence of buffers into which the data will be read
             * @param token A token for the asynchronous operation
             * @return A `boost::asio::async_result<ReadToken, std::size_t>` object representing the result of the asynchronous operation
             */
            template <class MutableBufferSequence, class ReadToken>
            auto async_read_data(const std::initializer_list<std::size_t> &index,
                                 const MutableBufferSequence &buffers, ReadToken &&token)
            {
                std::size_t offset = sizeof(T) * parent_hdu_.calculate_offset(index);

                if (offset > parent_hdu_.calculate_data_block_size() + parent_hdu_.offset_)
                {
                    throw std::runtime_error("Index is out of bounds");
                }

                return boost::asio::async_read_at(parent_hdu_.parent_ifits_.file_, // Read from the file
                                                  parent_hdu_.offset_ + offset,    // Starting from the offset
                                                  buffers,                         // Into these buffers
                                                  std::forward<ReadToken>(token)); // With this token
            }

            /**
             * @brief Synchronously read image data at a specific index
             *
             * This function synchronously reads image data from a specific index.
             * The function returns the number of bytes read.
             *
             * @param index The initial position for reading data
             * @param buffers A sequence of buffers into which the data will be read
             * @return The number of bytes read
             */
            template <class MutableBufferSequence>
            std::size_t read_data(std::initializer_list<std::size_t> index,
                                  const MutableBufferSequence &buffers)
            {
                std::size_t offset = sizeof(T) * parent_hdu_.calculate_offset(index);

                if (offset > parent_hdu_.calculate_data_block_size() + parent_hdu_.offset_)
                {
                    throw std::runtime_error("Index is out of bounds");
                }

                return boost::asio::read_at(parent_hdu_.parent_ifits_.file_, // Read from the file
                                            parent_hdu_.offset_ + offset,    // Starting from the offset
                                            buffers);                        // Into these buffers
            }

        private:
            hdu &parent_hdu_; // The parent HDU
        };

    private:
        ifits &parent_ifits_;        // The parent IFITS object
        header_container_t headers_; // The HDU headers
        std::uint64_t offset_;       // The current HDU's offset
    };

public:
    /**
     * @brief Get the hdu object
     *
     * @tparam N
     * @return auto&
     */
    template <std::size_t N>
    auto &get_hdu()
    {
        if (N >= hdus_.size())
        {
            throw std::out_of_range("Index out of bounds");
        }

        auto it = std::next(hdus_.begin(), N);

        return *it;
    }

    std::list<hdu>::const_iterator cbegin() const
    {
        return hdus_.cbegin();
    }

    std::list<hdu>::const_iterator begin() const
    {
        return hdus_.begin();
    }

    std::list<hdu>::const_iterator cend() const
    {
        return hdus_.cend();
    }

    std::list<hdu>::const_iterator end() const
    {
        return hdus_.end();
    }

    const std::list<hdu> &get_hdus() const noexcept
    {
        return hdus_;
    }

private:
    boost::asio::io_context io_context_;   // IO context to use for asynchronous operations
    boost::asio::random_access_file file_; // The FITS file
    std::list<hdu> hdus_;                  // The list of HDUs
};