/**
 * @file ifits.hpp
 * @brief
 *
 */

#include "pch.hpp"
#include "details/search.hpp"

#if !defined(BOOST_ASIO_HAS_FILE)
#error "BOOST_ASIO_HAS_FILE not defined"
#endif

class ifits
{
public:
    using header_container_t = std::unordered_multimap<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual>;

    ifits() = default;

    ifits(const ifits &) = delete;
    ifits(ifits &&) = delete;

    ifits &operator=(const ifits &) = delete;
    ifits &operator=(ifits &) = delete;

    class hdu
    {
    private:
        static constexpr auto kSizeHeaderBlock = 2880;

    public:
        hdu(ifits &parent_ifits, header_container_t headers)
            : parent_ifits_(parent_ifits),
              headers_(headers)
        {
        }

        hdu(ifits &parent_ifits)
            : parent_ifits_(parent_ifits)
        {
        }

        const header_container_t &get_headers() const noexcept
        {
            return headers_;
        }

        std::size_t calculate_next_HDU_offset() const
        {
            // Рассчитывает смещение следующего HDU
            const std::size_t data_block_size = (((get_NAXIS_product() * std::abs(get_BITPIX()) / 8) + kSizeHeaderBlock - 1) / kSizeHeaderBlock) * kSizeHeaderBlock;
            const std::size_t offset = get_header_block_count() + data_block_size;

            return offset;
        }

        int get_NAXIS() const
        {
            auto it = headers_.find("NAXIS");
            if (it == headers_.end())
            {
                throw std::out_of_range("NAXIS not found");
            }
            return std::stoi(it->second);
        }

        std::size_t calculate_offset(const std::initializer_list<std::size_t> &index) const
        {
            std::size_t offset = 0;

            int naxis_size = (*this).get_NAXIS();

            if (index.size() > naxis_size)
            {
                throw std::runtime_error("Index size is greater than NAXIS size");
            }

            auto it = index.begin();
            int i = index.size();
            for (; it != index.end(); ++it, --i)
            {
                std::size_t product = *it;

                for (auto j = i; j != 1; --j)
                {
                    auto naxis_j = headers_.find("NAXIS" + std::to_string(j));
                    product *= std::stoi(naxis_j->second);
                }

                offset += product;
            }

            return offset;
        }

    private:
        static std::size_t round_offset(const std::size_t &offset)
        {
            return (offset % kSizeHeaderBlock == 0) ? offset : (offset / kSizeHeaderBlock + 1) * kSizeHeaderBlock;
        }

        std::size_t get_NAXIS_product() const
        {
            auto it = headers_.find("NAXIS");
            if (it == headers_.end())
            {
                throw std::out_of_range("NAXIS not found");
            }

            int product = 1;
            int NAXIS = std::stoi(it->second);
            for (int i = 1; i <= NAXIS; i++)
            {
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

        int get_BITPIX() const
        {
            auto it = headers_.find("BITPIX");
            if (it == headers_.end())
            {
                throw std::out_of_range("BITPIX not found");
            }
            return std::stoi(it->second);
        }

        std::size_t get_header_block_count() const
        {
            return (headers_.size() + kSizeHeaderBlock - 1) / kSizeHeaderBlock;
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
        template <class T>
        T value_as(std::string_view key) const
        {
            auto it = headers_.find(std::string(key));
            if (it == headers_.end())
            {
                throw std::out_of_range("Not found");
            }

            std::istringstream iss(it->second);
            T value;
            if (!(iss >> value))
            {
                throw std::runtime_error("Failed to convert value");
            }

            return value;
        }

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
                    throw std::runtime_error("Failed to convert value");
                }
                return value;
            }
            return std::nullopt;
        }

        /* паттерн visitor */
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

        std::pair<hdu, std::uint64_t> extract_next_HDU(std::uint64_t offset)
        {
            char buffer[81];
            while (true)
            {
                boost::asio::read_at(parent_ifits_.file_, offset, boost::asio::buffer(buffer, 80));
                buffer[80] = '\0';

                std::string key = std::string(buffer, 8);
                boost::algorithm::trim(key);

                std::string value = std::string(buffer + 8, 30);
                if (value.find("/") != std::string::npos)
                {
                    value = std::string(buffer + 8, value.find("/"));
                }

                boost::algorithm::erase_all(value, " ");
                boost::algorithm::erase_all(value, "=");

                if (key == "END")
                {
                    break;
                }

                headers_.emplace(key, value);

                offset += 80;
            }

            offset_ = offset;

            return std::make_pair(hdu(*this), round_offset(offset));
        }

        template <class T>
        class image_hdu
        {
        public:
            image_hdu(ifits &parent_hdu)
                : parent_hdu_(parent_hdu)
            {
            }

            template <std::size_t N, class MutableBufferSequence, class ReadToken>
            auto async_read_data(const std::initializer_list<std::size_t> &index, const MutableBufferSequence &buffers, ReadToken &&token)
            {
                std::size_t offset = sizeof(T) * parent_hdu_.calculate_offset(index);

                return boost::asio::async_read_at(parent_hdu_.parent_ifits_.file_, parent_hdu_.offset_ + offset, buffers, std::forward<ReadToken>(token));
            }

            template <class MutableBufferSequence>
            std::size_t read_at(std::initializer_list<std::size_t> index, const MutableBufferSequence &buffers)
            {
                std::size_t offset = sizeof(T) * parent_hdu_.calculate_offset(index);

                return boost::asio::read_at(parent_hdu_.parent_ifits_.file_, parent_hdu_.offset_ + offset, buffers);
            }

        private:
            hdu &parent_hdu_;
        };

    private:
        ifits &parent_ifits_;
        header_container_t headers_;
        std::uint64_t offset_;
    };

    explicit ifits(boost::asio::io_context &io_context, const std::filesystem::path &filename)
        : file_(io_context, filename, boost::asio::random_access_file::read_only)
    {
        std::uint64_t next_hdu_offset = 0;

        while (true)
        {
            auto res = hdu(*this).extract_next_HDU(next_hdu_offset);

            auto new_hdu = res.first;

            hdus_.push_back(new_hdu);

            next_hdu_offset = res.second;
            next_hdu_offset += new_hdu.calculate_next_HDU_offset();

            if (file_.size() <= next_hdu_offset)
            {
                break;
            }
        }
    }

public:
    hdu get_hdu_at(std::size_t index) const {
        auto it = std::next(hdus_.begin(), index);
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
    boost::asio::random_access_file file_;
    std::list<hdu> hdus_;
};