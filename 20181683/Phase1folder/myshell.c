/* 
 * System Programming
 * Project1 phase1
 * Myshell.c
 */

#include "myshell.h"

int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    
    signal(SIGINT,handler); /* Ctrl + C (SIGINT) handle */

    while (1) {
        printf("CSE4100:P1-myshell> "); /* prompt */
        fgets(cmdline, MAXLINE, stdin); /* read */
        if (feof(stdin)) 
            exit(0);

        eval(cmdline); /* Evaluate */
    } 
}
  
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execvp() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int child_status;
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
 
    if (argv[0] == NULL) /* Ignore empty lines */
        return;   
    if (!builtin_command(argv)) /* not built_in command execute */
    {
        if((pid = Fork()) == 0) /* child process */
        {
            execvp(argv[0], argv);
            printf("%s: Command not found.\n", argv[0]);
            _exit(0);
	    }

	    if (!bg) /* parent process */
        {    
	        int status;
            Waitpid(pid,&status,WUNTRACED);
        }
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "exit")) /* exit command */
        exit(0);
    if(!strcmp(argv[0], "cd")) /* bin폴더 안에 없는 cd 명령어 예외처리 */
    {
        if(chdir(argv[1])==-1)
            perror("cd");
        return 1;
    }
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
        return 1;

    return 0;                     /* Not a builtin command */
}


/* 
 *  parseline 
 *   - Parse the command line and build the argv array
 *   - 따옴표 처리
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
    while ((delim = strchr(buf, ' '))) 
    {
        argv[argc++] = buf;
        char *temp = buf;
        char *quote = buf;//따옴표 시작
        char *end = buf; //따옴표 끝
        int quote_flag = 0; 
        while (*temp && (*temp != ' ')) /* Ignore spaces */
        {
            if(*temp == '\'')
            {
                quote = temp;
                temp++;
                end = strchr(temp, '\''); 
                temp = end;
                delim = strchr(temp, ' ');
                quote_flag = 1;
            }
            if(*temp == '\"')
            {
                quote = temp;
                temp++;
                end = strchr(temp, '\"');
                temp = end;
                delim = strchr(temp,  ' ');
                quote_flag = 1;
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
        while(*buf && (*buf == ' '))
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
 * handler 
 *  - In Phase1, handle SIGINT 
 */
void handler(int signum)
{
    switch(signum){
        case SIGINT:
            fprintf(stdout,"\n");
            break;
     }
}

/* wrappers */
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
