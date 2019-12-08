#include "types.h"
#include "stat.h"
#include "user.h"

 
int
main(int argc, char **argv)
{
    if (argc < 2){
        printf(2, "usage: chpl num\n");
        exit();
    }


    changePolicy(atoi(argv[1]));
    
    exit();
            
}
