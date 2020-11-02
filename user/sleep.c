#include "kernel/types.h"
#include "user/user.h"

// #define ARGCERR 1

int main(int argc, char* argv[])
{
    if(argc == 1)
    {
        fprintf(2,"You should input one argument\n");
        fprintf(2,"Eg: 'Sleep 10' for sleep 10 time_periods \n");
        exit();
    }
    if(argc > 2)
    {
        fprintf(2,"Too many arguments!\n");
        fprintf(2,"You should input only one argument\n");
        fprintf(2,"Eg: 'Sleep 10' for sleep 10 time_periods \n");
        exit();
    }
    int period = atoi(argv[1]);
    printf("(nothing happens for a little while)\n");
    sleep(period);
    exit();
}




