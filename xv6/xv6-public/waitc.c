#include "types.h"
#include "stat.h"
#include "user.h"


struct timeVariables {
  uint creationTime;
  uint terminationTime;
  uint sleepingTime;
  uint readyTime;
  uint runningTime;
};

 
int
main(int argc, char **argv)
{


    struct timeVariables times;


    int id = fork();

    if (id > 0){

        int pid = waitForChild(&times);

        printf(1, "pid : %d\tppid : %d\n" , pid , getpid());
        printf(1, "cTime\truTime\treTime\tsTime\ttTime\n");
        printf(1, "%d\t%d\t%d\t%d\t%d\n" , times.creationTime,times.runningTime,times.readyTime,times.sleepingTime,times.terminationTime);

    }else if (id == 0){
        int i=0;
        float dummy = 1.0;
        for (i=0;i<10000000;i++){
            dummy += 3.14 * i * i;
        }
        exit();
    }



    // int pid = waitForChild(&times);

    // printf(2, "pid : %d\tppid : %d\n" , pid , getpid());
    // printf(2, "cTime\truTime\treTime\tsTime\ttTime\n");
    // printf(2, "%d\t%d\t%d\t%d\t%d\n" , times.creationTime,times.runningTime,times.readyTime,times.sleepingTime,times.terminationTime);
    // // changePolicy(atoi(argv[1]));


    
    exit();
            
}
