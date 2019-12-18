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

    int numOfChild = 10;
    int pids[numOfChild];
    int cbt[numOfChild];
    int tt[numOfChild];
    int wt[numOfChild];
    int c[numOfChild];
    int s[numOfChild];
    int t[numOfChild];
    int red[numOfChild];


    int j;
    for ( j = 0; j < numOfChild; j++)
    {
        int pid = fork();

        if (pid == 0) {
            int i;
            for ( i = 0; i < 1000; i++)
            {
                printf(1, "%d: %d\n" , getpid() , i);
            }
            exit();

        }
    }  

    int avwt = 0;
    int avtt = 0;
    int avcbt = 0;


    for ( j = 0; j < numOfChild; j++)
    {
        struct timeVariables times;
        pids[j] = waitForChild(&times);
        c[j] = times.creationTime;
        s[j] = times.sleepingTime;
        t[j] = times.terminationTime  ;
        cbt[j] = times.runningTime;
        red[j] = times.readyTime;
        tt[j] = t[j] - c[j];
        wt[j] = t[j] - c[j] - cbt[j];

    }   

    printf(1, "pid\tcbt\tc\tt\tred\ts\ttt\twt\n");
    // printf(1, "pid\tcbt\ttt\twt\n");

    for ( j = 0; j < numOfChild; j++)
    {
        printf(1, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", pids[j], cbt[j], c[j] ,t[j], red[j], s[j] , tt[j], wt[j]); 
        // printf(1, "%d\t%d\t%d\t%d\n", pids[j], cbt[j],tt[j], wt[j]); 
    }  


    for (int j = 0; j < numOfChild; j++)
    {
        avwt += wt[j];
        avtt += tt[j];
        avcbt += cbt[j];

    }

    avcbt /= numOfChild;
    avtt /= numOfChild;
    avwt /= numOfChild;

    printf(1,"avcbt: %d\tavtt: %d\tavwt: %d\n" , avcbt, avtt, avwt);
    



    exit() ;    
            
}
