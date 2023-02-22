#include "status.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

char b_parent[] = {'G', 'G', 'H', 'H', 'I', 'I', 'J', 'J', 'M', 'M', 'K', 'N', 'N', 'L', 'C'};
int fifoTarget[] = {-1, 14, -1, 10, 13, -1, 8, 11, 9, 12};

void readPSSM(FILE *fp, Status *ps, int player_id, int pid, int parent_pid)
{
    read(STDIN_FILENO, ps, sizeof(Status));
    fprintf(fp, "%d,%d pipe from %c,%d %d,%d,%c,%d\n",
            player_id, pid,
            b_parent[player_id], parent_pid,
            ps->real_player_id, ps->HP, ps->current_battle_id, ps->battle_ended_flag);
}

void writePSSM(FILE *fp, Status *ps, int player_id, int pid, int parent_pid)
{
    write(STDOUT_FILENO, ps, sizeof(Status));
    fprintf(fp, "%d,%d pipe to %c,%d %d,%d,%c,%d\n",
            player_id, pid,
            b_parent[player_id], parent_pid,
            ps->real_player_id, ps->HP, ps->current_battle_id, ps->battle_ended_flag);
}

void initialPlayerStatus(Status *ps, int player_id)
{
    char s_attr[10];
    FILE *ps_fp = fopen("./player_status.txt", "r");
    if (!ps_fp)
        ERR_EXIT("No ./player_status.txt");

    ps->real_player_id = player_id;
    for (int i = 0; i <= player_id; i++)
    {
        fscanf(ps_fp, "%d %d %s %c %d",
               &ps->HP, &ps->ATK, s_attr, &ps->current_battle_id, &ps->battle_ended_flag);
    }
    fclose(ps_fp);

    if (s_attr[0] == 'F')
        ps->attr = FIRE;
    else if (s_attr[0] == 'G')
        ps->attr = GRASS;
    else if (s_attr[0] == 'W')
        ps->attr = WATER;
}

void realPlayerWriteFIFO(Status *ps)
{
    int wFIFO_fd;
    char s_fifo[1024];

    sprintf(s_fifo, "./player%d.fifo", fifoTarget[ps->current_battle_id - 'A']);
    mkfifo(s_fifo, 0666);
    wFIFO_fd = open(s_fifo, O_WRONLY);
    if (wFIFO_fd < 0)
        ERR_EXIT("open fifo error");
    write(wFIFO_fd, ps, sizeof(Status));
    close(wFIFO_fd);
}

void agentPlayerReadFIFO(Status *ps, int player_id)
{
    int rFIFO_fd;
    char s_fifo[1024];

    sprintf(s_fifo, "./player%d.fifo", player_id);
    mkfifo(s_fifo, 0666);
    rFIFO_fd = open(s_fifo, O_RDONLY);
    if (rFIFO_fd < 0)
        ERR_EXIT("open fifo error");
    read(rFIFO_fd, ps, sizeof(Status));
    close(rFIFO_fd);
}

int main(int argc, char **argv)
{
    Status ps; //player status
    pid_t pid, parent_pid;
    FILE *logfp;
    int player_id, original_HP;
    char s_log[1024];

    if (argc != 3)
        ERR_EXIT("wrong player usage!");

    pid = getpid();
    player_id = atoi(argv[1]);
    parent_pid = atoi(argv[2]);

    if (player_id <= 7)
    {
        sprintf(s_log, "log_player%d.txt", player_id);
        logfp = fopen(s_log, "a");
        if (!logfp)
            ERR_EXIT("open s_log file failed");

        initialPlayerStatus(&ps, player_id);
        writePSSM(logfp, &ps, player_id, pid, parent_pid);
    }
    else
    {
        agentPlayerReadFIFO(&ps, player_id);

        sprintf(s_log, "log_player%d.txt", ps.real_player_id);
        logfp = fopen(s_log, "a");
        if (!logfp)
            ERR_EXIT("open s_log file failed");

        fprintf(logfp, "%d,%d fifo from %d %d,%d\n",
                player_id, pid,
                ps.real_player_id,
                ps.real_player_id, ps.HP);
        writePSSM(logfp, &ps, player_id, pid, parent_pid);
    }

    original_HP = ps.HP;
    while (1)
    {
        while (1)
        {
            readPSSM(logfp, &ps, player_id, pid, parent_pid);
            if (ps.battle_ended_flag)
                break;
            writePSSM(logfp, &ps, player_id, pid, parent_pid);
        }
        // lose
        if (ps.HP <= 0)
            break;

        // win fianl
        if (ps.current_battle_id == 'A')
            exit(0);

        // win others
        ps.battle_ended_flag = 0;
        ps.HP += (original_HP - ps.HP) / 2;
        writePSSM(logfp, &ps, player_id, pid, parent_pid);
    }

    // lose
    if (ps.current_battle_id != 'A' && player_id <= 7)
    {
        ps.HP = original_HP;
        ps.battle_ended_flag = 0;
        realPlayerWriteFIFO(&ps);

        fprintf(logfp, "%d,%d fifo to %d %d,%d\n",
                player_id, pid,
                fifoTarget[ps.current_battle_id - 'A'],
                ps.real_player_id, ps.HP);
    }
    fclose(logfp);
    exit(0);
}
