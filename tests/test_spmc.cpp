#include <atomic>
#include <mutex>
#include <spmc_queue.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <immintrin.h>
#include <thread>
#include <ranges>
#include <vector>

namespace tests {

using namespace ::testing;

template <typename T>
struct test_multiple_consumers : ::testing::Test
{
    static constexpr std::size_t value = T::value;
};

using test_types = ::testing::Types<
    std::integral_constant<std::size_t, 1>,
    std::integral_constant<std::size_t, 2>,
    std::integral_constant<std::size_t, 4>,
    std::integral_constant<std::size_t, 8>,
    std::integral_constant<std::size_t, 16>
>;
TYPED_TEST_SUITE(test_multiple_consumers, test_types);

TYPED_TEST(test_multiple_consumers, test_N_consumers_push_first)
{
    constexpr auto threads_count = this->value;

    std::atomic<uint32_t> consumer_timeouts;
    std::atomic<bool> ready_flag;

    using test_type = int;
    static constexpr std::size_t FIFO_SIZE = 4'096;
    spmc_queue<test_type> under_test{FIFO_SIZE};
    constexpr int iterations = FIFO_SIZE;

    std::vector<int> actual_values;
    actual_values.reserve(FIFO_SIZE);
    std::mutex mutex;

    auto pop_thread_impl = [&] (std::stop_token token) {
        std::vector<int> thread_values;
        thread_values.reserve(FIFO_SIZE);

        // wait for producer to produce something first ...
        bool expected = false;
        ready_flag.wait(expected, std::memory_order_acquire);

        while (!token.stop_requested())
        {
            const auto result = under_test.pop();
            if (result.has_value())
            {
                thread_values.push_back(result.value());
            }
            else if (result.error() == error_status::CONSUMER_TIMEOUT)
            {
                consumer_timeouts.fetch_add(1, std::memory_order_relaxed);
            }
        }

        // add it back to the main thread before exiting ...
        {
            std::scoped_lock _{mutex};
            std::ranges::copy(thread_values, std::back_inserter(actual_values));
        }
    };

    // scoped namespace for testing.
    // print heuristics / EXPECT macros later
    {
        std::array<std::jthread, threads_count> threads;
        template for (constexpr auto i : std::views::iota(0uz, threads_count))
        {
            threads[i] = std::jthread{pop_thread_impl};
        }

        for (int i = 0; i < iterations; ++i)
        {
            under_test.push(i);
        }

        // signal all consumer threads to start consuming
        ready_flag.store(true, std::memory_order_release);
        ready_flag.notify_all();

        // wait for all consumers to finish consuming
        while (!under_test.empty()) _mm_pause();
    }

    std::cout << "Number of consumer timeouts: " << consumer_timeouts.load(std::memory_order_relaxed) << '\n';

    const std::vector<int> expected_values{std::from_range_t{}, std::views::iota(0uz, FIFO_SIZE)};
    std::ranges::sort(actual_values);
    EXPECT_EQ(expected_values, actual_values);
}

TYPED_TEST(test_multiple_consumers, test_N_consumers_concurrent_push_pop)
{
    constexpr auto threads_count = this->value;

    std::atomic<bool> ready_flag;

    using test_type = int;
    static constexpr std::size_t FIFO_SIZE = 4'096;
    spmc_queue<test_type> under_test{FIFO_SIZE};
    constexpr int iterations = FIFO_SIZE;

    std::vector<int> actual_values;
    actual_values.reserve(FIFO_SIZE);
    std::mutex mutex;

    auto pop_thread = [&] (std::stop_token stop_token) {
        std::vector<int> popped_values;
        popped_values.reserve(FIFO_SIZE);

        while (!stop_token.stop_requested())
        {
            if (auto result = under_test.pop(); result.has_value())
            {
                popped_values.push_back(result.value());
            }
        }

        std::scoped_lock _{mutex};
        std::ranges::copy(popped_values, std::back_inserter(actual_values));
    };

    // scoped namespace for testing.
    // EXPECT macros after
    {
        std::array<std::jthread, threads_count> threads;
        template for (constexpr auto i : std::views::iota(0uz, threads_count))
        {
            threads[i] = std::jthread{pop_thread};
        }

        for (auto i = 0; i < iterations; ++i)
        {
            while (!under_test.push(i));
        }

        // wait for threads to finish consumption
        while (!under_test.empty()) _mm_pause();
    }

    const std::vector<int> expected_values{std::from_range_t{}, std::views::iota(0uz, FIFO_SIZE)};
    std::ranges::sort(actual_values);
    EXPECT_EQ(expected_values, actual_values);
}

} // namespace tests
