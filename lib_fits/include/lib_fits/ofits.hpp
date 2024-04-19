/**
 * @file ofits.hpp
 * @author Alian Gubeeva
 * @brief Declaration of ofits class for writing FITS files.
 * @version 0.1
 * @date 2024-04-10
 *
 * @copyright Copyright (c) 2024
 *
 */

// STL
#include <string>
#include <tuple>
#include <optional>
#include <array>
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <filesystem>
#include <numeric>
#include <functional>

// Boost
#include <boost/asio.hpp>
#include <boost/asio/write_at.hpp>

#if !defined(BOOST_ASIO_HAS_FILE)
#error "BOOST_ASIO_HAS_FILE not defined"
#endif

/**
 * @brief Class for writing FITS files.
 *
 * This class allows to write FITS files with arbitrary number of HDUs and
 * their headers.
 *
 * @tparam Args Types of HDUs
 */
template <class... Args>
class ofits
{
public:
    ofits() = default;
    /**
     * @brief Constructor of ofits class.
     *
     * Creates a file for writing and initializes HDUs. The file will be overwritten.
     *
     * @param filename Path to the file to create and write
     * @param schema Schema for HDUs. Each element of the array specifies the size of
     * the corresponding HDU.
     */
    ofits(const std::filesystem::path &filename, std::array<std::initializer_list<std::size_t>, sizeof...(Args)> schema)
        : io_context_(),
          file_(io_context_.get_executor(), filename.string(), boost::asio::random_access_file::write_only | boost::asio::random_access_file::create),
          hdus_{make_hdu_tuple(*this, schema)}
    {
    }

    /**
     * @brief Run the I/O context.
     *
     * This function runs the I/O context until it is stopped. The function is
     * usually called after all HDUs are written to the file.
     */
    void run() noexcept
    {
        // Run the I/O context until it is stopped
        io_context_.run();
    }

    /**
     * @brief Stop the I/O context.
     *
     * This function stops the I/O context. The function is usually called after all
     * HDUs are written to the file and the I/O context is no longer needed.
     */
    void stop() noexcept
    {
        io_context_.stop();
    }

    /**
     * @brief Set value of a header in a given HDU.
     *
     * This function sets the value of a header key in a given HDU.
     *
     * @tparam N Index of the HDU in the tuple of HDUs
     * @tparam T Type of the value to set
     * @param key Key of the header to set
     * @param value Value to set
     */
    template <std::size_t N, class T>
    void value_as(std::string_view key, T value) const
    {
        try
        {
            /*
             * Set value of a header in a given HDU.
             *
             * This function sets the value of a header key in a given HDU.
             *
             * @param key Key of the header to set
             * @param value Value to set
             */
            std::get<N>(hdus_).template value_as<T>(key, value);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error("Error setting value of header " + std::string(key) +
                                     " in HDU " + std::to_string(N) + ": " + e.what());
        }
    }

    /**
     * @brief Write data to a given HDU.
     *
     * This function writes data to a given HDU. The HDU is identified by its index in
     * the tuple of HDUs.
     *
     * @tparam N Index of the HDU in the tuple of HDUs
     * @tparam ConstBufferSequence Type of the buffer sequence. Must meet the
     * requirements of @c SequenceContainer
     *
     * @param index The initial position for writing data
     * @param buffers Buffer sequence containing the data to write
     *
     * @return Number of bytes written
     */
    template <std::size_t N, class ConstBufferSequence>
    std::size_t write_data(const std::initializer_list<std::size_t> &index,
                           const ConstBufferSequence &buffers)
    {
        return std::get<N>(hdus_).write_data(index, buffers);
    }

    /**
     * @brief Asynchronously write data to a given HDU
     *
     * This function writes data to a given HDU asynchronously. The HDU is
     * identified by its index in the tuple of HDUs.
     *
     * @tparam N Index of the HDU in the tuple of HDUs
     * @tparam ConstBufferSequence Type of the buffer sequence. Must meet the
     * requirements of @c SequenceContainer
     * @tparam WriteToken Type of the token. Must meet the requirements of
     * @c AsyncWriteToken
     *
     * @param index The initial position for writing data
     * @param buffers Buffer sequence containing the data to write
     * @param token The token to pass to the completion handler
     *
     * @return A token that is used to retrieve the result of the asynchronous
     * operation
     */
    template <std::size_t N, class ConstBufferSequence, class WriteToken>
    auto async_write_data(const std::initializer_list<std::size_t> &index,
                          const ConstBufferSequence &buffers,
                          WriteToken &&token)
    {
        return std::get<N>(hdus_).async_write_data(index, buffers, std::forward<WriteToken>(token));
    }

    /**
     * @brief Get a reference to an HDU
     *
     * This function returns a reference to an HDU specified by its index in the
     * tuple of HDUs. The HDU is specified by its index in the tuple of HDUs.
     *
     * @tparam N Index of the HDU in the tuple of HDUs
     * @return Reference to the HDU
     */
    template <std::size_t N>
    auto &get_hdu()
    {
        return std::get<N>(hdus_);
    }

    /**
     * @brief Class of HDU object
     * 
     * @tparam T 
     */
    template <typename T>
    class hdu
    {
        hdu() = default;

        /**
         * @brief Size of the header block
         *
         * The header block is the part of the HDU that contains the keyword=value pairs.
         * It is 2880 bytes long.
         */
        static constexpr auto kSizeHeaderBlock = 2880;

    public:
        /**
         * @brief Construct a new HDU object
         *
         * Construct a new HDU object. The HDU is constructed by writing the necessary
         * header keywords to the parent OFITS object's file. The HDU is given an
         * offset in the file, which is the beginning of the HDU.
         *
         * @param parent_ofits Parent OFITS object
         * @param offset Offset of the HDU in the file
         * @param hdu_schema Schema of the HDU. Contains the size of each dimension of the HDU
         */
        hdu(ofits &parent_ofits, std::size_t offset, const std::initializer_list<std::size_t> &hdu_schema) noexcept
            : parent_ofits_(parent_ofits), headers_written_(0), header_block_(2880, ' '), offset_(offset)
        {
            write_header("SIMPLE", "T"); // Value is "T" because the HDU is simple

            // Calculate the number of bytes per pixel based on the type
            int bitpix = get_bitpix_for_type();

            write_header("BITPIX", std::to_string(bitpix));

            write_header("NAXIS", std::to_string(hdu_schema.size()));

            // Calculate the product of the sizes of all axes
            std::size_t naxis_product = 1;

            std::size_t i = 0;
            for (const auto &size : hdu_schema)
            {
                naxis_product *= size;

                naxis_.push_back(size);

                write_header("NAXIS" + std::to_string(++i), std::to_string(size));
            }

            // Calculate the size of the data block of the HDU
            data_block_size_ = naxis_product * std::abs(bitpix) / 8;

            write_header("EXTEND", "T"); // Value is "T" because the HDU is extended

            write_header("END", ""); // Value is empty
        }

        /**
         * @brief Write a value to the HDU's header
         *
         * This function writes the value to the PDU header in the specified key instead of the END.
         *
         * @tparam U Type of the value
         * @param key Key of the header keyword
         * @param value Value to be written
         */
        template <class U>
        void value_as(const std::string_view &key, const U &value) const
        {
            if (headers_written_ * 80 < header_block_.size())
            {
                std::string header = std::string(key) + " = " + std::string(value);
                header.resize(80, ' ');

                // Calculate the position of the new header
                size_t position = headers_written_ * 80 + offset_;
                boost::asio::write_at(parent_ofits_.file_, position, boost::asio::buffer(header));

                ++headers_written_;

                // Rewrite END
                header = "END";
                header.resize(80, ' ');

                position = headers_written_ * 80 + offset_;
                boost::asio::write_at(parent_ofits_.file_, position, boost::asio::buffer(header));
            }
            else
            {
                throw std::runtime_error("Not enough space in the HDU");
            }
        }

        /**
         * @brief Write data to the HDU
         *
         * This function writes the given data to the HDU at the given index. The
         * index is a sequence of integers, where each integer specifies the
         * index of the element in the corresponding dimension of the HDU.
         *
         * @tparam ConstBufferSequence Type of the buffer sequence
         * @param index Index of the element to write to
         * @param buffers Buffer sequence to write
         * @return Number of bytes written
         */
        template <class ConstBufferSequence>
        std::size_t write_data(const std::initializer_list<std::size_t> index, const ConstBufferSequence &buffers) const
        {
            // Calculate the offset by index
            std::size_t offset = calculate_offset(index);

            std::size_t data_size = boost::asio::buffer_size(buffers);

            // Check if there is enough space in the HDU data block
            if (data_size + offset > data_block_size_)
            {
                throw std::runtime_error("Not enough space in the HDU");
            }

            return boost::asio::write_at(parent_ofits_.file_, offset_ + kSizeHeaderBlock /*headers written*/ + offset, buffers);
        }

        /**
         * @brief Asynchronously write data to the HDU
         *
         * This function writes the given data to the HDU at the given index.
         * The index is a sequence of integers, where each integer specifies
         * the index of the element in the corresponding dimension of the HDU.
         * The function is asynchronous and returns immediately.
         *
         * @tparam ConstBufferSequence Type of the buffer sequence
         * @tparam WriteToken The type of the token
         * @param index Index of the element to write to
         * @param buffers Buffer sequence to write
         * @param token The token to pass to the completion handler
         * @return A token that is used to retrieve the result of the asynchronous operation
         */
        template <class ConstBufferSequence, class WriteToken>
        auto async_write_data(const std::initializer_list<std::size_t> &index, const ConstBufferSequence &buffers, WriteToken &&token)
        {
            // Calculate offset by index
            std::size_t offset = calculate_offset(index);

            std::size_t data_size = boost::asio::buffer_size(buffers);

            // Check if there is enough space in the HDU data block
            if (data_size + offset > data_block_size_)
            {
                throw std::runtime_error("Not enough space in the HDU");
            }

            return boost::asio::async_write_at(parent_ofits_.file_, offset_ + kSizeHeaderBlock /*headers written*/ + offset, buffers, std::forward<WriteToken>(token));
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

            auto it = index.begin();

            auto naxis_it = naxis_.rbegin();

            // Check if the first index is out of bounds
            if (*it > naxis_[0])
            {
                throw std::runtime_error("Index is out of bounds");
            }

            // Check if the size of the index list is greater than the
            // size of the naxis vector
            if (index.size() > naxis_.size())
            {
                throw std::runtime_error("Index size is greater than NAXIS size");
            }

            // Calculate the offset using the indices
            for (; it != index.end(); ++it, ++naxis_it)
            {
                std::size_t product = *it;

                // Calculate the product of the indices for all dimensions
                // except the first
                for (auto naxis_it_j = naxis_it; naxis_it_j != naxis_.rend() - 1; ++naxis_it_j)
                {
                    product *= *naxis_it_j;
                }

                // Add the product to the offset
                offset += product;
            }

            // Return the offset multiplied by the size of the element
            return offset * sizeof(T);
        }

        /**
         * @brief Get the headers written object
         * 
         * @return std::size_t 
         */
        std::size_t get_headers_written() const
        {
            return headers_written_;
        }

    private:
        /**
         * @brief Write a header keyword to the HDU
         *
         * Writes the header keyword and value to the HDU. If the keyword
         * is "END", then it is written as is to the end of the HDU.
         *
         * @param key Keyword of the header
         * @param value Value of the header keyword
         */
        void write_header(const std::string &key, const std::string &value)
        {
            if (key == "END")
            {
                // Write END to the HDU

                std::string header = key;
                header.resize(80, ' ');

                size_t position = headers_written_ * 80 + offset_;
                boost::asio::write_at(parent_ofits_.file_, position, boost::asio::buffer(header));

                return;
            }

            // Write a header keyword to the HDU

            std::string header = key + " = " + value;
            header.resize(80, ' ');

            if (headers_written_ * 80 < header_block_.size())
            {
                size_t position = headers_written_ * 80 + offset_;
                boost::asio::write_at(parent_ofits_.file_, position, boost::asio::buffer(header));

                ++headers_written_;
            }
            else
            {
                throw std::runtime_error("Not enough space in the HDU");
            }
        }

        /**
         * @brief Get the BITPIX value for the data type of the HDU
         *
         * This function returns the BITPIX value for the data type of the HDU. The
         * BITPIX value is used to specify the data type of the HDU in the FITS
         * header.
         *
         * @return BITPIX value for the data type of the HDU
         */
        int get_bitpix_for_type() const
        {
            if constexpr (std::is_same_v<T, std::uint8_t>)
            {
                return 8; // 8-bit unsigned integer
            }
            else if constexpr (std::is_same_v<T, std::int16_t>)
            {
                return 16; // 16-bit integer
            }
            else if constexpr (std::is_same_v<T, std::int32_t>)
            {
                return 32; // 32-bit integer
            }
            else if constexpr (std::is_same_v<T, std::int64_t>)
            {
                return 64; // 64-bit integer
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                return -32; // 32-bit floating-point number
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                return -64; // 64-bit floating-point number
            }
            else
            {
                static_assert(sizeof(T) == -1, "Unsupported data type");
            }
        }

    private:
        ofits &parent_ofits_;                 // Parent OFITS object
        std::string header_block_;            // Header block of the HDU
        mutable std::size_t headers_written_; // Number of headers written to the HDU
        std::size_t offset_;                  // Offset of the HDU in the file
        std::size_t data_block_size_;         // Size of the data block in the HDU
        std::vector<std::size_t> naxis_;      // Number of elements in each dimension of the HDU
    };

private:
    /**
     * @brief Size of the header block
     *
     * The header block is the part of the HDU that contains the keyword=value pairs.
     * It is 2880 bytes long.
     */
    static constexpr auto kSizeHeaderBlock = 2880;

    /**
     * @brief Round up the offset to the nearest multiple of the size of the header block.
     *
     * This function rounds up the given offset to the nearest multiple of the
     * size of the header block (2880 bytes). The result is the smallest
     * multiple of the header block that is greater than or equal to the
     * given offset.
     *
     * @param offset The offset to be rounded
     * @return The rounded offset
     */
    static std::size_t round_offset(const std::size_t &offset) noexcept
    {
        return (offset % kSizeHeaderBlock == 0) ? offset : (offset / kSizeHeaderBlock + 1) * kSizeHeaderBlock;
    }

    /**
     * @brief Create a tuple of HDUs based on a schema
     *
     * This function creates a tuple of HDUs based on a schema that specifies the
     * number of elements in each dimension. The schema is an array of initializer
     * lists, where each initializer list specifies the number of elements in the
     * corresponding dimension of the HDU. The function returns a tuple of HDU
     * objects, where each HDU object represents one HDU in the file.
     *
     * @param parent_ofits Parent OFITS object
     * @param schema Schema specifying the number of elements in each dimension
     * @return Tuple of HDU objects
     */
    static std::tuple<hdu<Args>...> make_hdu_tuple(ofits &parent_ofits, std::array<std::initializer_list<std::size_t>, sizeof...(Args)> schema) noexcept
    {
        // Calculate the offsets of the HDUs in the file
        std::array<size_t, sizeof...(Args)> offset;
        size_t current_offset = 0;

        std::array<std::size_t, sizeof...(Args)> sizes = {sizeof(Args)...};

        for (size_t i = 0; i < sizeof...(Args); ++i)
        {
            offset[i] = current_offset;

            current_offset += kSizeHeaderBlock + round_offset(std::accumulate(schema[i].begin(), schema[i].end(), sizes[i], std::multiplies<std::size_t>()));
        }

        return make_hdu_tuple_impl(parent_ofits, schema, offset, std::make_index_sequence<sizeof...(Args)>{});
    }

    /**
     * @brief Create a tuple of HDUs from a schema
     *
     * This function creates a tuple of HDUs based on a schema that specifies the
     * number of elements in each dimension. The schema is an array of initializer
     * lists, where each initializer list specifies the number of elements in the
     * corresponding dimension of the HDU. The function returns a tuple of HDU
     * objects, where each HDU object represents one HDU in the file.
     *
     * @tparam Is Indices of the HDUs in the tuple
     * @param parent_ofits Parent OFITS object
     * @param schema Schema of the HDUs
     * @param offset Offsets of the HDUs in the file
     * @param is_ Indices sequence
     * @return Tuple of HDU objects
     */
    template <size_t... Is>
    static std::tuple<hdu<Args>...> make_hdu_tuple_impl(ofits &parent_ofits, std::array<std::initializer_list<std::size_t>, sizeof...(Args)> &schema, std::array<size_t, sizeof...(Args)> &offset, std::index_sequence<Is...> is_) noexcept
    {
        // Create tuple of HDUs from the schema
        return std::make_tuple(hdu<Args>(parent_ofits, offset[Is], schema[Is])...);
    }

private:
    boost::asio::io_context io_context_;   // IO context to use for asynchronous operations
    boost::asio::random_access_file file_; // File to write to
    std::tuple<hdu<Args>...> hdus_;        // HDUs of the file
};
