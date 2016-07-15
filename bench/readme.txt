This is the benchmark I used to test pdqsort and co. bench.cpp just spits out
the raw cycle counts, I use Python for post-processing, such as making a bar
graph of the median cycle count.

Example:

    g++ -std=c++11 -O2 -m64 -march=native bench.cpp
    ./a.out > profiles/pdqsort.txt
    python3 bars.py "i5-4670k @ 3.4GHz"