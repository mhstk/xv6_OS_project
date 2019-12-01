#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sumOfEvenDigits(char s[]){
    int i;
    int sum=0;
    for(i=1 ; i<strlen(s) ; i+=2){
        sum += s[i] - 48 ;
    }
    return sum;

}


int main(){

    int fd[2]; 


    char input[100];




    scanf("%s" , input) ;






    if (pipe(fd)==-1) 
    { 
        printf("Pipe Failed\n" ); 
        return 1; 
    } 


    int pid = fork();
    if (pid == 0){ // child
        
        char ans[100];
        char buff[150] = "The sum of even digits in the input number :";
 
        //save answer (int) to  a string (ans[])
        
        int s = sumOfEvenDigits(input);
        int i=0;
        for(i=0; s != 0 ; i++){
            
            char c = s%10 + 48;
            ans[i] = c;
            s = s / 10;
        }

        //concating string and answer

        int k = strlen(buff);
        i = strlen(ans);
        int j;
        for (j=i-1; j>=0 ; j-- ){
            buff[k] = ans[j];
            k++;
        }

        close(fd[0]);

        write(fd[1] , buff, 150);
        close(fd[1]);

        exit(0);
            
    }else if (pid > 0){ //parent
        char final[100];
        wait(NULL);
        

        read( fd[0] , final , 150);
        close(fd[0]);

        printf("%s\n" , final);
        return(0);
    }else{
        return 1;
    }


}