#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>

#define MAX_DEPTH 5

// error handling
#define ERR_EXIT(a) do {perror(a);exit(1);} while(0)

// max number of movies
#define MAX_MOVIES 62500

// max length of each movies
#define MAX_LEN 255

// number of requests
#define MAX_REQ 255

// number of genre
#define NUM_OF_GENRE 19

// number of threads
#define MAX_THREAD 256

void sort(char **movies, double *pts, int size);

typedef struct request
{
    int id;
    char *keywords;
    double *profile;
} request;

typedef struct movie_profile
{
    int movieId;
    char *title;
    double *profile;
} movie_profile;

typedef struct recommend
{
    char *titles[MAX_MOVIES];
    double scores[MAX_MOVIES];
    int size;
} recommend;

typedef struct parameter
{
    int depth;
    int left;
    int right;
    recommend *rcmd;
} parameter;