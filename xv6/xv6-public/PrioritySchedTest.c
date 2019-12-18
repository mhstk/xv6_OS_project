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

    int gavcbt[6];   
    int gavtt[6];  
    int gavwt[6]; 

int
main(int argc, char **argv)
{

    int numOfChild = 25;

  
    int pids[numOfChild];
    int prio[numOfChild];
    int cbt[numOfChild];
    int tt[numOfChild];
    int wt[numOfChild];
    int c[numOfChild];
    int s[numOfChild];
    int t[numOfChild];
    int red[numOfChild];
    int ppid;


    int j;
    for ( j = 0; j < numOfChild; j++)
    {
        ppid = getpid();
        int pid = fork();


        if (pid == 0) {
            int num = getpid() - getppid();
            if (num <= 5){
                changePriority(getpid() , 5);


            }
            if (num > 5 && num <= 10){
                changePriority(getpid() , 4);

            }
            if (num >10 && num <= 15){
                changePriority(getpid() , 3);

            }
            if (num > 15 && num <= 20){
                changePriority(getpid() , 2);

            }
            if (num > 20 && num <= 25){
                changePriority(getpid() , 1);

            }
            int i;
            for ( i = 0; i < 500; i++)
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

    int k=0;
    for (k = 0; k < numOfChild; k++)
    {
            int num = pids[k] - ppid;
            if (num <= 5){
                prio[k] = 5;
                gavcbt[5] += cbt[k];
                gavtt[5] += tt[k];
                gavwt[5] += wt[k];
            }
            if (num > 5 && num <= 10){
                prio[k] = 4;
                gavcbt[4] += cbt[k];
                gavtt[4] += tt[k];
                gavwt[4] += wt[k];

            }
            if (num > 10 && num <= 15){
                prio[k] = 3;
                gavcbt[3] += cbt[k];
                gavtt[3] += tt[k];
                gavwt[3] += wt[k];

            }
            if (num > 15 && num <= 20){
                prio[k] = 2;
                gavcbt[2] += cbt[k];
                gavtt[2] += tt[k];
                gavwt[2] += wt[k];

            }
            if (num > 20 && num <= 25){
                prio[k] = 1;
                gavcbt[1] += cbt[k];
                gavtt[1] += tt[k];
                gavwt[1] += wt[k];
            }
        
    }

        

    printf(1, "pid\tpri\tcbt\tc\tt\tred\ts\ttt\twt\n");
    // printf(1, "pid\tpri\tcbt\ttt\twt\n");

    for ( j = 0; j < numOfChild; j++)
    {
        printf(1, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", pids[j],prio[j], cbt[j], c[j] ,t[j], red[j], s[j] , tt[j], wt[j]); 
        // printf(1, "%d\t%d\t%d\t%d\t%d\n", pids[j],prio[j], cbt[j],tt[j], wt[j]); 
    }  

    for (k=1 ; k<= 5; k++)
    {
        gavcbt[k] /= (numOfChild/5);
        gavtt[k] /= (numOfChild/5);
        gavwt[k] /= (numOfChild/5);
        printf(1,"\n");
        printf(1,"priority %d:\n" , k);
        printf(1,"avcbt: %d\tavtt: %d\tavwt: %d\n" , gavcbt[k], gavtt[k], gavwt[k]);
    }


    for (j = 0; j < numOfChild; j++)
    {
        avwt += wt[j];
        avtt += tt[j];
        avcbt += cbt[j];

    }

    avcbt /= numOfChild;
    avtt /= numOfChild;
    avwt /= numOfChild;

    printf(1, "\nAll:\n");
    printf(1,"avcbt: %d\tavtt: %d\tavwt: %d\n" , avcbt, avtt, avwt);
    



    exit() ;    
            
}
