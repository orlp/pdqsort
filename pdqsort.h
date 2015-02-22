#ifndef PDQSORT_H
#define PDQSORT_H

#include <utility>
#include <algorithm>

#if __cplusplus >= 201103L
    #define PREFER_MOVE(x) std::move(x)
#else
    #define PREFER_MOVE(x) (x)
#endif


namespace detail {
    using std::swap;

    // Constants used to tune the sorting algorithm.
    enum { insertion_sort_threshold = 24 };

    // Returns floor(log2(n)), assumes n > 0.
    template<class T>
    inline int log2(T n) {
        int log = 0;
        while (n >>= 1) ++log;
        return log;
    }

    // Sorts [begin, end) using insertion sort with the given comparison function.
    template<class Iter, class Compare>
    inline void insertion_sort(Iter begin, Iter end, Compare comp) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        if (begin == end) return;

        for (Iter cur = begin + 1; cur != end; ++cur) {
            Iter sift = cur;
            Iter sift_1 = cur - 1;

            // Compare first so we can elimite 2 moves for an element already positioned correctly.
            if (comp(*sift, *sift_1)) {
                T tmp = PREFER_MOVE(*cur);

                do { *sift-- = PREFER_MOVE(*sift_1--); }
                while (sift != begin && comp(tmp, *sift_1));

                *sift = PREFER_MOVE(tmp);
            }
        }
    }

    // Sorts [begin, end) using insertion sort with the given comparison function. Assumes
    // *(begin - 1) is an element smaller than or equal to any element in [begin, end).
    template<class Iter, class Compare>
    inline void unguarded_insertion_sort(Iter begin, Iter end, Compare comp) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        if (begin == end) return;

        for (Iter cur = begin + 1; cur != end; ++cur) {
            Iter sift = cur;
            Iter sift_1 = cur - 1;

            // Compare first so we can elimite 2 moves for an element already positioned correctly.
            if (comp(*sift, *sift_1)) {
                T tmp = PREFER_MOVE(*cur);

                do { *sift-- = PREFER_MOVE(*sift_1--); }
                while (comp(tmp, *sift_1));

                *sift = PREFER_MOVE(tmp);
            }
        }
    }

    // TODO: implement binary search
    template<class Iter, class Compare>
    inline bool partial_insertion_sort(Iter begin, Iter end, Compare comp, int limit) {
        if (begin == end) return true; // TODO: this check is probably unnecessary

        for (Iter cur = begin + 1; cur != end; ++cur) {
            if (limit <= 0) return false;

            if (comp(*cur, *(cur - 1))) {
                typename std::iterator_traits<Iter>::value_type tmp(PREFER_MOVE(*cur));

                *cur = PREFER_MOVE(*(cur - 1));
                Iter sift = cur - 1;

                while (sift != begin && comp(tmp, *(sift - 1))) {
                    *sift = PREFER_MOVE(*(sift - 1));
                    --sift; --limit;
                }

                *sift = PREFER_MOVE(tmp);
            }
        }

        return true;
    }

    // Sorts the elements *a, *b and *c using comparison function comp.
    template<class Iter, class Compare>
    inline void sort3(Iter a, Iter b, Iter c, Compare comp) {
        if (!comp(*b, *a)) {
            if (!comp(*c, *b)) return;

            swap(*b, *c);
            if (comp(*b, *a)) swap(*a, *b);

            return;
        }

        if (comp(*c, *b)) {
            swap(*a, *c);
            return;
        }

        swap(*a, *b);
        if (comp(*c, *b)) swap(*b, *c);
    }

    // Partitions [begin, end) around pivot *begin using comparison function comp. Elements equal
    // to the pivot are put in the right-hand partition. Returns the position of the pivot after
    // partitioning and whether the passed sequence already was correctly partitioned. Assumes the
    // pivot is a median of at least 3 elements and that [begin, end) is at least
    // insertion_sort_threshold long.
    template<class Iter, class Compare>
    inline std::pair<Iter, bool> partition_right(Iter begin, Iter end, Compare comp) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        
        // Move pivot into local for speed.
        T pivot(PREFER_MOVE(*begin));

        Iter first = begin;
        Iter last = end;

        // Find the first element greater than or equal than the pivot (the median of 3 guarantees
        // this exists).
        while (comp(*++first, pivot));

        // Find the first element strictly smaller than the pivot. We have to guard this search if
        // there was no element before *first because we moved the pivot to a local variable.
        if (first - 1 == begin) while (first < last && !comp(*--last, pivot));
        else                    while (                !comp(*--last, pivot));

        // If the first pair of elements that should be swapped to partition are the same element,
        // the passed in sequence already was correctly partitioned.
        bool already_partitioned = first >= last;
        
        // Keep swapping pairs of elements that are on the wrong side of the pivot. Previously
        // swapped pairs guard the searches, which is why the first iteration is special-cased
        // above.
        while (first < last) {
            swap(*first, *last);
            while (comp(*++first, pivot));
            while (!comp(*--last, pivot));
        }

        // Put the pivot in the right place.
        Iter pivot_pos = first - 1;
        *begin = PREFER_MOVE(*pivot_pos);
        *pivot_pos = PREFER_MOVE(pivot);

        return std::make_pair(pivot_pos, already_partitioned);
    }

    // Similar function to the one above, except elements equal to the pivot are put to the left of
    // the pivot and it doesn't check or return if the passed sequence already was partitioned.
    template<class Iter, class Compare>
    inline Iter partition_left(Iter begin, Iter end, Compare comp) {
        typedef typename std::iterator_traits<Iter>::value_type T;

        T pivot(PREFER_MOVE(*begin));
        Iter first = begin;
        Iter last = end;
        
        while (comp(pivot, *--last));

        if (last + 1 == end) while (first < last && !comp(pivot, *++first));
        else                 while (                !comp(pivot, *++first));

        while (first < last) {
            swap(*first, *last);
            while (comp(pivot, *--last));
            while (!comp(pivot, *++first));
        }

        Iter pivot_pos = last;
        *begin = PREFER_MOVE(*pivot_pos);
        *pivot_pos = PREFER_MOVE(pivot);

        return pivot_pos;
    }


    template<class Iter, class Compare>
    inline void pdqsort_loop(Iter begin, Iter end, Compare comp, int depth,
                            double perc = 0.5, bool leftmost = true) {
        typedef typename std::iterator_traits<Iter>::value_type T;
        typedef typename std::iterator_traits<Iter>::difference_type diff_t;

        // Use a while loop for tail recursion elimination.
        while (depth) {
            diff_t size = end - begin;

            // Insertion sort is faster for small arrays.
            if (size < insertion_sort_threshold) {
                if (leftmost) insertion_sort(begin, end, comp);
                else unguarded_insertion_sort(begin, end, comp);
                return;
            }

            // Choose pivot as median of 3.
            sort3(begin + size_t((size - 1) * perc), begin, end - 1, comp);

            // If *(begin - 1) is the end of the right partion of a previous partition operation
            // there is no element in [*begin, end) that is smaller than *(begin - 1). Then if our
            // pivot compares equal to *(begin - 1) we change strategy, putting equal elements in
            // the left partition, greater elements in the right partition. We do not have to
            // recurse on the left partition, since it's sorted (all equal).
            if (!leftmost && !comp(*(begin - 1), *begin)) {
                begin = partition_left(begin, end, comp) + 1;
                continue;
            }

            // Partition and get results.
            std::pair<Iter, bool> part_result = partition_right(begin, end, comp);
            Iter pivot_pos = part_result.first;
            bool already_partitioned = part_result.second;

            double pivot_perc = double(pivot_pos - begin) / size;
            bool highly_unbalanced = pivot_perc < 0.125 || pivot_perc > 0.875;

            // Compute where to look for the next pivot, wrapping [0, 1].
            perc += 1.61803398875 * (pivot_perc - 0.5);
            perc -= perc > 1; perc += perc < 0;

            // If we got a highly unbalanced partition we reduce the counter that determines the
            // maximum depth. Then we also shuffle some elements to break many patterns.
            if (highly_unbalanced) {
                --depth;

                diff_t size = pivot_pos - begin;
                if (size >= insertion_sort_threshold) {
                    swap(*begin, *(begin + size / 4));
                    swap(*(pivot_pos - 1), *(pivot_pos - size / 4));
                }
                
                size = end - pivot_pos;
                if (size >= insertion_sort_threshold) {
                    swap(*(pivot_pos + 1), *(pivot_pos + size / 4));
                    swap(*(end - 1), *(end - size / 4));
                }
            } else {
                // If we were decently balanced and we tried to sort an already partitioned
                // sequence try to use insertion sort.
                if (already_partitioned && 
                    partial_insertion_sort(begin, pivot_pos, comp, 8) &&
                    partial_insertion_sort(pivot_pos + 1, end, comp, 8)) return;
            }
                
            // Do the left partition first using recursion and do tail recursion elimination for
            // the right-hand partition.
            pdqsort_loop(begin, pivot_pos, comp, depth, perc, leftmost);
            begin = pivot_pos + 1;
            leftmost = false;
        }

        std::make_heap(begin, end, comp);
        std::sort_heap(begin, end, comp);
    }
}


template<class Iter, class Compare>
inline void pdqsort(Iter begin, Iter end, Compare comp) {
    if (begin == end) return;
    detail::pdqsort_loop(begin, end, comp, detail::log2(end - begin));
}


template<class Iter>
inline void pdqsort(Iter begin, Iter end) {
    typedef typename std::iterator_traits<Iter>::value_type T;
    pdqsort(begin, end, std::less<T>());
}


#undef PREFER_MOVE

#endif
