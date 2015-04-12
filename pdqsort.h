namespace std
{
  // When we detect an already sorted partition, attempt an insertion sort that
  // allows this amount of element moves before giving up.
  enum { _S_partial_insertion_sort = 8 };

  // Sorts [__first, __last) using insertion sort with the given comparison
  // function.
  template<typename _RandomAccessIterator, typename _Compare>
    inline void
    __pdq_insertion_sort(_RandomAccessIterator __first,
			 _RandomAccessIterator __last,
			 _Compare __comp)
    {
      typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      if (__first == __last) return;

      for (_RandomAccessIterator __cur = __first + 1; __cur != __last; ++__cur)
	{
	  _RandomAccessIterator __sift = __cur;
	  _RandomAccessIterator __sift_1 = __cur - 1;

	  // Compare first so we can elimite 2 moves for an element already
	  // positioned correctly.
	  if (__comp(__sift, __sift_1))
	    {
	      _ValueType __tmp = _GLIBCXX_MOVE(*__sift);

	      do { *__sift-- = _GLIBCXX_MOVE(*__sift_1); }
	      while (__sift != __first &&
		     __comp(std::__addressof(__tmp), --__sift_1));

	      *__sift = _GLIBCXX_MOVE(__tmp);
	    }
	}
    }

  // Sorts [__first, __last) using insertion sort with the given comparison
  // function. Assumes *(__first - 1) is an element smaller than or equal to any
  // element in [__first, __last).
  template<typename _RandomAccessIterator, typename _Compare>
    inline void
    __pdq_unguarded_insertion_sort(_RandomAccessIterator __first,
				   _RandomAccessIterator __last,
				   _Compare __comp)
    {
      typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      if (__first == __last) return;

      for (_RandomAccessIterator __cur = __first + 1; __cur != __last; ++__cur)
	{
	  _RandomAccessIterator __sift = __cur;
	  _RandomAccessIterator __sift_1 = __cur - 1;

	  // Compare first so we can elimite 2 moves for an element already
	  // positioned correctly.
	  if (__comp(__sift, __sift_1))
	    {
	      _ValueType __tmp = _GLIBCXX_MOVE(*__sift);

	      do { *__sift-- = _GLIBCXX_MOVE(*__sift_1); }
	      while (__comp(std::__addressof(__tmp), --__sift_1));

	      *__sift = _GLIBCXX_MOVE(__tmp);
	    }
	}
    }

  // Attempts to use insertion sort on [__first, __last). Will return false if
  // more than _S_partial_insertion_sort elements were moved, and abort sorting.
  // Otherwise it will succesfully sort and return true.
  template<typename _RandomAccessIterator, typename _Compare>
    inline bool
    __partial_insertion_sort(_RandomAccessIterator __first,
			     _RandomAccessIterator __last,
			     _Compare __comp)
    {
      typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      if (__first == __last) return true;
      
      int __limit = 0;
      for (_RandomAccessIterator __cur = __first + 1; __cur != __last; ++__cur)
	{
	  if (__limit > _S_partial_insertion_sort) return false;

	  _RandomAccessIterator __sift = __cur;
	  _RandomAccessIterator __sift_1 = __cur - 1;

	  // Compare first so we can elimite 2 moves for an element already
	  // positioned correctly.
	  if (__comp(__sift, __sift_1))
	    {
	      _ValueType __tmp = _GLIBCXX_MOVE(*__sift);

	      do { *__sift-- = _GLIBCXX_MOVE(*__sift_1); }
	      while (__sift != __first &&
		     __comp(std::__addressof(__tmp), --__sift_1));

	      *__sift = _GLIBCXX_MOVE(__tmp);
	      __limit += __cur - __sift;
	    }
	}

      return true;
    }

// Sorts the elements *__a, *__b and *__c using comparison function __comp.
  template<typename _RandomAccessIterator, typename _Compare>
    inline void
    __sort3(_RandomAccessIterator __a,
	    _RandomAccessIterator __b,
	    _RandomAccessIterator __c,
	    _Compare __comp)
    {
      if (!__comp(__b, __a))
	{
	  if (!__comp(__c, __b)) return;

	  std::iter_swap(__b, __c);
	  if (__comp(__b, __a)) std::iter_swap(__a, __b);

	  return;
	}

      if (__comp(__c, __b))
	{
	  std::iter_swap(__a, __c);
	  return;
	}

      std::iter_swap(__a, __b);
      if (__comp(__c, __b)) std::iter_swap(__b, __c);
    }

  // Partitions [__first, __last) around pivot *__first using comparison
  // function __comp. Elements equal to the pivot are put in the right-hand
  // partition. Returns the position of the pivot after partitioning and whether
  // the passed sequence already was correctly partitioned. Assumes the pivot is
  // a median of at least 3 elements and that [__first, __last) is at least
  // _S_insertion_sort long.
  template<typename _RandomAccessIterator, typename _Compare>
    inline std::pair<_RandomAccessIterator, bool>
    __partition_right(_RandomAccessIterator __first,
		      _RandomAccessIterator __last,
		      _Compare __comp)
    {
      typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      
      // Move pivot into local for speed.
      _ValueType __pivot(_GLIBCXX_MOVE(*__first));

      _RandomAccessIterator __left = __first;
      _RandomAccessIterator __right = __last;

      // Find the first element greater than or equal than the pivot (the median
      // of 3 guarantees this exists).
      while (__comp(++__left, std::__addressof(__pivot)));

      // Find the first element strictly smaller than the pivot. We have to
      // guard this search if there was no element before *__left.
      if (__left - 1 == __first)
	while (__left < __right &&
	       !__comp(--__right, std::__addressof(__pivot)));
      else
	while (!__comp(--__right, std::__addressof(__pivot)));

      // If the first pair of elements that should be swapped to partition are
      // the same element, the passed in sequence already was correctly
      // partitioned.
      bool __already_partitioned = __left >= __right;
      
      // Keep swapping pairs of elements that are on the wrong side of the
      // pivot. Previously swapped pairs guard the searches, which is why the
      // first iteration is special-cased above.
      while (__left < __right)
	{
	  std::iter_swap(__left, __right);
	  while (__comp(++__left, std::__addressof(__pivot)));
	  while (!__comp(--__right, std::__addressof(__pivot)));
	}

      // Put the pivot in the right place.
      _RandomAccessIterator __pivot_pos = __left - 1;
      *__first = _GLIBCXX_MOVE(*__pivot_pos);
      *__pivot_pos = _GLIBCXX_MOVE(__pivot);
      return std::make_pair(__pivot_pos, __already_partitioned);
    }

  // Similar function to the one above, except elements equal to the pivot are
  // put to the left of the pivot and it doesn't check or return if the passed
  // sequence already was partitioned.
  template<typename _RandomAccessIterator, typename _Compare>
    inline _RandomAccessIterator
    __partition_left(_RandomAccessIterator __first,
		     _RandomAccessIterator __last,
		     _Compare __comp)
    {
      typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;

      _ValueType __pivot(_GLIBCXX_MOVE(*__first));
      _RandomAccessIterator __left = __first;
      _RandomAccessIterator __right = __last;
      
      while (__comp(std::__addressof(__pivot), --__right));

      if (__right + 1 == __last)
	while (__left < __right &&
	       !__comp(std::__addressof(__pivot), ++__left));
      else
	while (!__comp(std::__addressof(__pivot), ++__left));

      while (__left < __right)
	{
	  std::iter_swap(__left, __right);
	  while (__comp(std::__addressof(__pivot), --__right));
	  while (!__comp(std::__addressof(__pivot), ++__left));
	}

      _RandomAccessIterator __pivot_pos = __right;
      *__first = _GLIBCXX_MOVE(*__pivot_pos);
      *__pivot_pos = _GLIBCXX_MOVE(__pivot);
      return __pivot_pos;
    }


  template<typename _RandomAccessIterator, typename _Compare>
    inline void
    __pdqsort_loop(_RandomAccessIterator __first,
		   _RandomAccessIterator __last,
		   _Compare __comp,
		   int __bad_allowed, bool __leftmost = true)
    {
      typedef typename std::iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      typedef
	typename std::iterator_traits<_RandomAccessIterator>::difference_type
	_DistanceType;
      const _DistanceType _S_insertion_sort = 16 + 8*__is_pod(_ValueType);

      // Use a while loop for tail recursion elimination.
      while (true)
	{
	  _DistanceType __size = __last - __first;

	  // Insertion sort is faster for small arrays.
	  if (__size < _S_insertion_sort)
	    {
	      if (__leftmost) __pdq_insertion_sort(__first, __last, __comp);
	      else __pdq_unguarded_insertion_sort(__first, __last, __comp);
	      return;
	    }

	  // Choose pivot as median of 3.
	  __sort3(__first + __size / 2, __first, __last - 1, __comp);

	  // If *(__first - 1) is the end of the right partition of a previous
	  // partition operation there is no element in [*__first, __last) that
	  // is smaller than *(__first - 1). Then if our pivot compares equal
	  // to *(__first - 1) we change strategy, putting equal elements in
	  // the left partition, greater elements in the right partition. We do
	  // not have to recurse on the left partition, since it's sorted (all
	  // equal).
	  if (!__leftmost && !__comp(__first - 1, __first))
	    {
	      __first = __partition_left(__first, __last, __comp) + 1;
	      continue;
	    }

	  // Partition and get results.
	  std::pair<_RandomAccessIterator, bool> __part_result =
	    __partition_right(__first, __last, __comp);
	  _RandomAccessIterator __pivot_pos = __part_result.first;
	  bool __already_partitioned = __part_result.second;

	  // Check for a highly unbalanced partition.
	  _DistanceType __pivot_offset = __pivot_pos - __first;
	  bool __highly_unbalanced = __pivot_offset < __size / 8 ||
				     __pivot_offset > (__size - __size / 8);

	  // If we got a highly unbalanced partition we shuffle elements to
	  // break many patterns.
	  if (__highly_unbalanced)
	    {
	      // If we had too many bad partitions, switch to heapsort to
	      // guarantee O(n log n).
	      if (--__bad_allowed == 0)
		{
		  std::__make_heap(__first, __last, __comp);
		  std::__sort_heap(__first, __last, __comp);
		  return;
		}

	      _DistanceType __partition_size = __pivot_pos - __first;
	      if (__partition_size >= _S_insertion_sort)
		{
		  std::iter_swap(__first,
				 __first + __partition_size / 4);
		  std::iter_swap(__pivot_pos - 1,
				 __pivot_pos - __partition_size / 4);
		}
	      
	      __partition_size = __last - __pivot_pos;
	      if (__partition_size >= _S_insertion_sort)
		{
		  std::iter_swap(__pivot_pos + 1,
				 __pivot_pos + __partition_size / 4);
		  std::iter_swap(__last - 1,
				 __last - __partition_size / 4);
		}
	    }
	  else
	    {
	      // If we were decently balanced and we tried to sort an already
	      // partitioned sequence try to use insertion sort.
	      if (__already_partitioned &&
		  __partial_insertion_sort(__first, __pivot_pos, __comp) &&
		  __partial_insertion_sort(__pivot_pos + 1, __last, __comp))
		return;
	    }
	      
	  // Sort the left partition first using recursion and do tail
	  // recursion elimination for the right-hand partition.
	  __pdqsort_loop(__first, __pivot_pos, __comp,
			 __bad_allowed, __leftmost);
	  __first = __pivot_pos + 1;
	  __leftmost = false;
	}
    }


  template<typename _RandomAccessIterator>
    inline void
    pdqsort(_RandomAccessIterator __first, _RandomAccessIterator __last)
    {
      if (__first == __last) return;
      __pdqsort_loop(__first, __last,
		     __gnu_cxx::__ops::__iter_less_iter(),
		     std::__lg(__last - __first));
    }


  template<typename _RandomAccessIterator, typename _Compare>
    inline void
    pdqsort(_RandomAccessIterator __first,
	    _RandomAccessIterator __last,
	    _Compare __comp)
    {
      if (__first == __last) return;
      __pdqsort_loop(__first, __last,
		     __gnu_cxx::__ops::__iter_comp_iter(__comp),
		     std::__lg(__last - __first));
    }
} // namespace std
