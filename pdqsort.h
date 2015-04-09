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


namespace pdqsort_detail {
    enum {
        // Partitions below this size are sorted using insertion sort.
        insertion_sort_threshold = 24,

        // When we detect an already sorted partition, attempt an insertion sort that allows this
        // amount of element moves before giving up.
        partial_insertion_sort_limit = 8
    };

    // Sorts [begin, end) using insertion sort with the given comparison function.
    template<class Iter, class Compare1, class Compare2>
    inline void insertion_sort(Iter begin, Iter end,
                               Compare1 comp_iter_iter, Compare2 comp_val_iter) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        if (begin == end) return;

        for (Iter cur = begin + 1; cur != end; ++cur) {
            Iter sift = cur;
            Iter sift_1 = cur - 1;

            // Compare first so we can elimite 2 moves for an element already positioned correctly.
            if (comp_iter_iter(sift, sift_1)) {
                T tmp = _GLIBCXX_MOVE(*cur);

                do { *sift-- = _GLIBCXX_MOVE(*sift_1--); }
                while (sift != begin && comp_val_iter(tmp, sift_1));

                *sift = _GLIBCXX_MOVE(tmp);
            }
        }
    }

    // Sorts [begin, end) using insertion sort with the given comparison function. Assumes
    // *(begin - 1) is an element smaller than or equal to any element in [begin, end).
    template<class Iter, class Compare1, class Compare2>
    inline void unguarded_insertion_sort(Iter begin, Iter end,
                                         Compare1 comp_iter_iter, Compare2 comp_val_iter) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        if (begin == end) return;

        for (Iter cur = begin + 1; cur != end; ++cur) {
            Iter sift = cur;
            Iter sift_1 = cur - 1;

            // Compare first so we can elimite 2 moves for an element already positioned correctly.
            if (comp_iter_iter(sift, sift_1)) {
                T tmp = _GLIBCXX_MOVE(*cur);

                do { *sift-- = _GLIBCXX_MOVE(*sift_1--); }
                while (comp_val_iter(tmp, sift_1));

                *sift = _GLIBCXX_MOVE(tmp);
            }
        }
    }

    // Attempts to use insertion sort on [begin, end). Will return false if more than
    // partial_insertion_sort_limit elements were moved, and abort sorting. Otherwise it will
    // succesfully sort and return true.
    template<class Iter, class Compare1, class Compare2>
    inline bool partial_insertion_sort(Iter begin, Iter end,
                                       Compare1 comp_iter_iter, Compare2 comp_val_iter) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        if (begin == end) return true;
        
        int limit = 0;
        for (Iter cur = begin + 1; cur != end; ++cur) {
            if (limit > partial_insertion_sort_limit) return false;

            Iter sift = cur;
            Iter sift_1 = cur - 1;

            // Compare first so we can elimite 2 moves for an element already positioned correctly.
            if (comp_iter_iter(sift, sift_1)) {
                T tmp = _GLIBCXX_MOVE(*cur);

                do {
                    *sift-- = _GLIBCXX_MOVE(*sift_1--);
                    ++limit;
                } while (sift != begin && comp_val_iter(tmp, sift_1));

                *sift = _GLIBCXX_MOVE(tmp);
            }
        }

        return true;
    }

    // Sorts the elements *a, *b and *c using comparison function comp.
    template<class Iter, class Compare>
    inline void sort3(Iter a, Iter b, Iter c, Compare comp) {
        if (!comp(b, a)) {
            if (!comp(c, b)) return;

            std::iter_swap(b, c);
            if (comp(b, a)) std::iter_swap(a, b);

            return;
        }

        if (comp(c, b)) {
            std::iter_swap(a, c);
            return;
        }

        std::iter_swap(a, b);
        if (comp(c, b)) std::iter_swap(b, c);
    }

    // Partitions [begin, end) around pivot *begin using comparison function comp. Elements equal
    // to the pivot are put in the right-hand partition. Returns the position of the pivot after
    // partitioning and whether the passed sequence already was correctly partitioned. Assumes the
    // pivot is a median of at least 3 elements and that [begin, end) is at least
    // insertion_sort_threshold long.
    template<class Iter, class Compare>
    inline std::pair<Iter, bool> partition_right(Iter begin, Iter end, Compare comp_iter_val) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        
        // Move pivot into local for speed.
        T pivot(_GLIBCXX_MOVE(*begin));

        Iter first = begin;
        Iter last = end;

        // Find the first element greater than or equal than the pivot (the median of 3 guarantees
        // this exists).
        while (comp_iter_val(++first, pivot));

        // Find the first element strictly smaller than the pivot. We have to guard this search if
        // there was no element before *first.
        if (first - 1 == begin) while (first < last && !comp_iter_val(--last, pivot));
        else                    while (                !comp_iter_val(--last, pivot));

        // If the first pair of elements that should be swapped to partition are the same element,
        // the passed in sequence already was correctly partitioned.
        bool already_partitioned = first >= last;
        
        // Keep swapping pairs of elements that are on the wrong side of the pivot. Previously
        // swapped pairs guard the searches, which is why the first iteration is special-cased
        // above.
        while (first < last) {
            std::iter_swap(first, last);
            while (comp_iter_val(++first, pivot));
            while (!comp_iter_val(--last, pivot));
        }

        // Put the pivot in the right place.
        Iter pivot_pos = first - 1;
        *begin = _GLIBCXX_MOVE(*pivot_pos);
        *pivot_pos = _GLIBCXX_MOVE(pivot);

        return std::make_pair(pivot_pos, already_partitioned);
    }

    // Similar function to the one above, except elements equal to the pivot are put to the left of
    // the pivot and it doesn't check or return if the passed sequence already was partitioned.
    template<class Iter, class Compare>
    inline Iter partition_left(Iter begin, Iter end, Compare comp_val_iter) {
        typedef typename std::iterator_traits<Iter>::value_type T;

        T pivot(_GLIBCXX_MOVE(*begin));
        Iter first = begin;
        Iter last = end;
        
        while (comp_val_iter(pivot, --last));

        if (last + 1 == end) while (first < last && !comp_val_iter(pivot, ++first));
        else                 while (                !comp_val_iter(pivot, ++first));

        while (first < last) {
            std::iter_swap(first, last);
            while (comp_val_iter(pivot, --last));
            while (!comp_val_iter(pivot, ++first));
        }

        Iter pivot_pos = last;
        *begin = _GLIBCXX_MOVE(*pivot_pos);
        *pivot_pos = _GLIBCXX_MOVE(pivot);

        return pivot_pos;
    }


    template<class Iter, class Compare>
    inline void pdqsort_loop(Iter begin, Iter end, Compare comp, int bad_allowed, bool leftmost = true) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        typedef typename std::iterator_traits<Iter>::difference_type diff_t;

        // Use a while loop for tail recursion elimination.
        while (true) {
            diff_t size = end - begin;

            // Insertion sort is faster for small arrays.
            if (size < insertion_sort_threshold) {
                if (leftmost) insertion_sort(begin, end, comp, __gnu_cxx::__ops::__val_comp_iter(comp));
                else unguarded_insertion_sort(begin, end, comp, __gnu_cxx::__ops::__val_comp_iter(comp));
                return;
            }

            // Choose pivot as median of 3.
            sort3(begin + size / 2, begin, end - 1, comp);

            // If *(begin - 1) is the end of the right partition of a previous partition operation
            // there is no element in [*begin, end) that is smaller than *(begin - 1). Then if our
            // pivot compares equal to *(begin - 1) we change strategy, putting equal elements in
            // the left partition, greater elements in the right partition. We do not have to
            // recurse on the left partition, since it's sorted (all equal).
            if (!leftmost && !comp(begin - 1, begin)) {
                begin = partition_left(begin, end, __gnu_cxx::__ops::__val_comp_iter(comp)) + 1;
                continue;
            }

            // Partition and get results.
            std::pair<Iter, bool> part_result = partition_right(begin, end, __gnu_cxx::__ops::__iter_comp_val(comp));
            Iter pivot_pos = part_result.first;
            bool already_partitioned = part_result.second;

            // Check for a highly unbalanced partition.
            diff_t pivot_offset = pivot_pos - begin;
            bool highly_unbalanced = pivot_offset < size / 8 || pivot_offset > (size - size / 8);

            // If we got a highly unbalanced partition we shuffle elements to break many patterns.
            if (highly_unbalanced) {
                // If we had too many bad partitions, switch to heapsort to guarantee O(n log n).
                if (--bad_allowed == 0) {
                    std::__make_heap(begin, end, comp);
                    std::__sort_heap(begin, end, comp);
                    return;
                }

                diff_t partition_size = pivot_pos - begin;
                if (partition_size >= insertion_sort_threshold) {
                    std::iter_swap(begin, begin + partition_size / 4);
                    std::iter_swap(pivot_pos - 1, pivot_pos - partition_size / 4);
                }
                
                partition_size = end - pivot_pos;
                if (partition_size >= insertion_sort_threshold) {
                    std::iter_swap(pivot_pos + 1, pivot_pos + partition_size / 4);
                    std::iter_swap(end - 1, end - partition_size / 4);
                }
            } else {
                // If we were decently balanced and we tried to sort an already partitioned
                // sequence try to use insertion sort.
                if (already_partitioned && partial_insertion_sort(begin, pivot_pos, comp, __gnu_cxx::__ops::__val_comp_iter(comp))
                                        && partial_insertion_sort(pivot_pos + 1, end, comp, __gnu_cxx::__ops::__val_comp_iter(comp))) return;
            }
                
            // Sort the left partition first using recursion and do tail recursion elimination for
            // the right-hand partition.
            pdqsort_loop(begin, pivot_pos, comp, bad_allowed, leftmost);
            begin = pivot_pos + 1;
            leftmost = false;
        }
    }
}


template<class Iter, class Compare>
inline void pdqsort(Iter begin, Iter end, Compare comp) {
    if (begin == end) return;
    pdqsort_detail::pdqsort_loop(begin, end,
                                 __gnu_cxx::__ops::__iter_comp_iter(comp),
                                 std::__lg(end - begin));
}


template<class Iter>
inline void pdqsort(Iter begin, Iter end) {
    if (begin == end) return;
    pdqsort_detail::pdqsort_loop(begin, end,
                                 __gnu_cxx::__ops::__iter_less_iter(),
                                 std::__lg(end - begin));
}


#endif
