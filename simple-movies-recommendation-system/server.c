#include "header.h"

movie_profile *movies[MAX_MOVIES];
unsigned int num_of_movies = 0;
unsigned int num_of_reqs = 0;
unsigned int num_of_threads = 0;

request *reqs[MAX_REQ];

pthread_t tids[MAX_THREAD];

void initialize(FILE *fp);

request *read_request();

#ifdef PROCESS
#define ASSIGN(dest, src) memcpy(dest, src, MAX_LEN)
#elif defined THREAD
#define ASSIGN(dest, src) dest = src
#endif

void *create_shared_memory(size_t size)
{
    int protection = PROT_READ | PROT_WRITE;
    int visibility = MAP_SHARED | MAP_ANONYMOUS;
    return mmap(NULL, size, protection, visibility, -1, 0);
}

parameter *genParameter(int depth, int left, int right, recommend *rcmd)
{
    parameter *temp = (parameter *) malloc(sizeof(parameter));
    temp->depth = depth;
    temp->left = left;
    temp->right = right;
    temp->rcmd = rcmd;
    return temp;
}

recommend *filtering(request *rqst)
{
#ifdef PROCESS
    recommend *data = create_shared_memory(sizeof(recommend));
    for (int i = 0; i < MAX_MOVIES; i++)
        data->titles[i] = create_shared_memory(sizeof(char) * MAX_LEN);
#elif defined THREAD
    recommend *data = (recommend *) malloc(sizeof(recommend));
#endif

    bool wildcard = !strcmp(rqst->keywords, "*");
    int cnt = 0;
    for (int i = 0; i < num_of_movies; i++)
    {
        if (wildcard || strstr(movies[i]->title, rqst->keywords) != NULL)
        {
            ASSIGN(data->titles[cnt], movies[i]->title);
            for (int j = 0; j < NUM_OF_GENRE; j++)
                data->scores[cnt] += rqst->profile[j] * movies[i]->profile[j];
            cnt++;
        }
    }
    data->size = cnt;
    return data;
}

bool cmp(int i, int j, recommend *rcmd)
{
    if (rcmd->scores[i] > rcmd->scores[j])
        return true;
    if (rcmd->scores[i] < rcmd->scores[j])
        return false;
    if (strcmp(rcmd->titles[i], rcmd->titles[j]) < 0)
        return true;
    return false;
}

void merge(int left, int mid, int right, recommend *rcmd)
{
    recommend *temp = (recommend *) malloc(sizeof(recommend));
    int i = left, j = mid, k = 0;

    while ((i <= mid - 1) && (j <= right))
    {
        if (cmp(i, j, rcmd))
        {
            temp->titles[k] = rcmd->titles[i];
            temp->scores[k++] = rcmd->scores[i++];
        }
        else
        {
            temp->titles[k] = rcmd->titles[j];
            temp->scores[k++] = rcmd->scores[j++];
        }
    }
    while (i <= mid - 1)
    {
        temp->titles[k] = rcmd->titles[i];
        temp->scores[k++] = rcmd->scores[i++];
    }
    while (j <= right)
    {
        temp->titles[k] = rcmd->titles[j];
        temp->scores[k++] = rcmd->scores[j++];
    }
    memcpy(rcmd->scores + left, temp->scores, sizeof(double) * (right - left + 1));
    memcpy(rcmd->titles + left, temp->titles, sizeof(char *) * (right - left + 1));
    free(temp);
}


void t_merge_sort(parameter *param)
{
    int depth = param->depth;
    int left = param->left;
    int right = param->right;
    recommend *rcmd = param->rcmd;

    if (depth >= MAX_DEPTH || right == left)
        sort(rcmd->titles + left, rcmd->scores + left, right - left + 1);
    else
    {
        int mid = (left + right) / 2;
        parameter *param1 = genParameter(depth + 1, left, mid, rcmd);
        parameter *param2 = genParameter(depth + 1, mid + 1, right, rcmd);
        if (num_of_threads < MAX_THREAD)
        {
            pthread_t tid;
            num_of_threads++;
            pthread_create(&tid, NULL, (void *) t_merge_sort, (void *) param1);
            t_merge_sort(param2);
            pthread_join(tid, NULL);
            num_of_threads--;
        }
        else
        {
            t_merge_sort(param1);
            t_merge_sort(param2);
        }
        merge(left, mid + 1, right, rcmd);
    }
}


void t_handle_request(request *rqst)
{
    recommend *rcmd = filtering(rqst);
    parameter *param = genParameter(0, 0, rcmd->size - 1, rcmd);
    t_merge_sort(param);

    FILE *output;
    char fileName[32];
    sprintf(fileName, "%dt.out", rqst->id);
    if ((output = fopen(fileName, "w+")) == NULL)
        ERR_EXIT("fopen");
    for (int i = 0; i < rcmd->size; i++)
        fprintf(output, "%s\n", rcmd->titles[i]);
    fclose(output);
    free(rcmd);
}

void p_merge_sort(int depth, int left, int right, recommend *rcmd)
{
    int len = right - left + 1;
    if (depth == MAX_DEPTH || right == left)
    {
        char **temp_titles = (char **) malloc(sizeof(char *) * len);
        for (int i = 0; i < len; i++)
        {
            temp_titles[i] = (char *) malloc(sizeof(char) * MAX_LEN);
            memcpy(temp_titles[i], rcmd->titles[i + left], MAX_LEN);
        }
        sort(temp_titles, rcmd->scores + left, right - left + 1);
        for (int i = 0; i < len; i++)
        {
            memcpy(rcmd->titles[i + left], temp_titles[i], MAX_LEN);
            free(temp_titles[i]);
        }
        free(temp_titles);
    }
    else
    {
        int mid = (left + right) / 2;
        pid_t pid = fork();
        if (!pid)
        {
            p_merge_sort(depth + 1, left, mid, rcmd);
            exit(0);
        }
        else
        {
            waitpid(pid, NULL, 0);
            p_merge_sort(depth + 1, mid + 1, right, rcmd);
            merge(left, mid + 1, right, rcmd);
        }
    }
}


int main(int argc, char *argv[])
{
    if (argc != 1)
    {
#ifdef PROCESS
        fprintf(stderr, "usage: ./pserver\n");
#elif defined THREAD
        fprintf(stderr, "usage: ./tserver\n");
#endif
        exit(-1);
    }

    FILE *fp;
    if ((fp = fopen("./data/movies.txt", "r")) == NULL)
        ERR_EXIT("fopen");

    initialize(fp);
    assert(fp != NULL);
    fclose(fp);

#ifdef PROCESS
    recommend *rcmd = filtering(reqs[0]);
    p_merge_sort(1, 0, rcmd->size - 1, rcmd);

    FILE *output;
    char fileName[32];
    sprintf(fileName, "%dp.out", reqs[0]->id);
    if ((output = fopen(fileName, "w")) == NULL)
        ERR_EXIT("fopen");
    for (int j = 0; j < rcmd->size; j++)
        fprintf(output, "%s\n", rcmd->titles[j]);

    fclose(output);
    munmap(rcmd, sizeof(recommend));
#elif defined THREAD
    for (int i = 0; i < num_of_reqs; i++)
    {
        num_of_threads++;
        pthread_create(&tids[i], NULL, (void *) t_handle_request, (void *) reqs[i]);
    }

    for (int i = 0; i < num_of_reqs; i++)
    {
        pthread_join(tids[i], NULL);
        num_of_threads--;
    }
#endif
    return 0;
}

/**=======================================
 * You don't need to modify following code *
 * But feel free if needed.                *
 =========================================**/

request *read_request()
{
    int id;
    char buf1[MAX_LEN], buf2[MAX_LEN];
    char delim[2] = ",";

    char *keywords;
    char *token, *ref_pts;
    char *ptr;
    double ret, sum;

    scanf("%u %254s %254s", &id, buf1, buf2);
    keywords = malloc(sizeof(char) * strlen(buf1) + 1);
    if (keywords == NULL)
    {
        ERR_EXIT("malloc");
    }

    memcpy(keywords, buf1, strlen(buf1));
    keywords[strlen(buf1)] = '\0';

    double *profile = malloc(sizeof(double) * NUM_OF_GENRE);
    if (profile == NULL)
    {
        ERR_EXIT("malloc");
    }
    sum = 0;
    ref_pts = strtok(buf2, delim);
    for (int i = 0; i < NUM_OF_GENRE; i++)
    {
        ret = strtod(ref_pts, &ptr);
        profile[i] = ret;
        sum += ret * ret;
        ref_pts = strtok(NULL, delim);
    }

    // normalize
    sum = sqrt(sum);
    for (int i = 0; i < NUM_OF_GENRE; i++)
    {
        if (sum == 0)
            profile[i] = 0;
        else
            profile[i] /= sum;
    }

    request *r = malloc(sizeof(request));
    if (r == NULL)
    {
        ERR_EXIT("malloc");
    }

    r->id = id;
    r->keywords = keywords;
    r->profile = profile;

    return r;
}

/*=================initialize the dataset=================*/
void initialize(FILE *fp)
{

    char chunk[MAX_LEN] = {0};
    char *token, *ptr;
    double ret, sum;
    int cnt = 0;

    assert(fp != NULL);

    // first row
    if (fgets(chunk, sizeof(chunk), fp) == NULL)
    {
        ERR_EXIT("fgets");
    }

    memset(movies, 0, sizeof(movie_profile *) * MAX_MOVIES);

    while (fgets(chunk, sizeof(chunk), fp) != NULL)
    {

        assert(cnt < MAX_MOVIES);
        chunk[MAX_LEN - 1] = '\0';

        const char delim1[2] = " ";
        const char delim2[2] = "{";
        const char delim3[2] = ",";
        unsigned int movieId;
        movieId = atoi(strtok(chunk, delim1));

        // title
        token = strtok(NULL, delim2);
        char *title = malloc(sizeof(char) * strlen(token) + 1);
        if (title == NULL)
        {
            ERR_EXIT("malloc");
        }

        // title.strip()
        memcpy(title, token, strlen(token) - 1);
        title[strlen(token) - 1] = '\0';

        // genres
        double *profile = malloc(sizeof(double) * NUM_OF_GENRE);
        if (profile == NULL)
        {
            ERR_EXIT("malloc");
        }

        sum = 0;
        token = strtok(NULL, delim3);
        for (int i = 0; i < NUM_OF_GENRE; i++)
        {
            ret = strtod(token, &ptr);
            profile[i] = ret;
            sum += ret * ret;
            token = strtok(NULL, delim3);
        }

        // normalize
        sum = sqrt(sum);
        for (int i = 0; i < NUM_OF_GENRE; i++)
        {
            if (sum == 0)
                profile[i] = 0;
            else
                profile[i] /= sum;
        }

        movie_profile *m = malloc(sizeof(movie_profile));
        if (m == NULL)
        {
            ERR_EXIT("malloc");
        }

        m->movieId = movieId;
        m->title = title;
        m->profile = profile;

        movies[cnt++] = m;
    }
    num_of_movies = cnt;

    // request
    scanf("%d", &num_of_reqs);
    assert(num_of_reqs <= MAX_REQ);
    for (int i = 0; i < num_of_reqs; i++)
    {
        reqs[i] = read_request();
    }
}
/*========================================================*/