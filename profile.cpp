#include <random>
#include <ctime>
#include <vector>
#include <iostream>
#include <chrono>
#include <utility>
#include <array>
#include <type_traits>

#include "pdqsort.h"

// Helper function.
template<class... T>
inline constexpr std::array<typename std::common_type<T...>::type, sizeof...(T)>
make_array(T&&... t) {
    return {{ std::forward<T>(t)... }};
}

template<class URNG>
std::vector<int> shuffled_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(i);
    std::shuffle(v.begin(), v.end(), rng);
    return v;
}

template<class URNG>
std::vector<int> shuffled_16_values_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(i % 16);
    std::shuffle(v.begin(), v.end(), rng);
    return v;
}

template<class URNG>
std::vector<int> all_equal_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(0);
    return v;
}

template<class URNG>
std::vector<int> ascending_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(i);
    return v;
}

template<class URNG>
std::vector<int> descending_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = size - 1; i >= 0; --i) v.push_back(i);
    return v;
}

template<class URNG>
std::vector<int> pipe_organ_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size/2; ++i) v.push_back(i);
    for (int i = size/2; i < size; ++i) v.push_back(size - i);
    return v;
}

template<class URNG>
std::vector<int> push_front_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 1; i < size; ++i) v.push_back(i);
    v.push_back(0);
    return v;
}

template<class URNG>
std::vector<int> push_middle_int(size_t size, URNG rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) {
        if (i != size/2) v.push_back(i);
    }
    v.push_back(size/2);
    return v;
}


template<class Iter, class Compare>
void heapsort(Iter begin, Iter end, Compare comp) {
    std::make_heap(begin, end, comp);
    std::sort_heap(begin, end, comp);
}


static __inline__ uint64_t rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

int main(int argc, char** argv) {
    std::mt19937_64 el(time(0));

    auto distributions = make_array(
        std::make_pair("shuffled_int", shuffled_int<std::mt19937_64>),
        std::make_pair("shuffled_16_values_int", shuffled_16_values_int<std::mt19937_64>),
        std::make_pair("all_equal_int", all_equal_int<std::mt19937_64>),
        std::make_pair("ascending_int", ascending_int<std::mt19937_64>),
        std::make_pair("descending_int", descending_int<std::mt19937_64>),
        std::make_pair("pipe_organ_int", pipe_organ_int<std::mt19937_64>),
        std::make_pair("push_front_int", push_front_int<std::mt19937_64>),
        std::make_pair("push_middle_int", push_middle_int<std::mt19937_64>)
    );

    auto sorts = make_array(
        std::make_pair("heapsort", heapsort<std::vector<int>::iterator, std::less<int>>),
        std::make_pair("introsort", std::sort<std::vector<int>::iterator, std::less<int>>),
        std::make_pair("pdqsort", pdqsort<std::vector<int>::iterator, std::less<int>>)
    );

    std::vector<int> sizes = {1000000};

    for (auto& distribution : distributions) {
        for (auto& sort : sorts) {
            for (auto size : sizes) {
                std::chrono::time_point<std::chrono::high_resolution_clock> total_start, total_end;
                std::vector<uint64_t> timings;
                
                total_start = std::chrono::high_resolution_clock::now();
                total_end = std::chrono::high_resolution_clock::now();
                while (std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count() < 10000) {
                    std::vector<int> v = distribution.second(size, el);
                    uint64_t start = rdtsc();
                    sort.second(v.begin(), v.end(), std::less<int>());
                    uint64_t end = rdtsc();
                    timings.push_back(double(end - start) / size + 0.5);
                    total_end = std::chrono::high_resolution_clock::now();
                }

                std::cout << size << " " << distribution.first << " " << sort.first << " ";
                std::sort(timings.begin(), timings.end());
                for (uint64_t timing : timings) std::cout << timing << " ";
                std::cout << "\n";
                std::cerr << size << " " << distribution.first << " " << sort.first << "\n";

                // std::vector<std::vector<int>> vectors;
                // std::vector<std::chrono::nanoseconds> timings;
                // int count = 1;

                // do {
                //     vectors.clear();
                //     vectors.reserve(count);
                //     for (int i = 0; i < count; ++i) vectors.push_back(distribution.second(size, el));

                //     std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
                    
                //     timings.clear();
                //     timings.reserve(count);
                    
                //     total_start = std::chrono::high_resolution_clock::now();
                //     for (auto& v : vectors) {
                //         start = std::chrono::high_resolution_clock::now();
                //         sort.second(v.begin(), v.end(), std::less<int>());
                //         end = std::chrono::high_resolution_clock::now();
                //         timings.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
                //     }
                //     total_end = std::chrono::high_resolution_clock::now();

                //     count *= 2;
                // } while (std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count() < 500);

                // std::sort(timings.begin(), timings.end());

                // std::cout << timings[0].count() << " "
                //           << timings[timings.size()/4].count() << " "
                //           << timings[timings.size()/2].count() << " "
                //           << timings[timings.size() - timings.size() / 4].count() << " "
                //           << timings[timings.size() - 1].count() << "\n";
            }
        }
    }

    return 0;
}
