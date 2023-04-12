/*
 * System Programming
 * Project 1 phase3
 * Myshell.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

/* 최대값 지정 */
#define MAXARGS 128
#define MAXLINE 8192
#define MAXFILE 256
#define MAXJOBS 100

sigset_t mask;

typedef struct _jobStruct{
    int status; /* Process status 0:stopped 1:sleep 2:running */
    int bg; /* 1:background */
    int pid;
    char cmd[MAXLINE];
}jobStruct;

jobStruct job[MAXJOBS];
static int jobNum;

int fg_pid;
char fg_cmd[100];


/* 함수 선언 */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void change_cmdline(char *cmdline);
void go_pipe(char **argv, int *pipe_idx, int pipe_cnt, int in_fd);
void mydup(int oldfd, int newfd);
void handler(int signum);
int do_fg(char **argv);
int do_bg(char **argv);
int do_kill(char **argv);
void printJobs();
void addJob(int pid, char *cmdline);
void delJob(int pid);
int getStatus(int pid);
int getIdx(int pid);
void updateJob();
void printcommand(char *cmdline);

/* wrappers */
void unix_error(char *msg);
pid_t Fork();
pid_t Wait(int *status);
pid_t Waitpid(pid_t pid, int *status, int options);
void Execvp(const char *filename, char *const argv[]);
