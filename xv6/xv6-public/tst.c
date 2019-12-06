#include "types.h"
#include "stat.h"
#include "user.h"

 
int
main(int argc, char **argv)
{
    if (argc < 2){
        printf(2, "usage: tst num\n");
        exit();
    }

    int num = atoi(argv[1]);
    int i;
    for ( i = 0; i < num; i++)
    {
        int pid = fork();

        if (pid > 0){

        }else if (pid == 0){
            int j;
            int k = 1;
            for ( j = 0; j < 100000000000; j++)
            {
                k = k * 2 + i * 3.14;
            }
            exit();
            
        }
    }
    


    
    

    // printf(1, "count:    %d\n" , getCount(atoi(argv[1])));
    exit();
            
}
