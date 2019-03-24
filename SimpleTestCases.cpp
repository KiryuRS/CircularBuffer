#include "circular_buffer.hpp"
#include <string>
#include <array>
#include <iostream>

#ifdef __GNUC__

//the following are UBUNTU/LINUX, and MacOS ONLY terminal color codes.
#define RESET   	"\033[0m"
#define BLACK   	"\033[30m"      		/* Black */
#define RED     	"\033[31m"      		/* Red */
#define GREEN   	"\033[32m"      		/* Green */
#define YELLOW  	"\033[33m"      		/* Yellow */
#define BLUE    	"\033[34m"      		/* Blue */
#define MAGENTA 	"\033[35m"      		/* Magenta */
#define CYAN    	"\033[36m"      		/* Cyan */
#define WHITE   	"\033[37m"      		/* White */
#define BOLDBLACK   "\033[1m\033[30m"     	/* Bold Black */
#define BOLDRED     "\033[1m\033[31m"     	/* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"     	/* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"     	/* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"     	/* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"     	/* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"     	/* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"     	/* Bold White */


#define TEST(expr) \
	std::cout << __FUNCTION__ << ":" << __LINE__ \
    << (expr ? (std::string{ ": " } + GREEN + "PASSED" + RESET + ": ") : (std::string{ ": "} + RED + "FAILED" + RESET + ": ")) \
    << #expr << std::endl

#else

#define TEST(expr) std::cout << __FUNCTION__ << ":" << __LINE__ \
    << (expr ? ": \u001b[32m" "PASSED" "\u001b[0m: " : ": \u001b[31m" "FAILED" "\u001b[0m: ") \
    << #expr << std::endl

#endif

using IntCB = CircularBuffer<int>;
using FloatCB = CircularBuffer<float>;
using StringCB = CircularBuffer<std::string>;
using UCharCB = CircularBuffer<unsigned char>;
using CharCB = CircularBuffer<char>;

bool TestConstruction()
{
	std::array<int, 5> arr{ 1,2,3,4,5 };
	IntCB cb1{ };
	IntCB cb2{ arr.begin(), arr.end() };
	IntCB cb3{ cb2 };
	IntCB cb4{ std::move(cb3) };
	return cb1.capacity() == 1 && cb4[4] == 5;
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
	return cb4[4] == 5;
}

bool TestPushPop()
{
	StringCB cb;
	cb.push("Hello World!");
	cb.push("Goodbye World!");
	cb.push("HHH");
	cb.pop();
	return cb.size() == 0;
}

bool TestRangedForLoop()
{
	CharCB cb{ 6 };
	cb.push('A');
	cb.push('B');
	cb.push('C');
	cb.push('D');
	cb.push('E');
	cb.push('F');
	cb.push('G');
	const std::string result{ "GBCDEF" };
	std::string str;
	for (auto& elem : cb)
		str += elem;
	return result == str;
}

bool TestSubscript()
{
	std::array<int, 10> arr{ 1,2,3,4,5,6,7,8,9,10 };
	IntCB cb1{ arr.begin(), arr.end() };
	for (unsigned i = 1; i != 10; ++i)
		cb1[i] += cb1[i - 1];
	return cb1[9] == 55;
}

int main(void)
{
	std::cout << "\n=========================== BEGIN TEST ===========================\n";
	TEST(TestConstruction());
	TEST(TestAssignment());
	TEST(TestPushPop());
	TEST(TestRangedForLoop());
	TEST(TestSubscript());
	std::cout << "============================ END TEST ============================" << std::endl;
}