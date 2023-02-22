#include "status.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

#define R_PIPE 0
#define W_PIPE 1
#define READ_END 0
#define WRITE_END 1

Status player[8];
int battle_attr[] = {0, 1, 2, 2, 0, 0, 0, 1, 2, 1, 1, 1, 0, 2};
int player_num[] = {0, 0, 1, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2, 2};
char *b_parent[] = {"none", "A", "A", "B", "B", "C", "D", "D", "E", "E", "F", "F", "K", "L"};
char *b_children[] = {"B", "C", "D", "E", "F", "14", "G", "H", "I", "J", "K", "L",
                      "0", "1", "2", "3", "4", "5", "6", "7", "M", "10", "N", "13", "8", "9", "11", "12"};

void initialPlayerStatus()
{
    FILE *ps_fp = fopen("player_status.txt", "r");
    char s_attr[10];
    for (int i = 0; i < 8; i++)
    {
        fscanf(ps_fp, "%d %d %s %c %d",
               &player[i].HP, &player[i].ATK, s_attr, &player[i].current_battle_id, &player[i].battle_ended_flag);

        if (s_attr[0] == 'F')
            player[i].attr = FIRE;
        else if (s_attr[0] == 'G')
            player[i].attr = GRASS;
        else if (s_attr[0] == 'W')
            player[i].attr = WATER;
    }
    fclose(ps_fp);
}

// if alive, return 1
int attack(Status *attacker, Status *defender, int attr)
{
    int ATK = attacker->ATK;
    if (player[attacker->real_player_id].attr == attr)
        ATK *= 2;
    defender->HP -= ATK;
    if (defender->HP <= 0)
    {
        attacker->battle_ended_flag = defender->battle_ended_flag = 1;
        return 0;
    }
    return 1;
}

void readPSSM(int fd, FILE *fp, Status *ps, char battle_id, int pid, char *target_id, int target_pid)
{
    read(fd, ps, sizeof(Status));
    fprintf(fp, "%c,%d pipe from %s,%d %d,%d,%c,%d\n",
            battle_id, pid,
            target_id, target_pid,
            ps->real_player_id, ps->HP, ps->current_battle_id, ps->battle_ended_flag);
}

void writePSSM(int fd, FILE *fp, Status *ps, char battle_id, int pid, char *target_id, int target_pid)
{
    write(fd, ps, sizeof(Status));
    fprintf(fp, "%c,%d pipe to %s,%d %d,%d,%c,%d\n",
            battle_id, pid,
            target_id, target_pid,
            ps->real_player_id, ps->HP, ps->current_battle_id, ps->battle_ended_flag);
}

int main(int argc, const char **argv)
{
    FILE *logfp;
    Status ps[2];
    pid_t pid, parent_pid, fork_pid, child_pid[2];
    int champion, round, player_exec_flag, attr, PASSING_MODE;
    char battle_id, s_log[1024], s_id[1024], s_parent_pid[1024];

    int r_pipe[2]; // [left/right]
    int w_pipe[2]; // [left/right]
    int pipefd[2][2][2]; // [left/right][r_pipe/w_pipe][read/write]

    if (argc != 3)
        ERR_EXIT("wrong battle usage!");

    // Init Mode
    battle_id = *argv[1];
    parent_pid = atoi(argv[2]);
    pid = getpid();
    player_exec_flag = 2 - player_num[battle_id - 'A'];
    attr = battle_attr[battle_id - 'A'];
    initialPlayerStatus();

    sprintf(s_log, "log_battle%c.txt", battle_id);
    logfp = fopen(s_log, "a");
    if (!logfp)
        ERR_EXIT("open log file failed");

    for (int i = 0; i <= 1; i++)
    {
        pipe(pipefd[i][R_PIPE]);
        pipe(pipefd[i][W_PIPE]);
        fork_pid = fork();
        child_pid[i] = fork_pid;
        if (!fork_pid)
        {
            close(pipefd[i][R_PIPE][READ_END]);
            close(pipefd[i][W_PIPE][WRITE_END]);
            dup2(pipefd[i][R_PIPE][WRITE_END], 1);
            dup2(pipefd[i][W_PIPE][READ_END], 0);
            close(pipefd[i][R_PIPE][WRITE_END]);
            close(pipefd[i][W_PIPE][READ_END]);
            sprintf(s_parent_pid, "%d", pid);
            if (i >= player_exec_flag)
            {
                sprintf(s_id, "%d", atoi(b_children[2 * (battle_id - 'A') + i]));
                execl("./player", "./player", s_id, s_parent_pid, (char *) 0);
            }
            else
            {
                sprintf(s_id, "%s", b_children[2 * (battle_id - 'A') + i]);
                execl("./battle", "./battle", s_id, s_parent_pid, (char *) 0);
            }
        }
        close(pipefd[i][R_PIPE][WRITE_END]);
        close(pipefd[i][W_PIPE][READ_END]);
        r_pipe[i] = pipefd[i][R_PIPE][READ_END];
        w_pipe[i] = pipefd[i][W_PIPE][WRITE_END];
    }

    // Waiting Mode
    for (int i = 0; i <= 1; i++)
        readPSSM(r_pipe[i], logfp, &ps[i], battle_id, pid, b_children[2 * (battle_id - 'A') + i], child_pid[i]);

    // Playing Mode
    while (!ps[0].battle_ended_flag)
    {
        ps[0].current_battle_id = battle_id;
        ps[1].current_battle_id = battle_id;

        if (ps[0].HP < ps[1].HP)
            round = 0;
        else if (ps[0].HP == ps[1].HP && ps[0].real_player_id < ps[1].real_player_id)
            round = 0;
        else
            round = 1;

        if (attack(&ps[round], &ps[1 - round], attr))
        {
            round = 1 - round;
            attack(&ps[round], &ps[1 - round], attr);
        }
        for (int i = 0; i <= 1; i++)
            writePSSM(w_pipe[i], logfp, &ps[i], battle_id, pid, b_children[2 * (battle_id - 'A') + i], child_pid[i]);

        if (!ps[0].battle_ended_flag)
            for (int i = 0; i <= 1; i++)
                readPSSM(r_pipe[i], logfp, &ps[i], battle_id, pid, b_children[2 * (battle_id - 'A') + i], child_pid[i]);
    }
    wait(NULL);

    // Passing Mode
    PASSING_MODE = 1;
    while (PASSING_MODE)
    {
        if (battle_id != 'A')
        {
            readPSSM(r_pipe[round], logfp, &ps[round], battle_id, pid, b_children[2 * (battle_id - 'A') + round], child_pid[round]);
            writePSSM(STDOUT_FILENO, logfp, &ps[round], battle_id, pid, b_parent[battle_id - 'A'], parent_pid);
            readPSSM(STDIN_FILENO, logfp, &ps[round], battle_id, pid, b_parent[battle_id - 'A'], parent_pid);
            writePSSM(w_pipe[round], logfp, &ps[round], battle_id, pid, b_children[2 * (battle_id - 'A') + round], child_pid[round]);

            if (ps[round].HP <= 0 || ps[round].current_battle_id == 'A' && ps[round].battle_ended_flag == 1)
            {
                wait(NULL);
                PASSING_MODE = 0;
            }
        }
        else
        {
            champion = ps[round].real_player_id;
            wait(NULL);
            PASSING_MODE = 0;
        }
    }

    if (battle_id == 'A')
    {
        printf("Champion is P%d\n", champion);
        fflush(stdout);
    }
    fclose(logfp);
    for (int i = 0; i <= 1; i++)
    {
        close(r_pipe[i]);
        close(w_pipe[i]);
    }
    exit(0);
}