#include "kernel/types.h"
#include "user/user.h"

#define READ 0
#define WRITE 1
#define SIZE 4

int main()
{
    int parent_fd[2];
    int child_fd[2];
    pipe(parent_fd);
    pipe(child_fd);
    char ping[] = {"ping"};
    char pong[] = {"pong"};
    // int size = sizeof(ping);
    char buffer[SIZE];
    if(fork()==0)
    {
        close(parent_fd[WRITE]);
        close(child_fd[READ]);
        if(read(parent_fd[READ], buffer, SIZE) != SIZE ){
            fprintf(2,"parent -> child read_size err\n");
            exit();
        }
        for(int i=0; i<SIZE; i++)
        {
            if(buffer[i]!=ping[i])
            {
                fprintf(2,"While receiving 'ping', the %d char err\n");
                exit();
            }
        }
        printf("%d: received ping\n", getpid());
        if(write(child_fd[WRITE], pong, SIZE) != SIZE )
        {
            fprintf(2,"parent <- child write_size err\n");
            exit();
        }
        exit();
    }
    close(parent_fd[READ]);
    close(child_fd[WRITE]);

    if(write(parent_fd[WRITE], ping, SIZE) != SIZE)
    {
        fprintf(2,"parent -> child write_size err\n");
        exit();
    }
    if(read(child_fd[READ], buffer, SIZE) != SIZE )
    {
            fprintf(2,"parent <- child read_size err\n");
            exit();
    }
    for(int i=0; i<SIZE; i++)
    {
        if(buffer[i]!=pong[i])
        {
            fprintf(2,"While receving 'pong', the %d char err\n");
            exit();
        }
    }
    printf("%d: received pong\n", getpid());
    wait(); //
    exit();
}