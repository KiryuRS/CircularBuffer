#include "../header/circular_buffer.hpp"
#include <string>
#include <array>
#include <iostream>
#include <algorithm>

#ifdef __GNUC__

//the following are UBUNTU/LINUX, and MacOS ONLY terminal color codes.
#define RESET       "\033[0m"
#define BLACK       "\033[30m"              /* Black */
#define RED         "\033[31m"              /* Red */
#define GREEN       "\033[32m"              /* Green */
#define YELLOW      "\033[33m"              /* Yellow */
#define BLUE        "\033[34m"              /* Blue */
#define MAGENTA     "\033[35m"              /* Magenta */
#define CYAN        "\033[36m"              /* Cyan */
#define WHITE       "\033[37m"              /* White */
#define BOLDBLACK   "\033[1m\033[30m"       /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"       /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"       /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"       /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"       /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"       /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"       /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"       /* Bold White */


#define TEST(expr) \
    std::cout << __FUNCTION__ << ":" << __LINE__ \
    << (expr ? (std::string{ ": " } + GREEN + "PASSED" + RESET + ": ") : (std::string{ ": "} + RED + "FAILED" + RESET + ": ")) \
    << #expr << std::endl

#else

#define TEST(expr) std::cout << __FUNCTION__ << ":" << __LINE__ \
    << (expr ? ": \u001b[32m" "PASSED" "\u001b[0m: " : ": \u001b[31m" "FAILED" "\u001b[0m: ") \
    << #expr << std::endl

#endif

using IntCB = circular_buffer<int>;
using FloatCB = circular_buffer<float>;
using StringCB = circular_buffer<std::string>;
using UCharCB = circular_buffer<unsigned char>;
using CharCB = circular_buffer<char>;

template <template <typename> class CB, typename T>
void PrintCircularBuffer(const CB<T>& cb)
{
    std::cout << "\n";
    for (const auto& elem : cb)
        std::cout << elem << " ";
    std::cout << "\n";
}

bool TestConstruction()
{
    std::array<int, 5> arr{ 1,2,3,4,5 };
    IntCB cb1(arr.size());
    IntCB cb2{ arr.begin(), arr.end() };
    IntCB cb3{ cb2 };
    IntCB cb4{ std::move(cb3) };
    IntCB cb5{ 1,2,3,4,5 };
    UCharCB cb(1);

    bool result1 = cb5.capacity() == cb4.size();
    bool result2 = cb4.back() == cb5.back();
    bool result3 = cb.front() == 0xCC;
    return result1 && result2 && result3;
}

bool TestAssignment()
{
    std::array<int, 5> arr{ 1,2,3,4,5 };
    IntCB cb1{ arr.begin(), arr.end() };
    IntCB cb2, cb3, cb4;
    cb2 = cb1;
    cb3 = std::move(cb2);
    cb4 = cb3;
    cb3 = cb4;
    cb4 = std::move(cb3);
    cb4 = cb4;
    return cb4[4] == arr.back();
}

bool TestPushEmplacePop()
{
    std::array<int, 5> arr{ -1, 1, 2, 3, 4 };
    IntCB cb(5);
    cb.push(-1);
    cb.emplace(1);
    cb.push(2);
    cb.emplace(3);
    cb.emplace(4);
    cb.emplace(5);
    cb.emplace(6);
    cb.emplace(7);
    cb.pop();
    cb.pop();
    cb.pop();
    cb.emplace(-1);
    cb.push(1);
    cb.emplace(2);
    int* cData = cb.data();
    bool is_same = true;
    for (unsigned i = 0; i != 5; ++i)
        is_same = cData[i] != arr[i] ? false : is_same;
    return is_same;
}

bool TestIterators()
{
    std::string str1, str2, str3;
    CharCB cb(7);
    cb.emplace('A');
    cb.emplace('B');
    cb.emplace('C');
    cb.emplace('D');
    cb.emplace('E');
    cb.emplace('F');
    cb.emplace('G');
    
    for (auto begin = cb.cbegin(), end = cb.cend(); begin != end; ++begin)
        str1 += *begin;
    for (auto begin = cb.crbegin(), end = cb.crend(); begin != end; ++begin)
        str2 += *begin;
    std::reverse(str2.begin(), str2.end());

    for (const auto& elem : cb)
        str3 += elem;
    auto rIter = cb.rbegin();
    ++rIter;
    auto iter = cb.begin();
    iter = iter + 6;
    --iter;

    bool result1 = str1 == str2;
    bool result2 = str3 == str1;
    bool result3 = *iter == *rIter;
    return result1 && result2 && result3;
}

bool TestSubscript()
{
    std::array<int, 10> arr{ 1,2,3,4,5,6,7,8,9,10 };
    IntCB cb1{ arr.begin(), arr.end() };
    for (unsigned i = 1; i != 10; ++i)
        cb1[i] += cb1[i - 1];
    bool success;
    try
    {
        success = cb1[999] == 55;           // should invoke a std::out_of_range error
    } catch (const std::out_of_range& e)
    {
        success = cb1[9] == 55;
    }
    return success;
}

bool TestSTLAlgorithms()
{
    std::array<int, 10> arr{ 1,2,3,4,5,6,7,8,9,10 };
    IntCB cb{ arr.begin(), arr.end() };
    int sum = 0;
    std::for_each(cb.begin(), cb.end(), [&sum](int value){ sum += value; });
    sum %= sum;
    std::fill(cb.begin(), cb.end(), 1);
    IntCB anotherCB(cb.size());
    std::copy(cb.begin(), cb.end(), anotherCB.begin());
    auto iter = std::find(anotherCB.begin(), anotherCB.end(), sum);
    return iter == anotherCB.end();
}

bool TestResize()
{
    std::array<int, 5> arr{ 1,2,3,4,5 };
    IntCB cb{ arr.begin(), arr.end() };
    cb.resize(10);
    cb.resize(20);
    cb.resize(7);
    cb.resize(6);
    cb.resize(1);
    cb.resize(3);
    cb.resize(5);
    int front = cb.front();
    int back = cb.back();
    return front == back;
}

int main(void)
{
    std::cout << "\n=========================== BEGIN TEST ===========================\n";
    TEST(TestConstruction());
    TEST(TestAssignment());
    TEST(TestPushEmplacePop());
    TEST(TestIterators());
    TEST(TestSubscript());
    TEST(TestSTLAlgorithms());
    TEST(TestResize());
    std::cout << "============================ END TEST ============================" << std::endl;
}