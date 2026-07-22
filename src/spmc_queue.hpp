#pragma once

#include "concepts.hpp"
#include "utility.hpp"

#include <atomic>
#include <immintrin.h>
#include <memory>
#include <expected>
#include <utility>
#include <debugging>

enum class error_status
{
    EMPTY_BUFFER = 1 << 1,
    CONSUMER_TIMEOUT = 1 << 2,
};

// this version makes consumer(s) contest for pending work.
// targets for thread_pool with a single producer.
//
// In most of the implementation it will be vastly similar to fifo_5.hpp.
// Main difference is in pop function
template <ring_compatible T, typename Alloc = std::allocator<T>>
class spmc_queue : Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = allocator_traits::size_type;

    explicit spmc_queue(size_type capacity, const Alloc& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{round_nearest_exponent(capacity)}
        , ring_{allocator_traits::allocate(*this, capacity_)}
        , push_cursor_{}
        , cached_push_cursor_{}
        , pop_cursor_{}
        , cached_pop_cursor_{}
    {
    }

    ~spmc_queue()
    {
        while (!empty())
        {
            const auto pop_cursor = pop_cursor_.load(std::memory_order_relaxed);
            std::destroy_at(get_element(pop_cursor));
            pop_cursor_.store(pop_cursor + 1, std::memory_order_relaxed);
        }
        allocator_traits::deallocate(*this, ring_, capacity_);
    }

    spmc_queue(const spmc_queue&) = delete;
    spmc_queue(spmc_queue&&) noexcept = delete;
    spmc_queue& operator=(const spmc_queue&) = delete;
    spmc_queue& operator=(spmc_queue&&) noexcept = delete;

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
    // @return: value wrapped in std::expected. std::nullopt either if empty, or consumer failed retrieval after some retries
    std::expected<value_type, error_status> pop()
    {
        // do not make the consumer fall into an infinite retry - give other threads a chance
        for (uint8_t i = 0; i != retries; ++i)
        {
            auto pop_cursor = pop_cursor_.load(std::memory_order_relaxed);
            if (empty(cached_push_cursor_, pop_cursor))
            {
                cached_push_cursor_ = push_cursor_.load(std::memory_order_acquire);
                if (empty(cached_push_cursor_, pop_cursor_))
                {
                    return std::unexpected{error_status::EMPTY_BUFFER};
                }
            }

            auto* elem_at = get_element(pop_cursor);
            value_type copy = *elem_at;

            // ensure that no other consumers have retrieved this same value and moved ahead of us.
            // if so, we undergo a spin-lock and retry again
            auto temp_pop_cursor = pop_cursor;
            if (!pop_cursor_.compare_exchange_weak(temp_pop_cursor, pop_cursor + 1, std::memory_order_release, std::memory_order_relaxed))
            {
                pop_cursor = temp_pop_cursor;
                _mm_pause();
                continue;
            }

            // destroy the element once we are certain that we are the only consumer
            std::destroy_at(elem_at);
            return copy;
        }

        return std::unexpected{error_status::CONSUMER_TIMEOUT};
    }

private:
    template <typename Self>
    auto* get_element(this Self&& self, size_type index)
    {
        return std::forward<Self>(self).ring_ + (index & (self.capacity_ - 1));
    }

    size_type size(size_type push_cursor, size_type pop_cursor) const
    {
        return push_cursor - pop_cursor;
    }

    // used by push thread
    bool full(size_type push_cursor, size_type pop_cursor) const
    {
        return size(push_cursor, pop_cursor) == capacity_;
    }

    // used by pop thread(s)
    bool empty(size_type push_cursor, size_type pop_cursor) const
    {
        // the cached_push_cursor_ will always start from 0, once we have a non-zero pop_cursor,
        // the value returned from size will always underflow. hence its not 100% accurate to check if its 0.
        const auto maybe_inaccurate_size = size(push_cursor, pop_cursor);
        return maybe_inaccurate_size == 0 || maybe_inaccurate_size > capacity_;
    }

private:
    using atomic_size_t = std::atomic<size_type>;
    static_assert(atomic_size_t::is_always_lock_free);

    static constexpr size_type hardware_destructive_interference_size = 64;
    static constexpr uint8_t retries = 5;

    const size_type capacity_;
    T* ring_;

    alignas(hardware_destructive_interference_size) atomic_size_t push_cursor_;
    alignas(hardware_destructive_interference_size) atomic_size_t cached_push_cursor_;

    alignas(hardware_destructive_interference_size) atomic_size_t pop_cursor_;
    alignas(hardware_destructive_interference_size) atomic_size_t cached_pop_cursor_;

    // padding to avoid false sharing with adjacent objects
    char padding_[hardware_destructive_interference_size - sizeof(atomic_size_t)]{};
};
