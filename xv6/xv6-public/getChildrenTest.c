#include "types.h"
#include "stat.h"
#include "user.h"
 
int
main(void)
{
//   printf(1, "My first xv6 program    %d\n" , getChildren(getppid()));

    int pid = fork();

    if (pid>0){
        // wait();

        int pid1 = fork();
        if (pid1>0){
            // wait();
            printf(1, "pid:    %d\n" , getpid());
            printf(1, "parent pid:    %d\n" , getppid());
            printf(1, "children:   %d\n" , getChildren(getpid()));

        }else if (pid1==0){
            printf(1, "pid:    %d\n" , getpid());
            printf(1, "parent pid:    %d\n" , getppid());
            exit();
        }

    }else if (pid==0){
        printf(1, "pid:    %d\n" , getpid());
        printf(1, "parent pid:    %d\n" , getppid());
        exit();
    }
    exit();
}
