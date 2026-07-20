#include "../src/fifo_5.hpp"

#include <boost/lockfree/policies.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <locale>
#include <pthread.h>
#include <thread>
#include <unistd.h>

namespace benchmarks {

struct apostrophe_sep : std::numpunct<char>
{
    char do_thousands_sep() const override { return '\''; }
    std::string do_grouping() const override { return "\3"; } // group every 3 digits
};

template <typename T>
inline __attribute__((always_inline)) void DoNotOptimize(const T& value)
{
    asm volatile("" : : "r,m" (value) : "memory");
}

static void pin_thread(int cpu)
{
    if (cpu < 0)
        return;

    ::cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (::pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == -1)
    {
        std::perror("pthread_setaffinity_rp");
        std::exit(EXIT_FAILURE);
    }
}

template <typename Derived>
class benchmark_base : public Derived
{
public:
    using value_type = Derived::value_type;

    auto operator()(uint64_t iterations, int cpu1, int cpu2)
    {
        using namespace std::chrono_literals;

        // push the pop operations into a thread
        auto thread = std::jthread([this, iterations, cpu1] {
            pin_thread(cpu1);

            // pop warmup
            for (auto i = value_type{}; i < Derived::FIFO_SIZE; ++i)
            {
                Derived::pop(i);
            }

            // pop benchmark run
            for (auto i = value_type{}; i < iterations; ++i)
            {
                Derived::pop(i);
            }
        });

        pin_thread(cpu2);

        // push warmup
        for (auto i = value_type{}; i < Derived::FIFO_SIZE; ++i)
        {
            Derived::push(i);
        }

        Derived::wait_for_empty();

        // push benchmark run
        const auto start = std::chrono::steady_clock::now();
        for (auto i = value_type{}; i < iterations; ++i)
        {
            Derived::push(i);
        }
        Derived::wait_for_empty();
        const auto end = std::chrono::steady_clock::now();

        const auto delta = end - start;
        // delta is measured in nanoseconds
        // we will multiple by 1s to change from ops/ns -> ops/sec
        return (iterations * 1s) / delta;
    }
};

template <typename QueueT>
class benchmarking
{
protected:
    static constexpr std::size_t FIFO_SIZE = 131'072;
    using value_type = QueueT::value_type;

    void push(value_type value)
    {
        while (auto again = !queue_.push(value))
        {
            DoNotOptimize(again);
        }
    }

    void pop(value_type expected)
    {
        std::optional<value_type> val{};
        do
        {
            val = queue_.pop();
        } while (!val.has_value());

        if (val.value() != expected)
        {
            throw std::runtime_error("invalid value!");
        }
    }

    void wait_for_empty()
    {
        while (auto again = !queue_.empty())
        {
            DoNotOptimize(again);
        }
    }

    QueueT queue_{FIFO_SIZE};
};

template <std::integral T>
struct use_boost_tag{};

template <typename T>
class benchmarking<use_boost_tag<T>>
{
protected:
    static constexpr std::size_t FIFO_SIZE = 131'072;
    using value_type = T;

    void push(value_type value)
    {
        while (auto again = !queue_.push(value))
        {
            DoNotOptimize(again);
        }
    }

    void pop(value_type expected)
    {
        value_type val{};
        while (auto again = !queue_.pop(val));

        if (val != expected)
        {
            throw std::runtime_error("invalid value!");
        }
    }

    void wait_for_empty()
    {
        while (auto again = !queue_.empty())
        {
            DoNotOptimize(again);
        }
    }

    boost::lockfree::spsc_queue<T, boost::lockfree::capacity<FIFO_SIZE>> queue_{};
};

template <typename FifoT>
void bench(std::size_t iterations, const char* name, int argc, char* argv[])
{
    int cpu1 = 1;
    int cpu2 = 2;
    // ./exe cpu1 cpu2
    if (argc == 3)
    {
        cpu1 = std::atoi(argv[1]);
        cpu2 = std::atoi(argv[2]);
    }

    benchmark_base<benchmarking<FifoT>> benchmarker;
    auto ops_per_sec = benchmarker(iterations, cpu1, cpu2);

    std::cout << std::format("    {}", name) << " : " << ops_per_sec << " ops/sec\n";
}

void print_cache_sizes()
{
    std::cout << "Cache Line Information:\n";
    std::cout << std::setw(11) << std::left << "    L1d" << ": " << ::sysconf(_SC_LEVEL1_DCACHE_SIZE) << " bytes\n"
              << std::setw(11) << std::left << "    L1i" << ": " << ::sysconf(_SC_LEVEL1_ICACHE_SIZE) << " bytes\n"
              << std::setw(11) << std::left << "    L1 Line" << ": " << ::sysconf(_SC_LEVEL1_DCACHE_LINESIZE) << " bytes\n"
              << std::setw(11) << std::left << "    L2 " << ": " << ::sysconf(_SC_LEVEL2_CACHE_SIZE) << " bytes\n"
              << std::setw(11) << std::left << "    L3 " << ": " << ::sysconf(_SC_LEVEL3_CACHE_SIZE) << " bytes\n\n";
}

} // namespace benchmarks

int main(int argc, char* argv[])
{
    std::cout.imbue(std::locale(std::cout.getloc(), new benchmarks::apostrophe_sep));
    benchmarks::print_cache_sizes();

    constexpr std::size_t iterations = 400'000'0001;
    // constexpr std::size_t iterations = 100'000'001;

    std::cout << "Starting benchmarks with iteration count: " << iterations << '\n';
    benchmarks::bench<fifo_5<std::size_t>>(iterations, "fifo_exponent_atomic", argc, argv);
    benchmarks::bench<benchmarks::use_boost_tag<std::size_t>>(iterations, "boost::lockfree::spsc_queue", argc, argv);
}
