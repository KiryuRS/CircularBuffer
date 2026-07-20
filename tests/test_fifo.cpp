#include <fifo_5.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <format>
#include <thread>
#include <source_location>

namespace tests {

using namespace ::testing;

class test_fifo_single_thread : public ::testing::Test
{
protected:
    using test_type = int;

    void push(int i)
    {
        while(!under_test.push(i));
    }

    void pop(test_type expected, std::source_location location = std::source_location::current())
    {
        const std::string test_location = std::format("{}:{}", location.file_name(), location.line());
        std::optional<test_type> val;
        while (!val)
        {
            val = under_test.pop();
        }
        ASSERT_TRUE(val.has_value())  << "Expected buffer to be not empty. At line: " << test_location;
        EXPECT_EQ(val.value(), expected) << "Expected " << expected << ", got: " << val.value() << ". At line: " << test_location;
    }

    static constexpr std::size_t FIFO_SIZE = 131'072;
    fifo_5<test_type> under_test{FIFO_SIZE};
};

TEST_F(test_fifo_single_thread, test_push_pop_round_trip)
{
    constexpr int iterations = FIFO_SIZE;
    for (int i = 0; i < iterations; ++i)
    {
        push(i);
    }

    EXPECT_TRUE(under_test.full());

    // utilize RAII from jthread to join after all pop is done
    {
        std::jthread pop_thread{[&] {
            for (int i = 0; i < iterations; ++i)
            {
                pop(i);
            }
        }};
    }

    EXPECT_TRUE(under_test.empty());
}

} // namespace tests
