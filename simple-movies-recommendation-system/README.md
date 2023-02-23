# Simple Movies Recommendation System

## Tech Stack

- fork
- mmap shared memory
- memcpy
- pthread_create, pthread_join

## Description

In this project, I implement a simple movies recommendation system.
There are two main tasks should be handled.

1. Each request contains a keyword and an user profile, the system should
    1. filter the movies by keywords
    2. do sorting via recommendation scores.
2. Handle multiple search requests simultaneously.

The dataset I used is [MovieLens](https://grouplens.org/datasets/movielens/),
you can go to the page if you want to get more information about it.

## Task 1: Filtering and Sorting

Each request contains a keyword, I first need to filter the movies containing the keyword.
Then I calculate recommendation scores for the filtered movies.

After calculating recommendation scores, I'm going to sort the movies by the scores.

I divide the list of the movies into several parts, delegate each part to a thread,
use the sorting library to sort each part, then merge them to get the final result.

There are two kinds of my merge sort:

1. multithread: tserver
2. multiprocess: pserver

The figure below shows the details of the sorting (with depth = 3).

![merge sort](https://i.imgur.com/RYEFMrq.png)

## Task 2: Handle Multiple Requests Simultaneously

In some test cases, they will include more than one request,
so I use multithread to handle multiple requests simultaneously.
It might not be possible to expect requests to be processed sequentially within the time limit (5s).

## Sample Execution

My `Makefile` would produce executable file `tserver` for using multithread to do merge sort,
and `pserver` for using multiprocess to do merge sort.

```
make 
./pserver < /path/to/input/data  
./tserver < /path/to/input/data
```

Remember `make clean` to remove all {}.out and executable file.

## Reference

[SPEC Link](https://hackmd.io/@claire2222/SkLyBSnIo)
