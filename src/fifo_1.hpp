#pragma once

#include "concepts.hpp"
#include "utility.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <memory>

// this is the basic version of a circular buffer. meant not to be used in a single threaded environment.
// also mandates that the capacity must be a power of 2 (automatically upgraded)

template <ring_compatible T, typename Allocator = std::allocator<T>>
class fifo_1 : Allocator
{
    using value_type = T;
    using allocator_traits = std::allocator_traits<Allocator>;
    using size_type = allocator_traits::size_type;

public:
    explicit fifo_1(size_type capacity, Allocator alloc = Allocator{})
        : Allocator{alloc}
        , capacity_{round_nearest_exponent(capacity)}
        , buffer_{allocator_traits::allocate(*this, capacity_)}
        , push_cursor_{}
        , pop_cursor_{}
    {
    }

    // okay for copy and move semantics
    fifo_1(const fifo_1& other)
        : Allocator{other}
        , capacity_{other.capacity_}
        , buffer_{allocator_traits::allocate(*this, capacity_)}
        , push_cursor_{other.push_cursor_}
        , pop_cursor_{other.pop_cursor_}
    {
        // copy the contents over from the other's buffer
        std::ranges::copy(other.buffer_, buffer_);
    }

    fifo_1& operator=(const fifo_1& other)
    {
        if (*this == other)
        {
            return *this;
        }

        destroy_all_elements();
        buffer_ = allocator_traits::allocate(*this, other.capacity_);
        std::ranges::copy(other.buffer_, buffer_);

        capacity_ = other.capacity_;
        push_cursor_ = other.push_cursor_;
        pop_cursor_ = other.pop_cursor_;
        return *this;
    }

    fifo_1(fifo_1&& other) noexcept
        : Allocator{other}
        , capacity_{other.capacity_}
        , buffer_{other.buffer_}
        , push_cursor_{other.push_cursor_}
        , pop_cursor_{other.pop_cursor_}
    {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
        other.push_cursor_ = 0;
        other.pop_cursor_ = 0;
    }

    fifo_1& operator=(fifo_1&& other) noexcept
    {
        if (*this == other)
        {
            return *this;
        }

        destroy_all_elements();
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
        std::swap(push_cursor_, other.push_cursor_);
        std::swap(pop_cursor_, other.pop_cursor_);
        return *this;
    }

    ~fifo_1()
    {
        destroy_all_elements();
    }

    constexpr std::size_t capacity() const
    {
        return capacity_;
    }

    constexpr std::size_t size() const
    {
        return push_cursor_ - pop_cursor_;
    }

    constexpr bool empty() const
    {
        return size() == 0;
    }

    constexpr bool full() const
    {
        return size() == capacity_;
    }

    constexpr auto operator<=>(const fifo_1&) const noexcept = default;

    // push one object into the fifo
    // @param value: object to be pushed into the fifo
    // @return: true if object can be pushed into fifo, false if fifo is full
    template <std::convertible_to<value_type> U>
    bool push(U&& value)
    {
        if (full())
        {
            return false;
        }

        std::construct_at(get_element(push_cursor_), std::forward<U>(value));
        ++push_cursor_;
        return true;
    }

    // pop an object from the fifo
    // @return: value in std::optional if fifo is not empty, else std::nullopt
    std::optional<value_type> pop()
    {
        if (empty())
        {
            return std::nullopt;
        }

        auto* data_ptr = get_element(pop_cursor_);
        value_type data{*data_ptr};
        std::destroy_at(data_ptr);
        ++pop_cursor_;
        return data;
    }

private:
    void destroy_all_elements()
    {
        size_type copy_push = push_cursor_;
        size_type copy_pop = pop_cursor_;

        while (copy_pop != copy_push)
        {
            std::destroy_at(get_element(copy_pop++));
        }
        allocator_traits::deallocate(*this, buffer_, capacity_);
    }

    template <typename Self>
    auto get_element(this Self&& self, size_type index)
    {
        return std::forward<Self>(self).buffer_ + (index & (self.capacity_ - 1));
    }

    const size_type capacity_;
    T* buffer_;

    size_type push_cursor_;
    size_type pop_cursor_;
};
