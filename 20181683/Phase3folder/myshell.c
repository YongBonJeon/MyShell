/*
 * System Programming
 * Project1 phase3
 * Myshell.c
 */

#include "myshell.h"

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    
    jobNum = 0;
    signal(SIGINT,handler); /* signal handle */
    signal(SIGTSTP,handler);
    signal(SIGCHLD,handler);

    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigaddset(&mask,SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, NULL); /* signal block */

    while (1) 
    {
        fg_pid = 0; /* nothing run in foreground */
        updateJob(); /* update background process */

        printf("CSE4100:P1-myshell> "); /* prompt */
        fflush(stdout);
        fgets(cmdline, MAXLINE, stdin); /* read */
	    if (feof(stdin))
            exit(0);
	    
        eval(cmdline); /* evalutate */
        fflush(stdout);
    } 
}

/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS];    /* Argument list execvp() */
    char buf[MAXLINE];      /* Holds modified command line */
    int bg;                 /* Should the job run in bg or fg? */
    pid_t pid;              /* Process id */
    int pipe_idx[MAXARGS];  /* pipe 수행하는 argv index */
    int pipe_cnt = 0;       /* pipe 개수 */
    int pipe_flag = 0;;     /* pipe 처리해주어야하는지 */


    change_cmdline(cmdline); //pipe,bg관련해서 공백처리를 위해 가벼운 변경

    strcpy(buf, cmdline);
    bg = parseline(buf, argv); /* cmdline parsing + 1:background , 0:foreground */
    for(int i=0 ;i<strlen(cmdline); i++){ /* not use '&' anymore */
        if(cmdline[i]=='&')
            cmdline[i]='\0';
    }

    if (argv[0] == NULL) /* Ignore Empty line */ 
		return;    
    if (!builtin_command(argv)) /* built_in command handling */
    {
        if((pid = Fork()) == 0) /* child process */
        {
            setpgid(0,0); /* send sinal proces group error handling.. */

            sigprocmask(SIG_UNBLOCK, &mask, NULL); /* signal unblockP */

            if(strchr(cmdline,'|')) /* pipe exist */
            {
                sigprocmask(SIG_UNBLOCK, &mask,NULL);
                for(int i = 0; argv[i] !=NULL; i++) /* check '|'(pipe) index in argv */
                {
                    if(!strcmp(argv[i], "|"))
                    {
                        argv[i] = NULL;
                        pipe_idx[++pipe_cnt] = i+1;
                        pipe_flag=1;
                    }
                }
                if(pipe_flag == 1)
                {
                    go_pipe(argv, pipe_idx, pipe_cnt, STDIN_FILENO); /* pipe execute */
                }
            }
            else /* pipe not exist */
            {
                sigprocmask(SIG_UNBLOCK,&mask,NULL);
                execvp(argv[0], argv);
                printf("%s: Command not found.\n", argv[0]);
                _exit(0);
            }
        }
        else if(pid > 0) /* parent process */
        {
            if (bg) /* not wait for background process */
            {
                sigprocmask(SIG_BLOCK, &mask, NULL);
                addJob(pid, cmdline);
            }
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            if (!bg) /* wait for foreground process */
            {
                memset(fg_cmd, 0, sizeof(fg_cmd));
                fg_pid = pid; /* running fore ground process pid now */
                strcpy(fg_cmd,cmdline); /* fg_cmd = running foreground process cmd now */
                int status;
                waitpid(pid,&status,WUNTRACED);
            }
        }
    }
    return;
}
/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "exit")) /* exit command */
		exit(0);
    if(!strcmp(argv[0], "cd")) /*bin폴더 안에 없는 cd 명령어 예외처리*/
    {
       if(chdir(argv[1])==-1)
	     perror("cd");
       return 1;
    }
    if(!strcmp(argv[0], "jobs")) /* print background jobs */
    {
        printJobs();
        return 1;
    }
    if(!strcmp(argv[0], "bg")) /* stopped job -> ruuning background job */
    {
        if(do_bg(argv) < 0)
            fprintf(stdout,"No Such Job\n");
        return 1;
    }
    if(!strcmp(argv[0], "fg")) /* stopped | running background job -> running foreground job*/
    {
        if(do_fg(argv) < 0)
            fprintf(stdout, "No Such Job\n");
        return 1;
    }
    if(!strcmp(argv[0], "kill")) /* kill job */
    {
        if(do_kill(argv) < 0)
            fprintf(stdout, "No Such Job\n");
        return 1;
    }

    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
    return 0;                     /* Not a builtin command */
}

/*  parseline 
 *  - Parse the command line and build the argv array
 *  - 따옴표 처리
 */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	    buf++;

    /* Build the argv list */
    argc = 0;
    while((delim = strchr(buf, ' ')))
    {
        argv[argc++]=buf;

        char *qt = buf;
        char *quote = buf; //따옴표 시작
        char *end = buf; //따옴표 끝
        int quote_flag = 0; 

        while(*qt && (*qt!= ' '))
        {
            if(*qt == '\'')
            {
                quote = qt;
                qt++;
                end = strchr(qt, '\'');
                qt = end;
                delim = strchr(qt, ' ');
                quote_flag = 1;
                break;
            }
            if(*qt == '\"')
            {
                quote = qt;
                qt++;
                end = strchr(qt, '\"');
                qt = end;
                delim = strchr(qt, ' ');
                quote_flag = 1;
                break;
            }
            qt++;
        }
        *delim = '\0';
        
        if(quote_flag == 1)
        {
            qt = buf;
            while(qt <= delim)
            {
                if(buf > delim)
                {
                    *qt = ' ';
                    qt++;
                }
                else if(buf != quote && buf != end)
                {
                    *qt = *buf;
                    qt++;
                }
                buf++;
            }
        }
        buf = delim + 1;
        while(*buf && (*buf ==' '))
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
        return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
        argv[--argc] = NULL;
    return bg;
}

/*
 * hanlder 
 *  -SIGINT : kill foreground process
 *  -SIGTSTP : suspend foreground process
 *  -SIGCHLD : catch, reaping background process terminate 
 */
void handler(int signum)
{
    if(fg_pid!=0){  
        if(signum == SIGINT)
        {
            fprintf(stdout,"\n");
            kill(-fg_pid, SIGINT); /* sned signal to terminate */
            sleep(1);
            delJob(fg_pid);
        }
        else if(signum == SIGTSTP)
        {
            fprintf(stdout,"\n");
            kill(-fg_pid,SIGTSTP); /* send signal to suspend */
            sleep(1);
            addJob(fg_pid, fg_cmd);
        }
        else if(signum == SIGCHLD)
        {
            while(waitpid(-1,NULL,WNOHANG) > 0); /* reaping */
        }
    }
}
/*
 * updateJob()
 *  -if background process terminate, update jobstructure and show termination notice 
 */
void updateJob()
{
    int stat;
    pid_t pid;

    if(jobNum == 0)
        return ;
    for(int i = jobNum-1; i >=0 ; i--)
    {
        pid = job[i].pid;
        if(pid == -1)
            continue;

        stat = getStatus(pid);
        job[i].status = stat;
        if(stat == 0 || stat == 1 || stat == 2)
            continue;
        else
        {
            fprintf(stdout, "[%d] Done %s\n",i+1,job[i].cmd);

            job[i].pid = -1;
            memset(job[i].cmd, 0, sizeof(job[i].cmd));

            if(i == jobNum-1)
                jobNum--;
        }
    }
}
/*
 * delJob
 *  - delete from jobstructure
 */
void delJob(int pid)
{
    int i = getIdx(pid);
    if(i == -1)
        return;

    job[i].pid = -1;
    job[i].bg = 0;
    memset(job[i].cmd, 0, sizeof(job[i].cmd));

    if(i == jobNum -1)
        jobNum--;
}
/*
 * addjob
 *  - add in jobstructure
 *  - by Ctrl + z / execute background process
 */
void addJob(pid_t pid, char *cmdline)
{
    int stat;
    int i;

    stat = getStatus(pid); /* current process status */
    i = getIdx(pid); /* current process job index */
    
    if(i == -1)
        i=jobNum++;

    job[i].status = stat;
    job[i].pid=pid;
    job[i].bg = 1;
    strcpy(job[i].cmd, cmdline);

    if(stat == 0) /* stopped */
    {
        fprintf(stdout, "[%d] suspended %s\n", i+1, cmdline);
    }
    else
        fprintf(stdout, "[%d] %d\n", i+1, pid);

    return ;
}

/*
 * getIdx
 *  -job struct에서 idx 찾아 반환하는 함수
 */
int getIdx(pid_t pid)
{
    for(int i = 0; i < jobNum; i++)
    {
        if(job[i].pid == pid)
            return i;
    }
    return -1;
}
/*
 * getStatus
 *  - pid로 process status 반환하는 함
 */
int getStatus(pid_t pid)
{
    char path[MAXFILE];
    char buf[MAXFILE];
    char stat[10];
    FILE *fp;
    char garbage[30];
    int garbage1;

    if(kill(pid, 0) == -1)
        return -1;

    sprintf(path, "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%d %s %s\n", &garbage1, garbage, stat);
    if(strcmp(stat, "T") == 0)
        return 0;
    else if(strcmp(stat, "S") ==0)
        return 1;
    else if(strcmp(stat, "R") ==0)
        return 2;
    else if(strcmp(stat, "Z") ==0)
        return 3;

    return -1;
}

/*
 * do_kill
 *  -kill job
 */
int do_kill(char **argv)
{
    int i;
    int pid;

    i = atoi(&argv[1][1]) -1; /* job spec */
    if(i < 0 || i >= jobNum)
        return -1;
    pid = job[i].pid; /* job pid */
    if(pid == -1)
        return -1;
    kill(pid, SIGKILL); /* send signal to kill */
    delJob(pid); /* delete from job structure */
}

/*
 * do_fg
 *  - stopeed, running background process -> running foreground process
 */
int do_fg(char **argv)
{
    int i;
    int pid;

    i = atoi(&argv[1][1]) -1; /* job spec */
    if(i < 0 || i >= jobNum)
        return -1;
    pid = job[i].pid; /* job pid */
    if(pid == -1)
        return -1;
    job[i].status = getStatus(pid);
    if(job[i].status == 0 || job[i].status == 1 || job[i].status == 2)
    {
        char *argv[MAXARGS];
        char buf[MAXLINE];
        strcpy(buf,job[i].cmd);
        parseline(buf,argv);
        fprintf(stdout,"[%d] running %s\n",i+1, argv[0]);
        
        job[i].status = 2; /* running */
        job[i].bg = 0; /* foreground */
        kill(-pid, SIGCONT); /* stopped -> running */
        fg_pid = pid;
        strcpy(fg_cmd, job[i].cmd);
        waitpid(pid,NULL,WUNTRACED); /* wait running foreground process */
        return 0;
    }
    return -1;
}
/*
 * do_bg
 *  - command 'bg'
 *  - stopped background process -> running background process
 */
int do_bg(char **argv)
{
    int i;
    int pid;
    
    i = atoi(&argv[1][1]) -1;
    if(i < 0 || i >= jobNum)
        return -1;
    pid = job[i].pid; 
    if(pid == -1)
        return -1;
    job[i].status = getStatus(pid);

    if(job[i].status == 0) /* stopped background -> running background */
    {
        char *argv[MAXARGS];
        char buf[MAXLINE];
        strcpy(buf,job[i].cmd);
        parseline(buf,argv);
        fprintf(stdout, "[%d] running %s\n", i+1, argv[0]);
        job[i].bg = 1; /* background */
        job[i].status = 2; /* background running */

        kill(-pid, SIGCONT); /* stopped -> running  */
    
        return 0;
    }
    return -1;
}

/* print macro is used by printjobs()*/
void printcommand(char *cmdline)
{
    char *argv[MAXARGS];
    char buf[MAXLINE];
    strcpy(buf,cmdline);
    int i = parseline(buf,argv);
    
    for(int j = 0; argv[j]!=NULL; j++)
    {
        if(strcmp(argv[j],"&") == 0)
            continue;
        printf("%s ",argv[j]);
    }
    printf("\n");
}

/*
 * printJobs
 *  - command 'jobs' 
 *  - print background jobs
 */
void printJobs()
{
    for(int i = 0; i < jobNum; i++)
    {
        if(job[i].pid != -1)
        {
            switch(job[i].status){
                case 0:
                    fprintf(stdout, "[%d] suspended ",i+1);
                    printcommand(job[i].cmd);
                    break;
                case 1:
                case 2:
                    fprintf(stdout, "[%d] running ",i+1);
                    printcommand(job[i].cmd);
                    break;
            }
        }
    }
}


/*
 * change_cmdline 
 *  - pipe, bg 명령어를 처리하기 위해 cmdline을 바꿔주는 함수
 */
void change_cmdline(char* cmdline)
{
    int i;
    int len = strlen(cmdline);
    char temp[MAXLINE];
    int temp_idx = 0;
    
    cmdline[strlen(cmdline) -1] = ' ';
    for(i = 0; i < len; i++)
    {
        if(cmdline[i]=='\t')
            cmdline[i]= ' '; //cmdline에 tab을 space로 바꿈
    }

    for(i = 0; i < len; i++)
    {
        if(cmdline[i] == '|' || cmdline[i] == '&')
        {
            temp[temp_idx++] = ' ';
            temp[temp_idx++] = cmdline[i];
            temp[temp_idx++] = ' ';
        }
        else
            temp[temp_idx++] = cmdline[i];
        temp[temp_idx] = '\0';
    }
    strcpy(cmdline, temp);
}

/*
 * mydup
 *  -dup2 + close macro
 */
void mydup(int oldfd, int newfd)
{
    if(oldfd!=newfd){
        if(dup2(oldfd, newfd) != -1)
            close(oldfd);
    }
}

/*
 * go_pipe
 *  - pipe execute recursively
 */
void go_pipe(char **argv, int *pipe_idx, int pipe_cnt,int in_fd)
{
    pid_t pid;
    int status;

    if(pipe_cnt == 0) // last command
    {
        mydup(in_fd, STDIN_FILENO);
        Execvp(argv[0], &argv[0]);
    }
    else
    {
        int pipefd[2];
    
        if(pipe(pipefd) < 0)
        {
            _exit(0);
        }
        switch(pid=fork())
        {
            case -1:
                _exit(0);
            case 0 : //chlid process
                close(pipefd[0]);
                mydup(in_fd ,STDIN_FILENO);
                mydup(pipefd[1], STDOUT_FILENO);
                Execvp(argv[0],&argv[0]);
            default :
                close(pipefd[1]);
                close(in_fd);
                Waitpid(pid,&status,WUNTRACED); /* wait for child process */
                go_pipe(&argv[pipe_idx[1]-pipe_idx[0]], &pipe_idx[1], pipe_cnt - 1, pipefd[0]); /* recursive pipe function call */
        }
    }
}


/* wraapers */
void unix_error(char *msg)
{
   fprintf(stderr, "%s: %s\n", msg, strerror(errno));
   _exit(0);
}
pid_t Fork(void)
{
   pid_t pid;

   if((pid=fork())<0)
      unix_error("Fork error");
   return pid;
}
void Execvp(const char *filename, char *const argv[])
{
   if(execvp(filename,argv) < 0)
      unix_error("Execvp error");
}
pid_t Wait(int *status)
{
   pid_t pid;

   if((pid = wait(status)) < 0)
      unix_error("Wait error");
   return pid;
}

pid_t Waitpid(pid_t pid, int *iptr, int options)
{
   pid_t retpid;

   if((retpid = waitpid(pid, iptr, options)) < 0)
      unix_error("WaitPid error");

   return (retpid);
}	
