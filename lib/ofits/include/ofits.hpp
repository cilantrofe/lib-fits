#include "pch.hpp"
#include "details/search.hpp"

#if !defined(BOOST_ASIO_HAS_FILE)
#error "BOOST_ASIO_HAS_FILE not defined"
#endif

// так вызываем ofits<std::uint8_t, float> double_hdu_file{"file.fits", {{200, 300}, {100, 50, 50}}};

template <class... Args>
class ofits
{
public:
    ofits() = default;
    ofits(boost::asio::io_context &io_context, const std::filesystem::path &filename, std::array<std::initializer_list<std::size_t>, sizeof...(Args)> schema)
        : file_(io_context, filename, boost::asio::random_access_file::write_only),
          offset_(0),
          hdus_{std::apply([&](auto &&...args)
                           { return std::make_tuple(hdu<Args>(*this, offset_, args)...); },
                           schema)}
    {
    }

    void update_offset(std::size_t offset)
    {
        offset_ = offset;
    }

    template <std::size_t N, class T>
    void value_as(std::string_view key, T value) const
    {
        std::get<N>(hdus_).template value_as<T>(key, value);
    }

    template <std::size_t N>
    std::size_t write_data(const std::initializer_list<std::size_t>& index, const std::vector<typename std::tuple_element<N, std::tuple<Args...>>::type>& data)
    {
        boost::asio::const_buffer buffer(data.data(), data.size() * sizeof(typename std::tuple_element<N, std::tuple<Args...>>::type));
        return std::get<N>(hdus_).write_data(index, buffer);
    }

    template <std::size_t N, class WriteToken>
    auto async_write_data(const std::initializer_list<std::size_t>& index, const std::vector<typename std::tuple_element<N, std::tuple<Args...>>::type>& data, WriteToken&& token)
    {
        boost::asio::const_buffer buffer(data.data(), data.size() * sizeof(typename std::tuple_element<N, std::tuple<Args...>>::type));
        return std::get<N>(hdus_).async_write_data(index, buffer, std::forward<WriteToken>(token));
    }

    template <std::size_t N>
    auto &get_hdu()
    {
        return std::get<N>(hdus_);
    }

    template <typename T>
    class hdu
    {
        hdu() = default;

        static constexpr auto kSizeHeaderBlock = 2880;

    public:
        hdu(ofits &parent_ofits, std::size_t offset, const std::initializer_list<std::size_t> &hdu_schema)
            : parent_ofits_(parent_ofits), headers_written_(0), header_block_(2880, ' '), offset_(offset)
        {
            write_header("SIMPLE", "T");

            int bitpix = get_bitpix_for_type();
            write_header("BITPIX", std::to_string(bitpix));

            write_header("NAXIS", std::to_string(hdu_schema.size()));

            std::size_t naxis_product = 1;

            std::size_t i = 0;
            for (const auto &size : hdu_schema)
            {
                naxis_product *= size;
                naxis_.push_back(size);
                write_header("NAXIS" + std::to_string(++i), std::to_string(size));
            }

            write_header("EXTEND", "T");

            write_header("END", "");

            data_block_size_ = (((naxis_product * std::abs(bitpix) / 8) + kSizeHeaderBlock - 1) / kSizeHeaderBlock) * kSizeHeaderBlock;

            parent_ofits_.update_offset(round_offset(80 * headers_written_) + data_block_size_);
        }

        template <class U>
        void value_as(const std::string_view &key, const U &value) const
        {
            if (headers_written_ * 80 < header_block_.size())
            {
                std::string header = std::string(key) + " = " + std::string(value);
                header.resize(80, ' ');

                size_t position = headers_written_ * 80 + offset_; // перезаписываю END
                boost::asio::write_at(parent_ofits_.file_, position, boost::asio::buffer(header));

                ++headers_written_;

                // пишу END
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

        template <class ConstBufferSequence>
        std::size_t write_data(const std::initializer_list<std::size_t> index, const ConstBufferSequence &buffers) const
        {
            std::size_t offset = calculate_offset(index);

            std::size_t data_size = boost::asio::buffer_size(buffers);

            if (data_size + offset > data_block_size_) {
                throw std::runtime_error("Not enough space in the HDU");
            }

            return boost::asio::write_at(parent_ofits_.file_, offset_ + kSizeHeaderBlock + offset, buffers);
        }

        template <class ConstBufferSequence, class WriteToken>
        auto async_write_data(const std::initializer_list<std::size_t>& index, const ConstBufferSequence& buffers, WriteToken&& token)
        {
            std::size_t offset = calculate_offset(index);

            std::size_t data_size = boost::asio::buffer_size(buffers);

            if (data_size + offset > data_block_size_) {
                throw std::runtime_error("Not enough space in the HDU");
            }
            
            return boost::asio::async_write_at(parent_ofits_.file_, offset_ + kSizeHeaderBlock + offset, buffers, std::forward<WriteToken>(token));
        }

        std::size_t calculate_offset(const std::initializer_list<std::size_t>& index) const
        {
            std::size_t offset = 0;

            auto it = index.begin();
            auto naxis_it = naxis_.rbegin();

            if (*it > naxis_[0]) {
                throw std::runtime_error("Index is out of bounds");
            }

            if (index.size() > naxis_.size()) {
                throw std::runtime_error("Index size is greater than naxis_ size");
            }

            for (; it != index.end(); ++it, ++naxis_it)
            {
                std::size_t product = *it;

                for (auto naxis_it_j = naxis_it; naxis_it_j != naxis_.rend() - 1; ++naxis_it_j)
                {
                    product *= *naxis_it_j;
                }

                offset += product;
            }

            return offset * sizeof(T);
        }


        // для проверки
        std::size_t get_headers_written() const
        {
            return headers_written_;
        }

    private:
        void write_header(const std::string &key, const std::string &value)
        {
            if (key == "END")
            {
                std::string header = key;
                header.resize(80, ' ');

                size_t position = headers_written_ * 80 + offset_;
                boost::asio::write_at(parent_ofits_.file_, position, boost::asio::buffer(header));

                return;
            }

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

        int get_bitpix_for_type() const
        {
            if constexpr (std::is_same_v<T, std::uint8_t>)
            {
                return 8;
            }
            else if constexpr (std::is_same_v<T, std::int16_t>)
            {
                return 16;
            }
            else if constexpr (std::is_same_v<T, std::int32_t>)
            {
                return 32;
            }
            else if constexpr (std::is_same_v<T, std::int64_t>)
            {
                return 64;
            }
            else if constexpr (std::is_same_v<T, float>)
            {
                return -32;
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                return -64;
            }
            else
            {
                static_assert(sizeof(T) == -1, "Unsupported data type");
            }
        }

        static std::size_t round_offset(const std::size_t &offset)
        {
            return (offset % kSizeHeaderBlock == 0) ? offset : (offset / kSizeHeaderBlock + 1) * kSizeHeaderBlock;
        }

    private:
        ofits &parent_ofits_;
        std::string header_block_;
        mutable std::size_t headers_written_;
        std::size_t offset_;
        std::size_t data_block_size_;
        std::vector<std::size_t> naxis_;
    };

private:
    boost::asio::random_access_file file_;
    std::size_t offset_;
    std::tuple<hdu<Args>...> hdus_;
};
