#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
 int main ()   
{   
    pid_t fpid; //fpid表示fork函数返回的值  
    
    fpid=fork();   
    if (fpid < 0)   
        printf("error in fork!");   
    else if (fpid == 0) 
	{  
        printf("i am the child process, my process id is %d\n",getpid());   
      	int ret ;
		ret = execl("/usr/bin/vi","vi","test.txt",NULL);
		if(ret=-1)
		perror("execl");
      
    }  
    else {  
		while(1)
        printf("i am the parent process, my process id is %d\n",getpid());   
       
      
    }  
  
    return 0;  
}   
