#include <circular_buffer.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ranges>
#include <sstream>

namespace ds::tests {

using namespace ::testing;

TEST(test_circular_buffer, test_capacity)
{
    circular_buffer<int, 1> cb1;
    EXPECT_EQ(cb1.capacity(), 1);

    circular_buffer<int, 9> cb2;
    EXPECT_EQ(cb2.capacity(), 16); // next exponent == 16

    circular_buffer<int, 500> cb3;
    EXPECT_EQ(cb3.capacity(), 512); // next exponent == 512

    circular_buffer<int, 256> cb4;
    EXPECT_EQ(cb4.capacity(), 256);
}

TEST(test_circular_buffer, test_empty)
{
    circular_buffer<int, 2> cb;
    cb.push(19);
    cb.push(100);

    int value{};
    cb.pop(value);
    cb.pop(value);

    EXPECT_TRUE(cb.empty());
}

TEST(test_circular_buffer, test_size)
{
    circular_buffer<int, 16> cb;

    // push 10 values
    for (int i : std::views::iota(0, 10))
        cb.push(i);
    EXPECT_EQ(cb.size(), 10);

    // pop 3  values
    for (int _ : std::views::iota(0, 3))
    {
        int value{};
        cb.pop(value);
    }
    EXPECT_EQ(cb.size(), 7);
}

TEST(test_circular_buffer, test_full_buffer_no_pop)
{
    circular_buffer<int, 10> cb; // will round up to 16
    for (int i : std::views::iota(0, 10))
        cb.push(i);

    EXPECT_THAT(cb.get_buffer(), ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, _, _, _, _, _, _));
}

TEST(test_circular_buffer, test_overflow_no_pop)
{
    circular_buffer<int, 8> cb;
    for (int i : std::views::iota(0, 15))
        cb.emplace(i);

    EXPECT_THAT(cb.get_buffer(), ElementsAre(8, 9, 10, 11, 12, 13, 14, 7));
}

TEST(test_circular_buffer, test_round_trip_no_overflow)
{
    circular_buffer<int, 16> cb;
    for (int i : std::views::iota(0, 16))
        cb.push(i);

    // initial buffer after push
    EXPECT_THAT(cb.get_buffer(), ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));

    // pop everything
    std::vector<int> popped_elements;
    popped_elements.reserve(16);
    bool result{};
    do
    {
        int value{};
        if (result = cb.pop(value); result)
            popped_elements.push_back(value);
    } while (result);

    EXPECT_THAT(popped_elements, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15));
}

}
