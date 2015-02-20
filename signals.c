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
#define READ 0
#define WRITE 1
//global variables
int total=0,l,time=0;//time for each process.
pid_t master_pid,proc[4];//parent's pid and array of pid's;
void SIGUSR2_handler(int signum)
{ 
        printf("   Child[3] with PID %d    received SIGUSR2[%d]\n",getpid(),signum);//confirmation of receiveing the signal
        if((kill(master_pid,SIGINT))!=0)
        printf("\n\t#######  Error Sending signal to master from child %d\n", getpid());  
}
void SIGUSR1_handler(int signum)
{
        if(getpid()==proc[1]) 
            printf("   Child[1] with PID %d    received SIGUSR1[%d]\n",getpid(),signum);//confirmation of receiveing the signal
        else 
            printf("   Child[2] with PID %d    received SIGUSR1[%d]\n",getpid(),signum);//confirmation of receiveing the signal
        if((kill(master_pid,SIGINT))!=0)       
        printf("\n\t#######  Error Sending signal to master from child %d\n", getpid());   
}
void SIGALRM3_handler(int signum)
{
    if (signal(SIGALRM, SIGALRM3_handler) == SIG_ERR) {			//resetting handler
          printf("\n\t#######  SIGALRM install error in 3\n");
          exit(1);
     } 
    alarm(9);//setting the cycle of signals
    time=time+9;//every process stores its time.
    if((kill(proc[1],0))==0)//checking if processes hasn't died. 0 means can send signals to it.
    {
        if((kill(proc[2],0)!=0))  printf("\t\t Time elapsed : %d secs.\n",time);//prints its time only if previous processes have exited.
        printf("\t      Sent SIGUSR1 from 3 to 1\n");//Sending the signals to the brother's.
        if ((kill(proc[1],SIGUSR1))!=0) 
        printf("\n\t#######  Error sending signal from process 3\n");
    } 
}
void SIGALRM2_handler(int signum)
{
    if (signal(SIGALRM, SIGALRM2_handler) == SIG_ERR) {			//resetting handler
          printf("\n\t#######  SIGALRM install error in 2\n");
          exit(1);
     } 
    alarm(6);//setting the cycle of signals
    time=time+6;//every process stores its time.
    if((kill(proc[3],0))==0)//checking if processes hasn't died. 0 means can send signals to it.
    {
        if((kill(proc[1],0)!=0))  printf("\t\t Time elapsed : %d secs.\n",time);//prints its time only if previous processes have exited.
        printf("\t      Sent SIGUSR2 from 2 to  3\n");//Sending the signals to the brother's.
        if ((kill(proc[3],SIGUSR2))!=0)
        printf("\n\t#######  Error sending signal from process 2\n");
    }  
}
void SIGALRM1_handler(int signum)
{
    if (signal(SIGALRM, SIGALRM1_handler) == SIG_ERR) {			//resetting handler
          printf("\n\t#######  SIGALRM install error in 1\n");
          exit(1);
     } 
    alarm(3);//setting the cycle of signals
    time=time+3;//every process stores its time.
    if((kill(proc[2],0))==0)//checking if processes hasn't died. 0 means can send signals to it.
    {
         printf("\t\t Time elapsed : %d secs.\n",time);//prints its time only if previous processes have exited.
         printf("\t      Sent SIGUSR1 from 1 to  2\n");//Sending the signals to the brother's.
         if ((kill(proc[2],SIGUSR1))!=0)
         printf("\n\t#######  Error sending signal from process 1\n");

    }    
}
void SIGALRM_handler(int signum)
{
    printf("\n*************** Starting Signaling *****************\n");//Start signalling after write has been performed in parent.
    printf("***************--------------------*****************\n\n");
    if (signal(SIGALRM, SIGALRM_handler) == SIG_ERR) {			//resetting handler
          printf("\n\t#######  SIGALRM install error in master\n");
          exit(1);
     } 
}
void SIGINT_handler(int signum)
{
         total++;//only parent has to deal with total.
         printf("   --------------> The Total is %d <--------------\n",total);
         if(total-l==0)
         {
            if ((kill(proc[1],SIGTERM))!=0)   							//sending SIGTERM to respective child. 
            printf("\n\t#######  Error in stopping child process 1\n");
         }
         if(total-l==3)
         {
            if ((kill(proc[2],SIGTERM))!=0)    							//sending SIGTERM to respective child.
            printf("\n\t#######  Error in stopping child process 2\n");
            if ((kill(proc[3],SIGTERM))!=0)    							//sending SIGTERM to respective child.
            printf("\n\t#######  Error in stopping child process 3\n");
         }
         if(signal(SIGINT, SIGINT_handler) == SIG_ERR) {      //resetting handler
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
         }
}
void SIGTERM_handler(int signum)
{
    if (signal(SIGTERM, SIGTERM_handler) == SIG_ERR) {			//resetting handler
          printf("\n\t#######  SIGTERM install error\n");
          exit(1);
     } 
     printf("         ---------------------------------\n");
     printf("         |   Child process %d Exiting |\n",getpid());
     printf("         ---------------------------------\n");
     exit(0);//Child exits.

}
int main()
{
    int p1[2];int p2[2];int i;//initializing
    printf("\n             /*************************\n             *  NetProg Lab Assign. 3 *\n             *     Vaibhav Kashyap    *\n             *      2012A3PS143P      *\n             *************************/\n");
    printf("\n\t     \tEnter the value of L\n");
    scanf("%d",&l);//taking value of l.
    pid_t pid;
    proc[0]=getpid();
    master_pid=getpid();//parent process
    if (pipe(p1) == -1)//creating two pipes
    {
    perror("\n\t#######  Pipe 1 creation failed");
    exit(1);
    } 
    if (pipe(p2) == -1)//creating two pipes
    {
    perror("\n\t#######  Pipe 2 creation failed");
    exit(1);
    }
    // INSTALLING SIGNAL HANDLERS
    if (signal(SIGALRM, SIGALRM_handler) == SIG_ERR) {	
          printf("\n\t#######  SIGALRM install error\n");
          exit(1);
     } 
    if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
    if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGUSR1 install error\n");
          exit(1);
     }
     if (signal(SIGUSR2, SIGUSR2_handler) == SIG_ERR) {
          printf("\n\t#######  SIGUSR2 install error\n");
          exit(1);
     }
     if (signal(SIGTERM, SIGTERM_handler) == SIG_ERR) {
          printf("\n\t#######  SIGTERM install error\n");
          exit(1);
     }          
    printf("************ %d is the Master process ************\n",getpid());//Declaring parent process
    for(i=1;i<=3;i++)
    {
        if((pid=fork())<0)//forking 3 child processes. The code below ensures only master arrives at the fork statement.
        perror("fork");
        else if(pid!=0)
        {
            //parent
            proc[i]=pid;//registering the process id for child's memory for 3rd child to read directly.
            printf("\tChild process %d registered to proc[%d].\n",pid,i);
            close(p1[READ]);//closing the read end for the parent.            
            if(i!=3)continue;//create more process until no. of children is 3
            else			 //start writing to child 1 and 2 after array of pid is initialized completely.
            {
                printf("****************  End of forking  ******************\n");
                if(!((write(p1[WRITE],proc,sizeof(proc)))>0)) //writing to pipe 1
                        printf("\n\t#######  Error in writing to Pipe 1\n");
                close(p1[WRITE]);
                if(!((write(p2[WRITE],proc,sizeof(proc)))>0)) //writing to pipe 2
                        printf("\n\t#######  Error in writing to Pipe 2\n");   
                close(p2[WRITE]); 
                sleep(.5);//waiting slightly for read to complete
                raise(SIGALRM);//start the signalling.            
                while (pid = waitpid(-1, NULL, 0))  //waits for the child processes to complete
                 {
                   if (errno == ECHILD) 
                   {
                      printf("            ----------------------\n");
                      printf("            |   Adios -by Parent |\n");
                      printf("            ----------------------\n");
                      exit(0);//parent exits after children are done.
                   }
                }
            }     
        }
        else
        {       
            //child
                if(i==1) //first child
                {
                    if (signal(SIGALRM, SIGALRM1_handler) == SIG_ERR) {    	//INSTALLING SIGNAL HANDLERS FOR CHILD.
                      printf("\n\t#######  SIGALRM install error in 1\n");
                      exit(1);
                    }
                    proc[i]=getpid();//registering the process id for child's memory for 3rd child to read directly.
                    close(p1[WRITE]);
                    if(!((read(p1[READ],proc,sizeof(proc)))>0))   //reading from respective pipes the array of pid's.
                       printf("\n\t#######  Error in reading from Pipe 1\n");
                    close(p1[READ]);
                    alarm(3);//initial call for the start of signalling.
                    while(1);//keeping child alive.
                }
                if(i==2) //second child
                {
                    if (signal(SIGALRM, SIGALRM2_handler) == SIG_ERR) {    	//INSTALLING SIGNAL HANDLERS FOR CHILD.
                      printf("\n\t#######  SIGALRM install error in 2\n");
                      exit(1);
                    }
                    proc[i]=getpid();//registering the process id for child's memory for 3rd child to read directly.
                    close(p2[WRITE]);                   
                    if(!((read(p2[READ],proc,sizeof(proc)))>0))   //reading from respective pipes the array of pid's.
                        printf("\n\t#######  Error in reading from Pipe 2\n");
                    close(p2[READ]);
                    alarm(6);//initial call for the start of signalling.
                    while(1);//keeping child alive.
                }
                if(i==3) //third child
                {
                    if (signal(SIGALRM, SIGALRM3_handler) == SIG_ERR) {    	//INSTALLING SIGNAL HANDLERS FOR CHILD.
                      printf("\n\t#######  SIGALRM install error in 3\n");
                      exit(1);
                    }
                    proc[i]=getpid();//registering the process id for child's memory for 3rd child to read directly.
                    alarm(9);//initial call for the start of signalling.
                    while(1);//keeping child alive.                    
                }
            break;//if child comes here, stops it from creating further child processes.
        }
    }   
    return 0;
}