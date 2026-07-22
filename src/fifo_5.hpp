#pragma once

#include "concepts.hpp"

#include <atomic>
#include <memory>
#include <optional>
#include <utility>

template <ring_compatible T, typename Alloc = std::allocator<T>>
class fifo_5 : Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = allocator_traits::size_type;

    explicit fifo_5(size_type capacity, const Alloc& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{capacity}
        , ring_{allocator_traits::allocate(*this, capacity_)}
        , push_cursor_{}
        , cached_push_cursor_{}
        , pop_cursor_{}
        , cached_pop_cursor_{}
    {
        if (!power_of_2(capacity_))
        {
            throw std::runtime_error("capacity is not the power of 2!");
        }
    }

    ~fifo_5()
    {
        while (!empty())
        {
            const auto pop_cursor = pop_cursor_.load(std::memory_order_relaxed);
            std::destroy_at(get_element(pop_cursor));
            pop_cursor_.store(pop_cursor + 1, std::memory_order_relaxed);
        }
        allocator_traits::deallocate(*this, ring_, capacity_);
    }

public:
    // returns the number of elements in the fifo
    constexpr size_type size() const noexcept
    {
        return push_cursor_.load(std::memory_order_relaxed) - pop_cursor_.load(std::memory_order_relaxed);
    }

    // returns whether the container has no elements
    constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    // returns whether the container has reached the buffer limits
    constexpr bool full() const noexcept
    {
        return size() == capacity_;
    }

    // returns the number of elements that can fit into the fifo
    constexpr size_type capacity() const noexcept
    {
        return capacity_;
    }

    // push one object into the fifo
    // @param value: object to be pushed into the fifo
    // @return: true if object can be pushed into fifo, false if fifo is full
    template <std::convertible_to<value_type> U>
    bool push(U&& object)
    {
        const auto push_cursor = push_cursor_.load(std::memory_order_relaxed);
        if (full(push_cursor, cached_pop_cursor_))
        {
            cached_pop_cursor_ = pop_cursor_.load(std::memory_order_acquire);
            if (full(push_cursor, cached_pop_cursor_))
            {
                return false;
            }
        }

        std::construct_at(get_element(push_cursor), std::forward<U>(object));
        push_cursor_.store(push_cursor + 1, std::memory_order_release);
        return true;
    }

    // pop an object from the fifo
    // @return: value in std::optional if fifo is not empty, else std::nullopt
    std::optional<value_type> pop()
    {
        const auto pop_cursor = pop_cursor_.load(std::memory_order_relaxed);
        if (empty(cached_push_cursor_, pop_cursor))
        {
            cached_push_cursor_ = push_cursor_.load(std::memory_order_acquire);
            if (empty(cached_push_cursor_, pop_cursor_))
            {
                return std::nullopt;
            }
        }

        auto* elem_at = get_element(pop_cursor);
        value_type copy = *elem_at;
        std::destroy_at(elem_at);
        pop_cursor_.store(pop_cursor + 1, std::memory_order_release);
        return copy;
    }

private:
    static bool power_of_2(size_type capacity)
    {
        return ((capacity) & (capacity - 1)) == 0;
    }

    auto* get_element(this auto&& self, size_type cursor)
    {
        return std::forward<decltype(self)>(self).ring_ + (cursor & (self.capacity_ - 1));
    }

    size_type size(size_type push_cursor, size_type pop_cursor) const
    {
        return push_cursor - pop_cursor;
    }

    bool full(size_type push_cursor, size_type pop_cursor) const
    {
        return size(push_cursor, pop_cursor) == capacity_;
    }

    bool empty(size_type push_cursor, size_type pop_cursor) const
    {
        const auto maybe_inaccurate_size = size(push_cursor, pop_cursor);
        return maybe_inaccurate_size == 0 || maybe_inaccurate_size > capacity_;
    }

private:
    using atomic_size_t = std::atomic<size_type>;
    static_assert(atomic_size_t::is_always_lock_free);

    static constexpr size_type hardware_destructive_interference_size = 64;

    const std::size_t capacity_;
    T* ring_;

    alignas(hardware_destructive_interference_size) atomic_size_t push_cursor_;
    alignas(hardware_destructive_interference_size) atomic_size_t cached_push_cursor_;

    alignas(hardware_destructive_interference_size) atomic_size_t pop_cursor_;
    alignas(hardware_destructive_interference_size) atomic_size_t cached_pop_cursor_;

    // padding to avoid false sharing with adjacent objects
    char padding_[hardware_destructive_interference_size - sizeof(atomic_size_t)]{};
};
