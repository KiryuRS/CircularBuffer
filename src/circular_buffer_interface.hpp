#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace ds {

namespace detail {

inline consteval std::size_t round_nearest_exponent(std::size_t n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if INTPTR_MAX == UINT64_MAX
    n |= n >> 32;
#endif
    ++n;
    return n;
}

} // namespace detail

template <typename T, std::size_t Size>
class circular_buffer_base
{
protected:
    static constexpr std::size_t ExponentSize = detail::round_nearest_exponent(Size);
    using container_type = std::array<T, ExponentSize>;

    template <typename U>
    struct iterator_impl
    {
        using value_type = U;
        using difference_type = std::ptrdiff_t;
        using pointer = U*;
        using reference = U&;

        iterator_impl(container_type& container, uint64_t index)
            : span_{container}
            , index_{index}
        {
        }

        U& operator*() const noexcept
        {
            return span_[index_];
        }

        iterator_impl& operator++() noexcept
        {
            ++index_;
            return *this;
        }

        iterator_impl operator++(int) noexcept
        {
            iterator_impl tmp{*this};
            ++index_;
            return tmp;
        }

        iterator_impl operator+(uint64_t offset) const noexcept
        {
            iterator_impl tmp{*this};
            tmp.index_ += offset;
            return tmp;
        }

        iterator_impl& operator+=(uint64_t offset) noexcept
        {
            index_ += offset;
            return *this;
        }

        iterator_impl& operator--() noexcept
        {
            --index_;
            return *this;
        }

        iterator_impl operator--(int) noexcept
        {
            iterator_impl tmp{*this};
            --index_;
            return tmp;
        }

        iterator_impl operator-(uint64_t offset) const noexcept
        {
            iterator_impl tmp{*this};
            tmp.index_ -= offset;
            return tmp;
        }

        iterator_impl& operator-=(uint64_t offset) noexcept
        {
            index_ -= offset;
            return *this;
        }

        constexpr bool operator<=>(const iterator_impl&) const noexcept = default;

    private:
        std::span<U, ExponentSize> span_;
        uint64_t index_;
    };

protected:
    constexpr uint64_t wrapped_index(uint64_t index) const noexcept
    {
        return index & (ExponentSize - 1);
    }

    constexpr uint64_t empty(uint64_t start_index, uint64_t end_index) const noexcept
    {
        return start_index == end_index;
    }

    constexpr uint64_t size(uint64_t start_index, uint64_t end_index) const noexcept
    {
        return end_index - start_index;
    }

protected:
    container_type buffer_;

// traits
public:
    using value_type = T;
    using reference_type = value_type&;
    using const_reference = const value_type&;
    using size_type = std::size_t;
    using iterator = iterator_impl<value_type>;
    using const_iterator = iterator_impl<const value_type>;

    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return ExponentSize;;
    }

    // should not be used in production, unit-testing API only
    const container_type& get_buffer() const noexcept
    {
        return buffer_;
    }
};

} // namespace ds
