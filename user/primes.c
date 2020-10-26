#include "kernel/types.h"
#include "user/user.h"

#define CHILD 0
#define PARENT 1
#define READ 0
#define WRITE 1

void close_pipe(int *p)
{
    close(p[0]);
    close(p[1]);
}

void primes()
{
    int prev_num;
    int next_num;
    int len;
    int fd[2];
    if((len = read(READ, &prev_num, sizeof(int))) <= 0 || prev_num <= 0) //not any numbers
    {
        close(1);
        exit();
    }
    printf("prime %d\n", prev_num);
    pipe(fd); //get two pipes
    if(fork()==0)
    {
        close(READ);
        dup(fd[READ]); //copy read_fd -> 0
        close_pipe(fd);
        primes();
    }
    else
    {
        close(WRITE);
        dup(fd[WRITE]);
        close_pipe(fd);
        while ((len = read(READ, &next_num, sizeof(int)))>0 && next_num >0)
        {
            if (next_num % prev_num != 0)
            {
                write(WRITE, &next_num, sizeof(int));
            }
        }
        if (len <= 0 || next_num <= 0)
        {
            close(WRITE);
            exit();
        }
    }
}

int main()
{
    int n;
    int fd[2];
    pipe(fd);
    if(fork()==0)
    {
        close(READ);
        dup(fd[READ]);
        close_pipe(fd);
        primes();
    }
    else
    {
        close(WRITE);
        dup(fd[WRITE]);
        close_pipe(fd);
        // init 2-35
        for(n = 2; n <= 35; n++)
        {
            write(WRITE, &n, sizeof(int));
        }
        close(WRITE);
        wait(); //wait for child_process
    }
    exit();
}