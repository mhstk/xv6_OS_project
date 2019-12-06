#include "types.h"
#include "stat.h"
#include "user.h"

 
int
main(void)
{

    int pid = fork();

    if (pid>0){


        int pid1 = fork();
        if (pid1>0){

            printf(1, "pid:    %d\n" , getpid());
            printf(1, "parent pid:    %d\n" , getppid());
            printf(1, "children:   %d\n" , getChildren(getpid()));
            wait();

        }else if (pid1==0){
            int i;
            int k=0;
            for (i=0 ; i<1000000000; i++){
                k = k*2;
            }
            printf(1, "pid:    %d\n" , getpid());
            printf(1, "parent pid:    %d\n" , getppid());
            exit();
        }
        wait();

    }else if (pid==0){
        int i;
        int k=0;
        for (i=0 ; i<1000000000; i++){
            k = k*2;
        }
        printf(1, "pid:    %d\n" , getpid());
        printf(1, "parent pid:    %d\n" , getppid());
        exit();
    }
    exit();
}
