/*
 * System programming
 * Project1 Phase2
 * Myshell.c
 */

#include "myshell.h"

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    signal(SIGINT,handler); /* Ctrl + c,z signal handle */
    signal(SIGTSTP,handler);

    while (1) 
    {
	    printf("CSE4100:P1-myshell> "); /* prompt */
	    fgets(cmdline, MAXLINE, stdin); /* command read */
	    if (feof(stdin))
            exit(0);

	    eval(cmdline); /* evaluate */
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
    bg = parseline(buf, argv); /* cmdline parsing */
    
    if (argv[0] == NULL) /* Igonre Empty line */
		return;    
    if (!builtin_command(argv)) /* built-in command handling */
    {
        if((pid = Fork()) == 0) /* child process */
        {
            if(strchr(cmdline,'|')) /* pipe exist */
            {
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
                execvp(argv[0], argv);
                printf("%s: Command not found.\n", argv[0]);
                _exit(0);
            }
        }
        else if(pid > 0) /* parent process */
        {
            if (!bg){ /* wait until child process exit */ 
                int status;
                Waitpid(pid,&status,WUNTRACED);
            }
        }
    }
    return;
}

/* signal handler */
void handler(int signum)
{
    switch(signum){
        case SIGINT:
            fprintf(stdout,"\n");
            break;
        case SIGTSTP:
            fprintf(stdout,"\n");
            break;
    }
}

/*
 * mydup
 *  -dup2+close macro
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
 *  - argv : command /  pipe_idx : '|' index / pipe_cnt : number of '|'(pipe)  
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
            fprintf(stderr, "pipe error\n");
            _exit(0);
        }
        switch(pid = fork())
        {
            case -1:
                fprintf(stderr, "fork error\n");
                _exit(0);
            case 0 : //chlid process
                close(pipefd[0]);
                mydup(in_fd ,STDIN_FILENO);
                mydup(pipefd[1], STDOUT_FILENO);
                Execvp(argv[0],&argv[0]);
            default :
                close(pipefd[1]);
                close(in_fd);
                Waitpid(pid,&status,WUNTRACED);
                go_pipe(&argv[pipe_idx[1]-pipe_idx[0]], &pipe_idx[1], pipe_cnt - 1, pipefd[0]);
        }
    }
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

        char *temp = buf;
        char *quote = buf; //따옴표 시작
        char *end = buf; //따옴표 시작
        int quote_flag = 0; 

        while(*temp && (*temp!= ' '))
        {
            if(*temp == '\'')
            {
                quote = temp;
                temp++;
                end = strchr(temp, '\'');
                temp = end;
                delim = strchr(temp, ' ');
                quote_flag = 1;
                break;
            }
            if(*temp == '\"')
            {
                quote = temp;
                temp++;
                end = strchr(temp, '\"');
                temp = end;
                delim = strchr(temp, ' ');
                quote_flag = 1;
                break;
            }
            temp++;
        }
        *delim = '\0';
        
        if(quote_flag == 1)
        {
            temp = buf;
            while(temp <= delim)
            {
                if(buf > delim)
                {
                    *temp = ' ';
                    temp++;
                }
                else if(buf != quote && buf != end)
                {
                    *temp = *buf;
                    temp++;
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
 * change_cmdline 
 *  - pipe, bg 명령어를 처리하기 위해 cmdline을 바꿔주는 함수
 */
void change_cmdline(char* cmdline)
{
    int i;
    int len = strlen(cmdline);
    char buf[MAXLINE];
    int buf_idx = 0;
    
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
            buf[buf_idx++] = ' ';
            buf[buf_idx++] = cmdline[i];
            buf[buf_idx++] = ' ';
        }
        else
            buf[buf_idx++] = cmdline[i];
        buf[buf_idx] = '\0';
    }
    strcpy(cmdline, buf);
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
