/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
*************************/
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#define READ 0
#define WRITE 1
void SIGUSR1_handler(int signum)
{
         if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGUSR1 install error\n");
          exit(1);
        }
}  
int main()
{
    int pfd[2],i;
    pid_t pid,proc[3],lite;
    char s[1024];
    printf("\n             /*************************\n             *  NetProg Lab Assign. 4 *\n             *     Vaibhav Kashyap    *\n             *      2012A3PS143P      *\n             *************************/\n");    
    lite=proc[0]=getpid();//parent process   
    if (pipe(pfd) == -1)//creating pipe
    {
    perror("\n\t#######  Pipe creation failed");
    exit(1);
    }
    // INSTALLING SIGNAL HANDLERS
    if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGUSR1 install error\n");
          exit(1);
     }       
    printf("************ %d is the Parent process ************\n",getpid());//Declaring parent process
    for(i=1;i<=2;i++)
    {
        if((pid=fork())<0)//forking 2 child processes. The code below ensures only master arrives at the fork statement.
        perror("fork");
        else if(pid!=0)
        {
            //parent
            proc[i]=pid;//registering the process id for parent's memory.
            printf("\tChild process %d registered to proc[%d].\n",pid,i);
            if(i!=2)continue;//create more process until no. of children is 2
            else       //start writing and reading in parent after both children have been forked.
            {
                   printf("****************  End of forking  ******************\n");
                   printf("\n****************** Enter String ********************\n");
                   while(1)
                   {
                       while(fgets(s,1024,stdin))
                       {
                         if (s[strlen(s)-1] == '\n')
                             s[strlen(s)-1] = '\0';//Legitmate string.
                         printf("*******  Parent (%d) -----> Child-1(%d)  *******\n\t\t       %s\n\n",getpid(),proc[1],s);//writing to child 1
                         if(!((write(pfd[WRITE],s,sizeof(s)))>0))
                            perror("write()");                   
                         kill(proc[1],SIGUSR1);//informing child 1 to start reading.
                         pause();//waits for child 1 to read the given string and then write back to parent.
                         if(!((read(pfd[READ],s,sizeof(s)))>0))
                          printf("errno in read from parent\n");//reading string written by child 1.
                         printf("*******  Parent (%d) -----> Child-2(%d)  *******\n\t\t       %s\n\n",getpid(),proc[2],s); //writing to child 2                      
                         if(!((write(pfd[WRITE],s,sizeof(s)))>0))
                            perror("write()");
                         kill(proc[2],SIGUSR1);//informing child 2 to start reading.
                         pause();//waits for child 2 to read the given string and then write back to parent.
                         if(!((read(pfd[READ],s,sizeof(s)))>0))
                          printf("errno in read from parent\n");//reading string written by child 2.
                         printf("****************** Final String ********************\n\t\t       %s\n",s);
                         printf("\n****************** Next String *******************\n");
                       }
                       if(feof(stdin))// Ctrl+D is EOF for stdin. feof is set if EOF is encountered in stdin.
                       {
                         printf("            ----------------------\n");
                         printf("            |   Adios -by Parent |\n");
                         printf("            ----------------------\n");
                         kill(proc[2],SIGKILL);
                         kill(proc[1],SIGKILL);
                         exit(0);
                       }              
                   }
            }     
        }
        else
        {       
            //child
               while(1)
                    {
                      pause();//waits for parent to write.
                      read(pfd[READ],s,sizeof(s));
                      if(i==1)
                        strcat(s,"C1");//concatenation acc. to child no.
                      else
                        strcat(s,"C2");//concatenation acc. to child no.
                      printf("*******  Child-%d(%d) -----> Parent (%d)  *******\n\t\t       %s\n\n",i,getpid(),getppid(),s);
                      if(!((write(pfd[WRITE],s,sizeof(s)))>0))//writing to parent after concatenation.
                        perror("write()");
                      kill(getppid(),SIGUSR1);//informs parent for it to start its execution.
                    }
            break;//if child comes here, stops it from creating further child processes.
        }
    }   
    return 0;
}
