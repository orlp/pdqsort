/*
    pdqsort.h - Pattern-defeating quicksort.

    Copyright (c) 2015 Orson Peters

    This software is provided 'as-is', without any express or implied warranty. In no event will the
    authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose, including commercial
    applications, and to alter it and redistribute it freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not claim that you wrote the
       original software. If you use this software in a product, an acknowledgment in the product
       documentation would be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be misrepresented as
       being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#ifndef PDQSORT_H
#define PDQSORT_H

#include <utility>
#include <algorithm>
#include <functional>

#include <bits/predefined_ops.h>


enum {
    // Partitions below this size are sorted using insertion sort.
    __insertion_sort_threshold = 24,

    // When we detect an already sorted partition, attempt an insertion sort that allows this
    // amount of element moves before giving up.
    __partial_insertion_sort_limit = 8
};

// Sorts [__begin, __end) using insertion sort with the given comparison function.
template<typename _Iter, typename _Compare>
inline void __pdq_insertion_sort(_Iter __begin, _Iter __end, _Compare __comp) {
    typedef typename std::iterator_traits<_Iter>::value_type _ValT;
    if (__begin == __end) return;

    for (_Iter __cur = __begin + 1; __cur != __end; ++__cur) {
        _Iter __sift = __cur;
        _Iter __sift_1 = __cur - 1;

        // Compare first so we can elimite 2 moves for an element already positioned correctly.
        if (__comp(__sift, __sift_1)) {
            _ValT __tmp = _GLIBCXX_MOVE(*__cur);

            do { *__sift-- = _GLIBCXX_MOVE(*__sift_1--); }
            while (__sift != __begin && __gnu_cxx::__ops::__val_comp_iter(__comp)(__tmp, __sift_1));

            *__sift = _GLIBCXX_MOVE(__tmp);
        }
    }
}

// Sorts [__begin, __end) using insertion sort with the given comparison function. Assumes
// *(__begin - 1) is an element smaller than or equal to any element in [__begin, __end).
template<typename _Iter, typename _Compare>
inline void __pdq_unguarded_insertion_sort(_Iter __begin, _Iter __end, _Compare __comp) {
    typedef typename std::iterator_traits<_Iter>::value_type _ValT;
    if (__begin == __end) return;

    for (_Iter __cur = __begin + 1; __cur != __end; ++__cur) {
        _Iter __sift = __cur;
        _Iter __sift_1 = __cur - 1;

        // Compare first so we can elimite 2 moves for an element already positioned correctly.
        if (__comp(__sift, __sift_1)) {
            _ValT __tmp = _GLIBCXX_MOVE(*__cur);

            do { *__sift-- = _GLIBCXX_MOVE(*__sift_1--); }
            while (__gnu_cxx::__ops::__val_comp_iter(__comp)(__tmp, __sift_1));

            *__sift = _GLIBCXX_MOVE(__tmp);
        }
    }
}

// Attempts to use insertion sort on [__begin, __end). Will return false if more than
// __partial_insertion_sort_limit elements were moved, and abort sorting. Otherwise it will
// succesfully sort and return true.
template<typename _Iter, typename _Compare>
inline bool __partial_insertion_sort(_Iter __begin, _Iter __end, _Compare __comp) {
    typedef typename std::iterator_traits<_Iter>::value_type _ValT;
    if (__begin == __end) return true;
    
    int __limit = 0;
    for (_Iter __cur = __begin + 1; __cur != __end; ++__cur) {
        if (__limit > __partial_insertion_sort_limit) return false;

        _Iter __sift = __cur;
        _Iter __sift_1 = __cur - 1;

        // Compare first so we can elimite 2 moves for an element already positioned correctly.
        if (__comp(__sift, __sift_1)) {
            _ValT __tmp = _GLIBCXX_MOVE(*__cur);

            do {
                *__sift-- = _GLIBCXX_MOVE(*__sift_1--);
                ++__limit;
            } while (__sift != __begin && __gnu_cxx::__ops::__val_comp_iter(__comp)(__tmp, __sift_1));

            *__sift = _GLIBCXX_MOVE(__tmp);
        }
    }

    return true;
}

// Sorts the elements *a, *b and *c using comparison function __comp.
template<typename _Iter, typename _Compare>
inline void __sort3(_Iter a, _Iter b, _Iter c, _Compare __comp) {
    if (!__comp(b, a)) {
        if (!__comp(c, b)) return;

        std::iter_swap(b, c);
        if (__comp(b, a)) std::iter_swap(a, b);

        return;
    }

    if (__comp(c, b)) {
        std::iter_swap(a, c);
        return;
    }

    std::iter_swap(a, b);
    if (__comp(c, b)) std::iter_swap(b, c);
}

// Partitions [__begin, __end) around pivot *__begin using comparison function __comp. Elements equal
// to the pivot are put in the right-hand partition. Returns the position of the pivot after
// partitioning and whether the passed sequence already was correctly partitioned. Assumes the
// pivot is a median of at least 3 elements and that [__begin, __end) is at least
// __insertion_sort_threshold long.
template<typename _Iter, typename _Compare>
inline std::pair<_Iter, bool> __partition_right(_Iter __begin, _Iter __end, _Compare __comp) {
    typedef typename std::iterator_traits<_Iter>::value_type _ValT;
    
    // Move pivot into local for speed.
    _ValT __pivot(_GLIBCXX_MOVE(*__begin));

    _Iter __first = __begin;
    _Iter __last = __end;

    // Find the first element greater than or equal than the pivot (the median of 3 guarantees
    // this exists).
    while (__comp(++__first, __pivot));

    // Find the first element strictly smaller than the pivot. We have to guard this search if
    // there was no element before *__first.
    if (__first - 1 == __begin) while (__first < __last && !__comp(--__last, __pivot));
    else                        while (                    !__comp(--__last, __pivot));

    // If the first pair of elements that should be swapped to partition are the same element,
    // the passed in sequence already was correctly partitioned.
    bool __already_partitioned = __first >= __last;
    
    // Keep swapping pairs of elements that are on the wrong side of the pivot. Previously
    // swapped pairs guard the searches, which is why the first iteration is special-cased
    // above.
    while (__first < __last) {
        std::iter_swap(__first, __last);
        while (__comp(++__first, __pivot));
        while (!__comp(--__last, __pivot));
    }

    // Put the pivot in the right place.
    _Iter __pivot_pos = __first - 1;
    *__begin = _GLIBCXX_MOVE(*__pivot_pos);
    *__pivot_pos = _GLIBCXX_MOVE(__pivot);

    return std::make_pair(__pivot_pos, __already_partitioned);
}

// Similar function to the one above, except elements equal to the pivot are put to the left of
// the pivot and it doesn't check or return if the passed sequence already was partitioned.
template<typename _Iter, typename _Compare>
inline _Iter __partition_left(_Iter __begin, _Iter __end, _Compare __comp) {
    typedef typename std::iterator_traits<_Iter>::value_type _ValT;

    _ValT __pivot(_GLIBCXX_MOVE(*__begin));
    _Iter __first = __begin;
    _Iter __last = __end;
    
    while (__comp(__pivot, --__last));

    if (__last + 1 == __end) while (__first < __last && !__comp(__pivot, ++__first));
    else                     while (                    !__comp(__pivot, ++__first));

    while (__first < __last) {
        std::iter_swap(__first, __last);
        while (__comp(__pivot, --__last));
        while (!__comp(__pivot, ++__first));
    }

    _Iter __pivot_pos = __last;
    *__begin = _GLIBCXX_MOVE(*__pivot_pos);
    *__pivot_pos = _GLIBCXX_MOVE(__pivot);

    return __pivot_pos;
}


template<typename _Iter, typename _Compare>
inline void __pdqsort_loop(_Iter __begin, _Iter __end, _Compare __comp, int __bad_allowed, bool __leftmost = true) {
    typedef typename std::iterator_traits<_Iter>::difference_type _diff_t;

    // Use a while loop for tail recursion elimination.
    while (true) {
        _diff_t __size = __end - __begin;

        // Insertion sort is faster for small arrays.
        if (__size < __insertion_sort_threshold) {
            if (__leftmost) __pdq_insertion_sort(__begin, __end, __comp);
            else __pdq_unguarded_insertion_sort(__begin, __end, __comp);
            return;
        }

        // Choose pivot as median of 3.
        __sort3(__begin + __size / 2, __begin, __end - 1, __comp);

        // If *(__begin - 1) is the end of the right partition of a previous partition operation
        // there is no element in [*__begin, __end) that is smaller than *(__begin - 1). Then if our
        // pivot compares equal to *(__begin - 1) we change strategy, putting equal elements in
        // the left partition, greater elements in the right partition. We do not have to
        // recurse on the left partition, since it's sorted (all equal).
        if (!__leftmost && !__comp(__begin - 1, __begin)) {
            __begin = __partition_left(__begin, __end, __gnu_cxx::__ops::__val_comp_iter(__comp)) + 1;
            continue;
        }

        // Partition and get results.
        std::pair<_Iter, bool> __part_result = __partition_right(__begin, __end, __gnu_cxx::__ops::__iter_comp_val(__comp));
        _Iter __pivot_pos = __part_result.first;
        bool __already_partitioned = __part_result.second;

        // Check for a highly unbalanced partition.
        _diff_t __pivot_offset = __pivot_pos - __begin;
        bool __highly_unbalanced = __pivot_offset < __size / 8 || __pivot_offset > (__size - __size / 8);

        // If we got a highly unbalanced partition we shuffle elements to break many patterns.
        if (__highly_unbalanced) {
            // If we had too many bad partitions, switch to heapsort to guarantee O(n log n).
            if (--__bad_allowed == 0) {
                std::__make_heap(__begin, __end, __comp);
                std::__sort_heap(__begin, __end, __comp);
                return;
            }

            _diff_t __partition_size = __pivot_pos - __begin;
            if (__partition_size >= __insertion_sort_threshold) {
                std::iter_swap(__begin, __begin + __partition_size / 4);
                std::iter_swap(__pivot_pos - 1, __pivot_pos - __partition_size / 4);
            }
            
            __partition_size = __end - __pivot_pos;
            if (__partition_size >= __insertion_sort_threshold) {
                std::iter_swap(__pivot_pos + 1, __pivot_pos + __partition_size / 4);
                std::iter_swap(__end - 1, __end - __partition_size / 4);
            }
        } else {
            // If we were decently balanced and we tried to sort an already partitioned
            // sequence try to use insertion sort.
            if (__already_partitioned && __partial_insertion_sort(__begin, __pivot_pos, __comp)
                                      && __partial_insertion_sort(__pivot_pos + 1, __end, __comp)) return;
        }
            
        // Sort the left partition first using recursion and do tail recursion elimination for
        // the right-hand partition.
        __pdqsort_loop(__begin, __pivot_pos, __comp, __bad_allowed, __leftmost);
        __begin = __pivot_pos + 1;
        __leftmost = false;
    }
}


template<typename _Iter, typename _Compare>
inline void pdqsort(_Iter __begin, _Iter __end, _Compare __comp) {
    if (__begin == __end) return;
    __pdqsort_loop(__begin, __end,
                   __gnu_cxx::__ops::__iter_comp_iter(__comp),
                   std::__lg(__end - __begin));
}


template<typename _Iter>
inline void pdqsort(_Iter __begin, _Iter __end) {
    if (__begin == __end) return;
    __pdqsort_loop(__begin, __end,
                   __gnu_cxx::__ops::__iter_less_iter(),
                   std::__lg(__end - __begin));
}


#endif
