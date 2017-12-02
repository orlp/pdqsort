#include <random>
#include <ctime>
#include <vector>
#include <iostream>
#include <chrono>
#include <utility>
#include <array>
#include <type_traits>
#include <functional>
#include <string>

#include "../pdqsort.h"
#include "timsort.h"

#ifdef _WIN32
    #include <intrin.h>
    #define rdtsc __rdtsc
#else
    #ifdef __i586__
        static __inline__ unsigned long long rdtsc() {
            unsigned long long int x;
            __asm__ volatile(".byte 0x0f, 0x31" : "=A" (x));
            return x;
        }
    #elif defined(__x86_64__)
        static __inline__ unsigned long long rdtsc(){
            unsigned hi, lo;
            __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
            return ((unsigned long long) lo) | (((unsigned long long) hi) << 32);
        }
    #else
        #error no rdtsc implementation
    #endif
#endif


std::vector<int> shuffled_int(int size, std::mt19937_64& rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(i);
    std::shuffle(v.begin(), v.end(), rng);
    return v;
}

std::vector<int> shuffled_16_values_int(int size, std::mt19937_64& rng) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(i % 16);
    std::shuffle(v.begin(), v.end(), rng);
    return v;
}

std::vector<int> all_equal_int(int size, std::mt19937_64&) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(0);
    return v;
}

std::vector<int> ascending_int(int size, std::mt19937_64&) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size; ++i) v.push_back(i);
    return v;
}

std::vector<int> descending_int(int size, std::mt19937_64&) {
    std::vector<int> v; v.reserve(size);
    for (int i = size - 1; i >= 0; --i) v.push_back(i);
    return v;
}

std::vector<int> pipe_organ_int(int size, std::mt19937_64&) {
    std::vector<int> v; v.reserve(size);
    for (int i = 0; i < size/2; ++i) v.push_back(i);
    for (int i = size/2; i < size; ++i) v.push_back(size - i);
    return v;
}

std::vector<int> push_front_int(int size, std::mt19937_64&) {
    std::vector<int> v; v.reserve(size);
    for (int i = 1; i < size; ++i) v.push_back(i);
    v.push_back(0);
    return v;
}

std::vector<int> push_middle_int(int size, std::mt19937_64&) {
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



int main() {
    auto seed = std::time(0);
    std::mt19937_64 el;

    typedef std::vector<int> (*DistrF)(int, std::mt19937_64&);
    typedef void (*SortF)(std::vector<int>::iterator, std::vector<int>::iterator, std::less<int>);

    std::pair<std::string, DistrF> distributions[] = {
        {"shuffled_int", shuffled_int},
        {"shuffled_16_values_int", shuffled_16_values_int},
        {"all_equal_int", all_equal_int},
        {"ascending_int", ascending_int},
        {"descending_int", descending_int},
        {"pipe_organ_int", pipe_organ_int},
        {"push_front_int", push_front_int},
        {"push_middle_int", push_middle_int}
    };

    std::pair<std::string, SortF> sorts[] = {
        {"pdqsort", &pdqsort<std::vector<int>::iterator, std::less<int>>},
        {"std::sort", &std::sort<std::vector<int>::iterator, std::less<int>>},
        {"std::stable_sort", &std::stable_sort<std::vector<int>::iterator, std::less<int>>},
        // {"std::sort_heap", &heapsort<std::vector<int>::iterator, std::less<int>>},
        // {"timsort", &gfx::timsort<std::vector<int>::iterator, std::less<int>>}
    };

    int sizes[] = {1000000, 100};

    for (auto& distribution : distributions) {
        for (auto& sort : sorts) {
            el.seed(seed);

            for (auto size : sizes) {
                std::chrono::time_point<std::chrono::high_resolution_clock> total_start, total_end;
                std::vector<uint64_t> cycles;

                total_start = std::chrono::high_resolution_clock::now();
                total_end = std::chrono::high_resolution_clock::now();
                while (std::chrono::duration_cast<std::chrono::milliseconds>(total_end - total_start).count() < 5000) {
                    std::vector<int> v = distribution.second(size, el);
                    uint64_t start = rdtsc();
                    sort.second(v.begin(), v.end(), std::less<int>());
                    uint64_t end = rdtsc();
                    cycles.push_back(uint64_t(double(end - start) / size + 0.5));
                    total_end = std::chrono::high_resolution_clock::now();
                    // if (!std::is_sorted(v.begin(), v.end())) {
                    //     std::cerr << "sort failed: ";
                    //     std::cerr << size << " " << distribution.first << " " << sort.first << "\n";
                    // }
                }

                std::sort(cycles.begin(), cycles.end());

                std::cerr << size << " " << distribution.first << " " << sort.first
                          << " " << cycles[cycles.size()/2] << "\n";
                std::cout << size << " " << distribution.first << " " << sort.first
                          << " " << cycles[cycles.size()/2] << "\n";
            }
        }
    }

    return 0;
}
