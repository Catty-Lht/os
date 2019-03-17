#include "stdio.h"
#include "sys/types.h"
#include "unistd.h"
#include "stdlib.h"

#define HASPRO -10
char *a;


int main()
{
    pid_t p1,p2,p3,p4,p5;

    int cnt=0;

    while((p1=fork()) == -1);

    if(!p1)
    {
        while((p2=fork()) == -1);

        if(!p2)
        {
            while ((p4=fork())==-1);
            if (!p4)
            {
                while(1)
                {	sleep(1);
                   printf(" p4  pid %d,  parent p2 pid %d\n",getpid(),getppid());
                    //wait(0);
                }
            }
            else
            {
                while ((p5=fork())==-1);

                if (!p5)
                {
                    while(1)
                    {sleep(1);
                        printf(" p5  pid %d,  parent p2 pid %d\n",getpid(),getppid());
                        //wait(0);
                    }

                }
                else
                {
                    ;
                }

            }

            while(1)
            {
				sleep(1);
                printf("p2  pid %d,  parent p1 pid %d\n",getpid(),getppid());
                //exit(0);
				printf("定义未初始化指针*a，制造段错误（数组中没有a[11]）：a[11]:%d\n",a[11]);
            }
        }
        else
        {
            while ((p3=fork())==-1);

                if (!p3)
                {
                    while(1)
                    {sleep(1);
                       printf("p3  pid %d,  parent p1 pid %d\n",getpid(),getppid());
                        //wait(0);
                    }

                }
                else
                {
                    ;
                }
        }

        while(1)
        {sleep(1);
           printf("p1  pid %d,  parent  pid %d\n\n",getpid(),getppid());
            //wait(0);
        }
    }
    
    return 0;
}

