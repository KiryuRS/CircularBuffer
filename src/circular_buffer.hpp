#pragma once

#include <array>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <new>
#include <ostream>
#include <span>
#include <utility>

namespace ds {

namespace detail {

consteval std::size_t round_nearest_exponent(std::size_t n)
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

template <typename T>
concept always_false = false;

} // namespace detail

// overflow version - FIFO behavior
template <typename T, std::size_t Size>
class circular_buffer
{
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

// traits
public:
    using value_type = T;
    using reference_type = value_type&;
    using const_reference = const value_type&;
    using size_type = std::size_t;
    using iterator = iterator_impl<value_type>;
    using const_iterator = iterator_impl<const value_type>;

public:
    constexpr circular_buffer() noexcept = default;

// metadata APIs
public:
    [[nodiscard]] constexpr size_type capacity() const noexcept
    {
        return ExponentSize;;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return start_index_ == end_index_;
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
        return end_index_ - start_index_;
    }

// modifier APIs
public:
    template <typename ... Args>
        requires std::constructible_from<T, std::remove_cvref_t<Args>...>
    constexpr void emplace(Args&& ... args)
    {
        auto ptr = buffer_.data() + (wrapped_index(end_index_++));
        new (ptr) value_type{std::forward<Args>(args)...};
    }

    template <typename U, typename RawU = std::remove_cvref_t<U>>
        requires std::constructible_from<T, U> ||
                 std::same_as<T, RawU>
    constexpr void push(U&& value)
    {
        buffer_[wrapped_index(end_index_++)] = std::forward<U>(value);
    }

    bool pop(T& value) // pops from the start_index_
    {
        if (empty())
            return false;

        if constexpr (std::is_move_constructible_v<T>)
            value = std::move(buffer_[wrapped_index(start_index_++)]);
        else if constexpr (std::is_copy_assignable_v<T>)
            value = buffer_[wrapped_index(start_index_++)];
        else
            static_assert(detail::always_false<T>, "Not circular buffer compatible. Type is neither move or copy assignable");

        return true;
    }

public:
    iterator begin()
    {
        return iterator{buffer_, wrapped_index(start_index_)};
    }

    iterator end()
    {
        return iterator{buffer_, wrapped_index(end_index_)};
    }

    const_iterator cbegin() const
    {
        return const_iterator{buffer_, wrapped_index(start_index_)};
    }

    const_iterator cend() const
    {
        return const_iterator{buffer_, wrapped_index(end_index_)};
    }

    // should not be used in production, unit-testing API only
    const container_type& get_buffer() const noexcept
    {
        return buffer_;
    }

private:
    constexpr uint64_t wrapped_index(uint64_t index) const noexcept
    {
        return index & (ExponentSize - 1);
    }

private:
    container_type buffer_;
    alignas(std::hardware_destructive_interference_size) uint64_t start_index_ = 0;
    alignas(std::hardware_destructive_interference_size) uint64_t end_index_ = 0;
};

} // namespace ds
