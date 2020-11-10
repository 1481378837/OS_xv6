#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define READ 0
#define WRITE 1
#define ARGVEND 0
// refer from sh.c 
#define ARGS 10
#define WORDS 30
#define LINE 100

char whitespace[] = " \t\r\n\v"; //use as sh.c


int getcmd(char *buf, int nbuf) //get cmd from terminal
{
    fprintf(2, "@ ");
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0) // EOF
        return -1;
    return 0;
}
// end 
char args[ARGS][WORDS]; //arglist  set as global to use conveniently in different functions
void parse_args(char* cmd, int* argc, char* argv[]);
void run_cmd(int argc, char* argv[]);
void exec_pipe(int argc, char* argv[]);

int main()
{
    char buf[LINE];
    while(getcmd(buf,sizeof(buf))>=0)
    {
        if(fork()==0)
        {
            int argc;
            char* argv[ARGS];
            parse_args(buf,&argc,argv);
            run_cmd(argc, argv);
        }
        wait(0); //wait for child_process exit
    }
    exit(0);
}

void parse_args(char* cmd, int* argc, char* argv[])
{
    for(int i =0; i<ARGS; i++)
    {
        argv[i] = &args[i][0]; // let pointer argv[i] point to memory_args[i][0]
    }
    int index = 0; //第index个word
    // int length = strlen(cmd);
    for(int j=0;cmd[j]!='\0'; j++)
    {
        argv[index] = cmd+j; //let argv[i] point to cmd[j]
        while(strchr(whitespace,cmd[j])==0)
        {
            j++;
        }
        cmd[j] = '\0';//set the end of string
        index++;
        while(strchr(whitespace, cmd[j])) //ingnore the whitespace gap
        {
            j++;
        }
    }
    argv[index] = ARGVEND; //set end flag
    *argc = index;  //set arg_count
}

void run_cmd(int argc, char* argv[])
{
    for(int i=1; i<argc; i++) // firstly, check "|" pipe
    {
        if(!strcmp(argv[i], "|")) //split cmd and run_cmd more cmds 
        {
            exec_pipe(argc, argv);
        }
    }
    for(int i=1; i<argc;i++)
    {
        if(strcmp(argv[i],">")==0) // get ">", redirect output
        {
            close(WRITE);
            open(argv[i+1], O_CREATE|O_WRONLY); // then set create_fd as stdout_fd
            argv[i] = 0;
        }
        if(strcmp(argv[i],"<")==0)// get "<", redirect stdin
        {
            close(READ);
            open(argv[i+1],O_RDONLY);
            argv[i] = 0;
        }
    }
    exec(argv[0],argv);
}

void exec_pipe(int argc, char* argv[])
{
    int index;
    for(index=0;index<argc;index++)
    {
        if(!strcmp(argv[index],"|")) //split for pipe |
        {
            argv[index] = ARGVEND;
            break;
        }
    }

    int fd[2];
    pipe(fd);
    if(fork()==0)
    {
        // child_process exec cmd in the left
        close(WRITE);
        dup(fd[WRITE]);
        close(fd[READ]);
        close(fd[WRITE]); //save fd num
        run_cmd(index,argv);
    }
    else
    {
        // parent_process exec cmd in the right
        close(READ);
        dup(fd[READ]);
        close(fd[READ]);
        close(fd[WRITE]);//save fd num
        run_cmd(argc-index-1,argv+index+1);
    }
}