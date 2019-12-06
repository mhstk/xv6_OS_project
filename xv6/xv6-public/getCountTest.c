#include "types.h"
#include "stat.h"
#include "user.h"

 
int
main(int argc, char **argv)
{
    if (argc < 2){
        printf(2, "usage: getCountTest num\n");
        exit();
    }

    int num = 7 , i=0;
    //invoke num times
    for (; i < num; i++)
    {
        getppid();      //systemcall number: 22,
    }
    
    

    printf(1, "count:    %d\n" , getCount(atoi(argv[1])));
    exit();
            
}
