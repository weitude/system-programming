#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define RECORD_PATH "./bookingRecord"

typedef struct
{
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct
{
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request *requestP = NULL;  // point to a list of requests
int maxfd;  // size of open file descriptor table, size of request list

const char IAC_IP[3] = "\xff\xf4";

static void init_server(unsigned short port);
// initialize a server, exit for error

static void init_request(request *reqP);
// initialize a request instance

static void free_request(request *reqP);
// free resources used by a request instance

typedef struct
{
    int id;          // 902001-902020
    int bookingState[3];
} record;

record temp;

int handle_read(request *reqP)
{
    /*  Return value:
     *      1: read successfully
     *      0: read EOF (client down)
     *     -1: read failed
     */
    int r;
    char buf[512];
    memset(buf, 0, sizeof(buf));

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char *p1 = strstr(buf, "\015\012");
    if (p1 == NULL)
    {
        p1 = strstr(buf, "\012");
        if (p1 == NULL)
        {
            if (!strncmp(buf, IAC_IP, 2))
            {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len - 1;
    return 1;
}

int setLock(int fd, int type, int conn_fd)
{
    struct flock lock;
    lock.l_type = type;
    lock.l_start = sizeof(record) * requestP[conn_fd].id;
    lock.l_whence = SEEK_SET;
    lock.l_len = sizeof(record);
    return (fcntl(fd, F_SETLK, &lock));
}

void printToClient(int conn_fd, char *buf)
{
    write(conn_fd, buf, strlen(buf));
}

void printInformation(int fd, int conn_fd)
{
    char buf[512];
    lseek(fd, sizeof(record) * requestP[conn_fd].id, SEEK_SET);
    read(fd, &temp, sizeof(record));
    sprintf(buf,
            "Food: %d booked\n"
            "Concert: %d booked\n"
            "Electronics: %d booked\n",
            temp.bookingState[0],
            temp.bookingState[1],
            temp.bookingState[2]);
    write(conn_fd, buf, strlen(buf));

}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;
    int conn_fd;  // fd for a new connection with client
    int file_fd = open(RECORD_PATH, O_RDWR);  // fd for file that we open for reading
    int ret;
    char buf[512];

    init_server((unsigned short) atoi(argv[1]));
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd, maxfd);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    int readLock[21] = {};
    bool writeLock[21] = {};

    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    FD_SET(svr.listen_fd, &master_set);

    while (1)
    {
        working_set = master_set;
        if (select(maxfd + 1, &working_set, NULL, NULL, &timeout) <= 0)
            continue;
        if (FD_ISSET(svr.listen_fd, &working_set))
        {
            clilen = sizeof(cliaddr);
            conn_fd = accept(svr.listen_fd, (struct sockaddr *) &cliaddr, (socklen_t *) &clilen);
            if (conn_fd < 0)
            {
                if (errno == EINTR || errno == EAGAIN) continue;  // try again
                if (errno == ENFILE)
                {
                    fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }
            FD_SET(conn_fd, &master_set);
            requestP[conn_fd].conn_fd = conn_fd;
            strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
            fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requestP[conn_fd].host);
            printToClient(conn_fd, "Please enter your id (to check your booking state):\n");
            continue;
        }

        bool close_flag = false;
        for (int i = 3; i < maxfd; i++)
        {
            if (FD_ISSET(i, &working_set))
            {
                conn_fd = i;
                if (!requestP[conn_fd].wait_for_write)
                {
                    ret = handle_read(&requestP[conn_fd]);
                    if (ret <= 0)
                    {
                        fprintf(stderr, "conn_fd %d has some trouble...\n", conn_fd);
                        close_flag = true;
                        break;
                    }
                    requestP[conn_fd].id = atoi(requestP[conn_fd].buf) - 902001;
                    if (requestP[conn_fd].id < 0 || requestP[conn_fd].id >= 20)
                    {
                        printToClient(conn_fd, "[Error] Operation failed. Please try again.\n");
                        close_flag = true;
                        break;
                    }
#ifdef READ_SERVER
                    //處理學號
                    if (setLock(file_fd, F_RDLCK, conn_fd) != -1)
                    {
                        readLock[requestP[conn_fd].id]++;
                        printInformation(file_fd, conn_fd);
                        printToClient(conn_fd, "\n(Type Exit to leave...)\n");
                    }
                    else
                    {
                        printToClient(conn_fd, "Locked.\n");
                        close_flag = true;
                        break;
                    }
#elif defined WRITE_SERVER
                    //處理學號
                    if (writeLock[requestP[conn_fd].id] == false &&
                        setLock(file_fd, F_WRLCK, conn_fd) != -1)
                    {
                        writeLock[requestP[conn_fd].id] = true;
                        printInformation(file_fd, conn_fd);
                        printToClient(conn_fd,
                                      "\nPlease input your booking command. (Food, Concert, Electronics. Positive/negative value increases/decreases the booking amount.):\n");
                    }
                    else
                    {
                        printToClient(conn_fd, "Locked.\n");
                        close_flag = true;
                        break;
                    }
#endif
                    requestP[conn_fd].wait_for_write = true;
                }

                else
                {
#ifdef READ_SERVER
                    //判斷 Exit
                    ret = handle_read(&requestP[conn_fd]);
                    if (ret <= 0)
                    {
                        fprintf(stderr, "conn_fd %d has some trouble...\n", conn_fd);
                        readLock[requestP[conn_fd].id]--;
                        if (readLock[requestP[conn_fd].id] == 0)
                            setLock(file_fd, F_UNLCK, conn_fd);
                        close_flag = true;
                        break;
                    }

                    if (!strcmp("Exit", requestP[conn_fd].buf))
                    {
                        readLock[requestP[conn_fd].id]--;
                        if (readLock[requestP[conn_fd].id] == 0)
                            setLock(file_fd, F_UNLCK, conn_fd);
                        close_flag = true;
                    }
#elif defined WRITE_SERVER
                    //處理 cmd
                    ret = handle_read(&requestP[conn_fd]);
                    if (ret <= 0)
                    {
                        fprintf(stderr, "conn_fd %d has some trouble...\n", conn_fd);
                        writeLock[requestP[conn_fd].id] = false;
                        setLock(file_fd, F_UNLCK, conn_fd);
                        close_flag = true;
                        break;
                    }

                    int num[3];
                    sscanf(requestP[conn_fd].buf, "%d %d %d", &num[0], &num[1], &num[2]);
                    char check[512];
                    sprintf(check, "%d %d %d", num[0], num[1], num[2]);
                    if (strcmp(requestP[conn_fd].buf, check))
                    {
                        writeLock[requestP[conn_fd].id] = false;
                        setLock(file_fd, F_UNLCK, conn_fd);
                        printToClient(conn_fd, "[Error] Operation failed. Please try again.\n");
                        close_flag = true;
                        break;
                    }

                    int sum = 0;
                    bool positive_flag = 1;
                    lseek(file_fd, sizeof(record) * requestP[conn_fd].id, SEEK_SET);
                    read(file_fd, &temp, sizeof(record));
                    for (int j = 0; j < 3; j++)
                    {
                        if (temp.bookingState[j] + num[j] < 0)
                        {
                            printToClient(conn_fd, "[Error] Sorry, but you cannot book less than 0 items.\n");
                            positive_flag = 0;
                            break;
                        }
                    }
                    if (positive_flag)
                    {
                        for (int j = 0; j < 3; j++)
                            sum += (temp.bookingState[j] + num[j]);

                        if (sum > 15)
                            printToClient(conn_fd, "[Error] Sorry, but you cannot book more than 15 items in total.\n");
                        else
                        {
                            for (int j = 0; j < 3; j++)
                                temp.bookingState[j] += num[j];
                            lseek(file_fd, sizeof(record) * (requestP[conn_fd].id), SEEK_SET);
                            write(file_fd, &temp, sizeof(record));
                            sprintf(buf, "Bookings for user %d are updated, the new booking state is:\n", requestP[conn_fd].id + 902001);
                            write(conn_fd, buf, strlen(buf));
                            printInformation(file_fd, conn_fd);
                        }
                    }
                    writeLock[requestP[conn_fd].id] = false;
                    setLock(file_fd, F_UNLCK, conn_fd);
                    close_flag = true;
#endif
                }
                break;
            }
        }
        if (close_flag)
        {
            FD_CLR(conn_fd, &master_set);
            close(conn_fd);
            free_request(&requestP[conn_fd]);
        }
    }
    close(file_fd);
    free(requestP);
    return 0;
}

// ======================================================================================================
// You don't need to know how the following codes are working
//#include <fcntl.h>

static void init_request(request *reqP)
{
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
    reqP->wait_for_write = false;
}

static void free_request(request *reqP)
{
    init_request(reqP);
}

static void init_server(unsigned short port)
{
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void *) &tmp, sizeof(tmp)) < 0)
    {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0)
    {
        ERR_EXIT("listen");
    }

    // Get file descriptor table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request *) malloc(sizeof(request) * maxfd);
    if (requestP == NULL)
    {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++)
    {
        init_request(&requestP[i]);
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);

    return;
}
