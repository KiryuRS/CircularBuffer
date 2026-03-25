#pragma once

#include "circular_buffer_interface.hpp"

#include <concepts>
#include <new>
#include <utility>

namespace ds {

namespace detail {

template <typename T>
concept always_false = false;

} // namespace detail

// overflow version - FIFO behavior
template <typename T, std::size_t Size>
class circular_buffer : circular_buffer_base<T, Size>
{
    using base = circular_buffer_base<T, Size>;

public:
    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return base::empty(start_index_, end_index_);
    }

    [[nodiscard]] constexpr base::size_type size() const noexcept
    {
        return base::size(start_index_, end_index_);
    }

    using base::capacity;
    using base::get_buffer;

// modifier APIs
public:
    template <typename ... Args>
        requires std::constructible_from<T, std::remove_cvref_t<Args>...>
    constexpr void emplace(Args&& ... args)
    {
        auto ptr = base::buffer_.data() + (base::wrapped_index(end_index_++));
        new (ptr) base::value_type{std::forward<Args>(args)...};
    }

    template <typename U, typename RawU = std::remove_cvref_t<U>>
        requires std::constructible_from<T, U> ||
                 std::same_as<T, RawU>
    constexpr void push(U&& value)
    {
        emplace(std::forward<U>(value));
    }

    bool pop(T& value) // pops from the start_index_
    {
        if (empty())
            return false;

        if constexpr (std::is_move_constructible_v<T>)
            value = std::move(base::buffer_[base::wrapped_index(start_index_++)]);
        else if constexpr (std::is_copy_assignable_v<T>)
            value = base::buffer_[base::wrapped_index(start_index_++)];
        else
            static_assert(detail::always_false<T>, "Not circular buffer compatible. Type is neither move or copy assignable");

        return true;
    }

public:
    base::iterator begin()
    {
        return {base::buffer_, base::wrapped_index(start_index_)};
    }

    base::iterator end()
    {
        return {base::buffer_, base::wrapped_index(end_index_)};
    }

    base::const_iterator cbegin() const
    {
        return {base::buffer_, base::wrapped_index(start_index_)};
    }

    base::const_iterator cend() const
    {
        return {base::buffer_, base::wrapped_index(end_index_)};
    }

private:
    uint64_t start_index_ = 0;
    uint64_t end_index_ = 0;
};

} // namespace ds
