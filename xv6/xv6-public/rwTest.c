#include "types.h"
#include "stat.h"
#include "user.h"


#define NCHILD 10

void testReaderWriters(int* , int);


int
main(int argc, char **argv)
{
    char argv_in[100];

    printf(1 , "enter pattern for readers/writers test\n");
    int read_size = read(0 , argv_in, sizeof(argv_in));
    argv_in[read_size - 1] = '\0';
    int pattern[100], i;
    for (i=0; argv_in[i+1] != '\0'; i++){
        if (argv_in[i+1] == '0')
            pattern[i] = 0;
        else if (argv_in[i+1] == '1')
            pattern[i] = 1;
        else{
            printf(1, "pattern must be consist of only 0 and 1\n");
            exit();
        }
    }

    testReaderWriters(pattern, i);
    exit();
    return 0;
            
}

void testReaderWriters(int* pattern, int pattern_size){
    int pid, i;
    rwinit();

    pid = fork();
    for (i=1 ; i<pattern_size; i++){
        if (pid < 0){
            printf(1, "fork failed\n");
            exit();
        }
        else if (pid > 0)
            pid = fork();
        else
            break;
    }

    if (pid < 0){
        printf(1 , "fork failed\n");
        exit();
    }
    else if (pid == 0){
        printf(1, "child adding to shared counter\n");
        int res = rwtest(pattern[i-1]);
        if (pattern[i-1] == 0)
            printf(2, "reader from shared counter : %d\n", res);
        else if (pattern[i-1] == 1){
            printf(2, "writer from shared counter\n");
        }
        

        
    }
    else{
            for (i=0; i< pattern_size; i++){
                wait();
            }
            printf(1, "user program finished\n");
            int res = rwtest(0);
            printf(1, "last value of shared counter: %d\n", res);
        }

    exit();

}
