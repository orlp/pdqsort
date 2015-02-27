/*
 * C++ implementation of timsort
 *
 * ported from Python's and OpenJDK's:
 * - http://svn.python.org/projects/python/trunk/Objects/listobject.c
 * - http://cr.openjdk.java.net/~martin/webrevs/openjdk7/timsort/raw_files/new/src/share/classes/java/util/TimSort.java
 *
 * Copyright (c) 2011 Fuji, Goro (gfx) <gfuji@cpan.org>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef GFX_TIMSORT_HPP
#define GFX_TIMSORT_HPP

#include <vector>
#include <cassert>
#include <iterator>
#include <algorithm>
#include <utility>

#ifdef ENABLE_TIMSORT_LOG
#include <iostream>
#define GFX_TIMSORT_LOG(expr) (std::clog << "# " << __func__ << ": " << expr << std::endl)
#else
#define GFX_TIMSORT_LOG(expr) ((void)0)
#endif

#if ENABLE_STD_MOVE && __cplusplus >= 201103L
#define GFX_TIMSORT_MOVE(x) std::move(x)
#else
#define GFX_TIMSORT_MOVE(x) (x)
#endif

namespace gfx {

// ---------------------------------------
// Declaration
// ---------------------------------------

/**
 * Same as std::stable_sort(first, last).
 */
template<typename RandomAccessIterator>
inline void timsort(RandomAccessIterator const first, RandomAccessIterator const last);

/**
 * Same as std::stable_sort(first, last, c).
 */
template<typename RandomAccessIterator, typename LessFunction>
inline void timsort(RandomAccessIterator const first, RandomAccessIterator const last, LessFunction compare);


// ---------------------------------------
// Implementation
// ---------------------------------------

template <typename Value, typename LessFunction> class Compare {
    public:
        typedef Value        value_type;
        typedef LessFunction func_type;

        Compare(LessFunction f)
            : less_(f) { }
        Compare(const Compare<value_type, func_type>& other)
            : less_(other.less_) { }

        bool lt(value_type x, value_type y) {
            return less_(x, y);
        }
        bool le(value_type x, value_type y) {
            return less_(x, y) || !less_(y, x);
        }
        bool gt(value_type x, value_type y) {
            return !less_(x, y) && less_(y, x);
        }
        bool ge(value_type x, value_type y) {
            return !less_(x, y);
        }

        func_type& less_function() {
            return less_;
        }
    private:
        func_type less_;
};

template <typename RandomAccessIterator, typename LessFunction>
class TimSort {
    typedef RandomAccessIterator iter_t;
    typedef typename std::iterator_traits<iter_t>::value_type value_t;
    typedef typename std::iterator_traits<iter_t>::reference ref_t;
    typedef typename std::iterator_traits<iter_t>::difference_type diff_t;
    typedef Compare<const value_t&, LessFunction> compare_t;

    static const int MIN_MERGE = 32;

    compare_t comp_;

    static const int MIN_GALLOP = 7;

    int minGallop_; // default to MIN_GALLOP

    std::vector<value_t> tmp_; // temp storage for merges
    typedef typename std::vector<value_t>::iterator tmp_iter_t;

    struct run {
        iter_t base;
        diff_t len;

        run(iter_t const b, diff_t const l) : base(b), len(l) { }
    };
    std::vector<run> pending_;

    static
    void sort(iter_t const lo, iter_t const hi, compare_t c) {
        assert( lo <= hi );

        diff_t nRemaining = (hi - lo);
        if(nRemaining < 2) {
            return; // nothing to do
        }

        if(nRemaining < MIN_MERGE) {
            diff_t const initRunLen = countRunAndMakeAscending(lo, hi, c);
            GFX_TIMSORT_LOG("initRunLen: " << initRunLen);
            binarySort(lo, hi, lo + initRunLen, c);
            return;
        }

        TimSort ts(c);
        diff_t const minRun = minRunLength(nRemaining);
        iter_t cur          = lo;
        do {
            diff_t runLen = countRunAndMakeAscending(cur, hi, c);

            if(runLen < minRun) {
                diff_t const force  = std::min(nRemaining, minRun);
                binarySort(cur, cur + force, cur + runLen, c);
                runLen = force;
            }

            ts.pushRun(cur, runLen);
            ts.mergeCollapse();

            cur        += runLen;
            nRemaining -= runLen;
        } while(nRemaining != 0);

        assert( cur == hi );
        ts.mergeForceCollapse();
        assert( ts.pending_.size() == 1 );

        GFX_TIMSORT_LOG("size: " << (hi - lo) << " tmp_.size(): " << ts.tmp_.size() << " pending_.size(): " << ts.pending_.size());
    } // sort()

    static
    void binarySort(iter_t const lo, iter_t const hi, iter_t start, compare_t compare) {
        assert( lo <= start && start <= hi );
        if(start == lo) {
            ++start;
        }
        for( ; start < hi; ++start ) {
            assert(lo <= start);
            /*const*/ value_t pivot = GFX_TIMSORT_MOVE(*start);

            iter_t const pos = std::upper_bound(lo, start, pivot, compare.less_function());
            for(iter_t p = start; p > pos; --p) {
                *p = GFX_TIMSORT_MOVE(*(p - 1));
            }
            *pos = GFX_TIMSORT_MOVE(pivot);
        }
    }

    static
    diff_t countRunAndMakeAscending(iter_t const lo, iter_t const hi, compare_t compare) {
        assert( lo < hi );

        iter_t runHi = lo + 1;
        if( runHi == hi ) {
            return 1;
        }

        if(compare.lt(*(runHi++), *lo)) { // descending
            while(runHi < hi && compare.lt(*runHi, *(runHi - 1))) {
                ++runHi;
            }
            std::reverse(lo, runHi);
        }
        else { // ascending
            while(runHi < hi && compare.ge(*runHi, *(runHi - 1))) {
                ++runHi;
            }
        }

        return runHi - lo;
    }

    static
    diff_t minRunLength(diff_t n) {
        assert( n >= 0 );

        diff_t r = 0;
        while(n >= MIN_MERGE) {
            r |= (n & 1);
            n >>= 1;
        }
        return n + r;
    }

    TimSort(compare_t c)
            : comp_(c), minGallop_(MIN_GALLOP) {
    }

    void pushRun(iter_t const runBase, diff_t const runLen) {
        pending_.push_back(run(runBase, runLen));
    }

    void mergeCollapse() {
        while( pending_.size() > 1 ) {
            diff_t n = pending_.size() - 2;

            if(n > 0 && pending_[n - 1].len <= pending_[n].len + pending_[n + 1].len) {
                if(pending_[n - 1].len < pending_[n + 1].len) {
                    --n;
                }
                mergeAt(n);
            }
            else if(pending_[n].len <= pending_[n + 1].len) {
                mergeAt(n);
            }
            else {
                break;
            }
        }
    }

    void mergeForceCollapse() {
        while( pending_.size() > 1 ) {
            diff_t n = pending_.size() - 2;

            if(n > 0 && pending_[n - 1].len < pending_[n + 1].len) {
                --n;
            }
            mergeAt(n);
        }
    }

    void mergeAt(diff_t const i) {
        diff_t const stackSize = pending_.size();
        assert( stackSize >= 2 );
        assert( i >= 0 );
        assert( i == stackSize - 2 || i == stackSize - 3 );

        iter_t base1 = pending_[i].base;
        diff_t len1  = pending_[i].len;
        iter_t base2 = pending_[i + 1].base;
        diff_t len2  = pending_[i + 1].len;

        assert( len1 > 0 && len2 > 0 );
        assert( base1 + len1 == base2 );

        pending_[i].len = len1 + len2;

        if(i == stackSize - 3) {
            pending_[i + 1] = pending_[i + 2];
        }

        pending_.pop_back();

        diff_t const k = gallopRight(*base2, base1, len1, 0);
        assert( k >= 0 );

        base1 += k;
        len1  -= k;

        if(len1 == 0) {
            return;
        }

        len2 = gallopLeft(*(base1 + (len1 - 1)), base2, len2, len2 - 1);
        assert( len2 >= 0 );
        if(len2 == 0) {
            return;
        }

        if(len1 <= len2) {
            mergeLo(base1, len1, base2, len2);
        }
        else {
            mergeHi(base1, len1, base2, len2);
        }
    }

    template <typename Iter>
    diff_t gallopLeft(ref_t key, Iter const base, diff_t const len, diff_t const hint) {
        assert( len > 0 && hint >= 0 && hint < len );

        diff_t lastOfs = 0;
        diff_t ofs = 1;

        if(comp_.gt(key, *(base + hint))) {
            diff_t const maxOfs = len - hint;
            while(ofs < maxOfs && comp_.gt(key, *(base + (hint + ofs)))) {
                lastOfs = ofs;
                ofs     = (ofs << 1) + 1;

                if(ofs <= 0) { // int overflow
                    ofs = maxOfs;
                }
            }
            if(ofs > maxOfs) {
                ofs = maxOfs;
            }

            lastOfs += hint;
            ofs     += hint;
        }
        else {
            diff_t const maxOfs = hint + 1;
            while(ofs < maxOfs && comp_.le(key, *(base + (hint - ofs)))) {
                lastOfs = ofs;
                ofs     = (ofs << 1) + 1;

                if(ofs <= 0) {
                    ofs = maxOfs;
                }
            }
            if(ofs > maxOfs) {
                ofs = maxOfs;
            }

            diff_t const tmp = lastOfs;
            lastOfs          = hint - ofs;
            ofs              = hint - tmp;
        }
        assert( -1 <= lastOfs && lastOfs < ofs && ofs <= len );

        return std::lower_bound(base+(lastOfs+1), base+ofs, key, comp_.less_function()) - base;
    }

    template <typename Iter>
    diff_t gallopRight(ref_t key, Iter const base, diff_t const len, diff_t const hint) {
        assert( len > 0 && hint >= 0 && hint < len );

        diff_t ofs = 1;
        diff_t lastOfs = 0;

        if(comp_.lt(key, *(base + hint))) {
            diff_t const maxOfs = hint + 1;
            while(ofs < maxOfs && comp_.lt(key, *(base + (hint - ofs)))) {
                lastOfs = ofs;
                ofs     = (ofs << 1) + 1;

                if(ofs <= 0) {
                    ofs = maxOfs;
                }
            }
            if(ofs > maxOfs) {
                ofs = maxOfs;
            }

            diff_t const tmp = lastOfs;
            lastOfs          = hint - ofs;
            ofs              = hint - tmp;
        }
        else {
            diff_t const maxOfs = len - hint;
            while(ofs < maxOfs && comp_.ge(key, *(base + (hint + ofs)))) {
                lastOfs = ofs;
                ofs     = (ofs << 1) + 1;

                if(ofs <= 0) { // int overflow
                    ofs = maxOfs;
                }
            }
            if(ofs > maxOfs) {
                ofs = maxOfs;
            }

            lastOfs += hint;
            ofs     += hint;
        }
        assert( -1 <= lastOfs && lastOfs < ofs && ofs <= len );

        return std::upper_bound(base+(lastOfs+1), base+ofs, key, comp_.less_function()) - base;
    }

    void mergeLo(iter_t const base1, diff_t len1, iter_t const base2, diff_t len2) {
        assert( len1 > 0 && len2 > 0 && base1 + len1 == base2 );

        copy_to_tmp(base1, len1);

        tmp_iter_t cursor1 = tmp_.begin();
        iter_t cursor2     = base2;
        iter_t dest        = base1;

        *(dest++) = *(cursor2++);
        if(--len2 == 0) {
            std::copy(cursor1, cursor1 + len1, dest);
            return;
        }
        if(len1 == 1) {
            std::copy(cursor2, cursor2 + len2, dest);
            *(dest + len2) = *cursor1;
            return;
        }

        int minGallop(minGallop_);

        // outer:
        while(true) {
            int count1 = 0;
            int count2 = 0;

            bool break_outer = false;
            do {
                assert( len1 > 1 && len2 > 0 );

                if(comp_.lt(*cursor2, *cursor1)) {
                    *(dest++) = *(cursor2++);
                    ++count2;
                    count1 = 0;
                    if(--len2 == 0) {
                        break_outer = true;
                        break;
                    }
                }
                else {
                    *(dest++) = *(cursor1++);
                    ++count1;
                    count2 = 0;
                    if(--len1 == 1) {
                        break_outer = true;
                        break;
                    }
                }
            } while( (count1 | count2) < minGallop );
            if(break_outer) {
                break;
            }

            do {
                assert( len1 > 1 && len2 > 0 );

                count1 = gallopRight(*cursor2, cursor1, len1, 0);
                if(count1 != 0) {
                    std::copy_backward(cursor1, cursor1 + count1, dest + count1);
                    dest    += count1;
                    cursor1 += count1;
                    len1    -= count1;

                    if(len1 <= 1) {
                        break_outer = true;
                        break;
                    }
                }
                *(dest++) = *(cursor2++);
                if(--len2 == 0) {
                    break_outer = true;
                    break;
                }

                count2 = gallopLeft(*cursor1, cursor2, len2, 0);
                if(count2 != 0) {
                    std::copy(cursor2, cursor2 + count2, dest);
                    dest    += count2;
                    cursor2 += count2;
                    len2    -= count2;
                    if(len2 == 0) {
                        break_outer = true;
                        break;
                    }
                }
                *(dest++) = *(cursor1++);
                if(--len1 == 1) {
                    break_outer = true;
                    break;
                }

                --minGallop;
            } while( (count1 >= MIN_GALLOP) | (count2 >= MIN_GALLOP) );
            if(break_outer) {
                break;
            }

            if(minGallop < 0) {
                minGallop = 0;
            }
            minGallop += 2;
        } // end of "outer" loop

        minGallop_ = std::min(minGallop, 1);

        if(len1 == 1) {
            assert( len2 > 0 );
            std::copy(cursor2, cursor2 + len2, dest);
            *(dest + len2) = *cursor1;
        }
        else {
            assert( len1 != 0 && "Comparision function violates its general contract");
            assert( len2 == 0 );
            assert( len1 > 1 );
            std::copy(cursor1, cursor1 + len1, dest);
        }
    }

    void mergeHi(iter_t const base1, diff_t len1, iter_t const base2, diff_t len2) {
        assert( len1 > 0 && len2 > 0 && base1 + len1 == base2 );

        copy_to_tmp(base2, len2);

        iter_t cursor1     = base1 + (len1 - 1);
        tmp_iter_t cursor2 = tmp_.begin() + (len2 - 1);
        iter_t dest        = base2 + (len2 - 1);

        *(dest--) = *(cursor1--);
        if(--len1 == 0) {
            std::copy(tmp_.begin(), tmp_.begin() + len2, dest - (len2 - 1));
            return;
        }
        if(len2 == 1) {
            dest    -= len1;
            cursor1 -= len1;
            std::copy_backward(cursor1 + 1, cursor1 + (1 + len1), dest + (1 + len1));
            *dest = *cursor2;
            return;
        }

        int minGallop( minGallop_ );

        // outer:
        while(true) {
            int count1 = 0;
            int count2 = 0;

            bool break_outer = false;
            do {
                assert( len1 > 0 && len2 > 1 );

                if(comp_.lt(*cursor2, *cursor1)) {
                    *(dest--) = *(cursor1--);
                    ++count1;
                    count2 = 0;
                    if(--len1 == 0) {
                        break_outer = true;
                        break;
                    }
                }
                else {
                    *(dest--) = *(cursor2--);
                    ++count2;
                    count1 = 0;
                    if(--len2 == 1) {
                        break_outer = true;
                        break;
                    }
                }
            } while( (count1 | count2) < minGallop );
            if(break_outer) {
                break;
            }

            do {
                assert( len1 > 0 && len2 > 1 );

                count1 = len1 - gallopRight(*cursor2, base1, len1, len1 - 1);
                if(count1 != 0) {
                    dest    -= count1;
                    cursor1 -= count1;
                    len1    -= count1;
                    std::copy_backward(cursor1 + 1, cursor1 + (1 + count1), dest + (1 + count1));

                    if(len1 == 0) {
                        break_outer = true;
                        break;
                    }
                }
                *(dest--) = *(cursor2--);
                if(--len2 == 1) {
                    break_outer = true;
                    break;
                }

                count2 = len2 - gallopLeft(*cursor1, tmp_.begin(), len2, len2 - 1);
                if(count2 != 0) {
                    dest    -= count2;
                    cursor2 -= count2;
                    len2    -= count2;
                    std::copy(cursor2 + 1, cursor2 + (1 + count2), dest + 1);
                    if(len2 <= 1) {
                        break_outer = true;
                        break;
                    }
                }
                *(dest--) = *(cursor1--);
                if(--len1 == 0) {
                    break_outer = true;
                    break;
                }

                minGallop--;
            } while( (count1 >= MIN_GALLOP) | (count2 >= MIN_GALLOP) );
            if(break_outer) {
                break;
            }

            if(minGallop < 0) {
                minGallop = 0;
            }
            minGallop += 2;
        } // end of "outer" loop

        minGallop_ = std::min(minGallop, 1);

        if(len2 == 1) {
            assert( len1 > 0 );
            dest    -= len1;
            cursor1 -= len1;
            std::copy_backward(cursor1 + 1, cursor1 + (1 + len1), dest + (1 + len1));
            *dest = *cursor2;
        }
        else {
            assert( len2 != 0 && "Comparision function violates its general contract");
            assert( len1 == 0 );
            assert( len2 > 1 );
            std::copy(tmp_.begin(), tmp_.begin() + len2, dest - (len2 - 1));
        }
    }

    void copy_to_tmp(iter_t const begin, diff_t const len) {
        tmp_.clear();
        tmp_.reserve(len);
        std::copy(begin, begin + len, std::back_inserter(tmp_));
    }

    // the only interface is the friend timsort() function
    template <typename IterT, typename LessT>
    friend void timsort(IterT first, IterT last, LessT c);
};

template<typename RandomAccessIterator>
inline void timsort(RandomAccessIterator const first, RandomAccessIterator const last) {
    typedef typename std::iterator_traits<RandomAccessIterator>::value_type value_type;
    timsort(first, last, std::less<value_type>());
}

template<typename RandomAccessIterator, typename LessFunction>
inline void timsort(RandomAccessIterator const first, RandomAccessIterator const last, LessFunction compare) {
    TimSort<RandomAccessIterator, LessFunction>::sort(first, last, compare);
}

} // namespace gfx

#undef GFX_TIMSORT_LOG
#undef GFX_TIMSORT_MOVE
#endif // GFX_TIMSORT_HPP
