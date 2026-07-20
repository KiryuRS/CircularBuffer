#include <atomic>
#include <spmc.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <ranges>
#include <vector>

namespace tests {

using namespace ::testing;

TEST(test_spmc, single_producer_single_consumer)
{
    using test_type = int;
    static constexpr std::size_t FIFO_SIZE = 131'072;
    spmc<test_type> under_test{FIFO_SIZE};

    constexpr int iterations = FIFO_SIZE;
    std::jthread pop_thread{[&] {
        for (int i = 0; i < iterations; ++i)
        {
            // default constructor sets has_value == true for a weird reason. Let's set to something else
            std::expected<test_type, error_status> result{std::unexpected{error_status::EMPTY_BUFFER}};
            while (!result)
            {
                result = under_test.pop();
            }
            ASSERT_EQ(result.value(), i);
        }
    }};

    for (int i = 0; i < iterations; ++i)
    {
        while (!under_test.push(i));
    }
}

TEST(test_spmc, single_producer_2_consumers)
{
    std::atomic<uint32_t> consumer_timeouts;

    using test_type = int;
    static constexpr std::size_t FIFO_SIZE = 256;
    spmc<test_type> under_test{FIFO_SIZE};
    constexpr int iterations = FIFO_SIZE;

    std::vector<int> actual_values;
    actual_values.reserve(FIFO_SIZE);
    std::mutex mutex;

    auto pop_thread_impl = [&] (std::stop_token token) {
        std::cout << "Thread: " << std::this_thread::get_id() << " performing consumption ...\n";
        while (!token.stop_requested())
        {
            const auto result = under_test.pop();
            if (result.has_value())
            {
                std::lock_guard _{mutex};
                actual_values.push_back(result.value());
                continue;
            }

            if (result.error() == error_status::CONSUMER_TIMEOUT)
            {
                consumer_timeouts.fetch_add(1, std::memory_order_relaxed);
                continue;
            }
        }
    };

    // scoped namespace for testing.
    // print heuristics after test
    {
        std::jthread pop_1{pop_thread_impl};
        std::jthread pop_2{pop_thread_impl};

        for (int i = 0; i < iterations; ++i)
        {
            while(!under_test.push(i));
        }
    }

    const std::vector<int> expected_values{std::from_range_t{}, std::views::iota(0uz, FIFO_SIZE)};
    std::cout << std::format("Consumer timeouts: {}\n", consumer_timeouts.load(std::memory_order_relaxed));
    EXPECT_EQ(actual_values, expected_values);
}

} // namespace tests
