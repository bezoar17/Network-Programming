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
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
int main()
{
    int i,sem_cnt,semid,shmid;//declaring all integers.
    pid_t pid,proc[4];//pid_t declaration
    char s[1024];
    key_t key;//shm_key
    key_t key_sem;//sem_key
    char *data,*point;//pointers for the shared memory    
    key = ftok ("sem.c", 'Y');//random inputs for 2nd argument kind for a unique key generation.
    key_sem = ftok ("sem.c", 'X');//random inputs for 2nd argument kind for a unique key generation.
    printf("\n             /*************************\n             *  NetProg Lab Assign. 5 *\n             *     Vaibhav Kashyap    *\n             *      2012A3PS143P      *\n             *************************/\n");    
    proc[0]=getpid();//parent process   
    printf("************ %d is the Parent process ************\n",getpid());//Declaring parent process
    if ((semid = semget(key_sem, 1, IPC_CREAT | 0666 | IPC_EXCL)) == -1) 
    {
      perror("semget: semget failed"); 
      exit(1); 
    }//creating semaphore
    printf(">>>>>>>>>>>  Parent(%d) created semaphore with id %d  \n",getpid(),semid);    
    if ((semctl (semid, 0, SETVAL, 0)) == -1)
    perror ("semctl()");//setting semaphore value to 0    
    printf(">>>>>>>>>>>  Parent(%d) set semaphore value to 0  \n",getpid());    
    for(i=1;i<=3;i++)
    {
        if((pid=fork())<0)//forking 3 child processes. The code below ensures only master arrives at the fork statement.
        perror("fork");
        else if(pid!=0)
        {
            //parent
            proc[i]=pid;//registering the process id for parent's memory.
            printf("\tChild process %d registered to proc[%d].\n",pid,i);
            if(i!=3)continue;//create more process until no. of children is 3
            else       //children have been forked, change semaphore to 1 to signal child 1 (A) to start its work.
            {
                   printf("****************  End of forking  ******************\n");
                   if ((semctl (semid, 0, SETVAL, 1))== -1)                    
                   perror ("semctl()"); //semaphore value changed to 1. Process A will start its progress.
                   printf(">>>>>>>>>>>  Parent(%d) set semaphore value to 1  \n",getpid());
                   while(1)
                   {
                     sem_cnt = semctl (semid, 0, GETVAL);
                     if(sem_cnt==0)
                     break;
                   }//parent waits for semaphore to be 0 again to resume its work.
                   kill(proc[3],SIGKILL);//end the child processes.
                   kill(proc[2],SIGKILL);//end the child processes.
                   semctl (semid,0,IPC_RMID);//creator of semaphore clears the semaphore.
		               printf(">>>>>>>>>>>  Parent cleared semaphore and killed children. \n");               
                   while (pid = waitpid(-1, NULL, 0))  //waits for all the child processes to complete
                    {
                     if (errno == ECHILD) 
                     exit(0);//parent exits after children are done.
                    }
            }     
        }
        else
        {       
               //child
               if(i==1)
               {
                   //process A
                   while(1)
                   {
                     sem_cnt = semctl (semid, 0, GETVAL);
                     if(sem_cnt==1)
                      break;
                   }//waits for semaphore value to be 1 to start executing.
                   if ((shmid = shmget (key, 1024, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
                   {
                   perror ("shmget: shmget failed");
                   exit (1);
                   }//creating shared memory segment of 1 KB with read and write permissions to the whole of Group processes.
                   printf(">>>>>>>>>>>  Process A(%d) created memory with id %d  \n",getpid(),shmid);
                   data = shmat (shmid, (void *) 0, 0);//getting the pointer to the shared memory. data is the pointer to the shared segment.
                   if (data == (char *) (-1))
                   perror ("shmat");//checking if shared memory attachment succeeded or not.
                   printf("\n****************** Enter String ********************\n");
                   while(1)//Start looping and take input and write until Ctrl+D is pressed.
                   {
                       point=data;//duplicating the pointer.
                       while(fgets(s,1024,stdin))//taking input from stdin
                       {
                         if (s[strlen(s)-1] == '\n')
                             s[strlen(s)-1] = '\0';//Legitmate string.
                         //Do your work
                         sprintf(data, "%s\n",s);//writing to the shared memory.
                         printf(">>>>>>>>>>>  Process A(%d) wrote to memory    \n",getpid());
                         if((semctl (semid, 0, SETVAL, 2))==-1)//sets semaphore to 2 to let process B know its time for C to start.
                         perror("semctl()");
                         printf(">>>>>>>>>>>  Process A(%d) set semaphore value to 2  \n",getpid());
                       }
                       if(feof(stdin))// Ctrl+D is EOF for stdin. feof is set if EOF is encountered in stdin.
                       {
                         printf("            ----------------------\n");
                         printf("            | Ctrl+D pressed in A|\n");
                         printf("            ----------------------\n");
                         shmdt(data);//clearing attachment from shared memory.
                         shmctl(shmid, IPC_RMID, NULL);//clear the shared memory segment.                         
                         if((semctl (semid, 0, SETVAL, 0))==-1)//signal parent to free semaphore
                         perror("semctl()");
                         printf(">>>>>>>>>>>  Process A(%d) set semaphore value to 0  \n",getpid());
                         exit(0);
                       }                         
                       while(1)
                       {
                         sem_cnt = semctl (semid, 0, GETVAL);
                         if(sem_cnt==1)
                          break;
                       }//stops for B and C to complete their work(semaphore becomes 1) and then loops.                       
                   }
               }
               if(i==2)
               {
                  //process B
                   while(1)
                   {
                     sem_cnt = semctl (semid, 0, GETVAL);
                     if(sem_cnt==2)
                      break;
                   }//waits for A to create shared memory and then change semaphore to 2 for B to get access.                                     
                   if ((shmid = shmget (key, 1024, 0666)) == -1)//gets the shared memory id with the same key which every child got after forking.
                   {
                   perror ("shmget: shmget failed");
                   exit (1);
                   }
                   data = shmat (shmid, (void *) 0, 0);//B attaches the shared memory segment to its address space.
                   if (data == (char *) (-1))
                   perror ("shmat");                   
                   while(1)//Looping until killed.
                   {
                      while(1)//waits for semaphore to be 2 so that it knows its time.
                       {
                         sem_cnt = semctl (semid, 0, GETVAL);
                         if(sem_cnt==2)
                          break;
                       }
                      //do your work
                      for (point=data; *point!='\0'; point++)//duplicate pointer point changes its value for access to all chars.
                         {  
                          if((int)(*point) >=97 && (int)(*point)<=122)
                              *point=(char)(((int)*point)-32); //reads the shared memory and replaces all lowercase to uppercase.                       
                         }
                      printf(">>>>>>>>>>>  Process B(%d) changed string     \n",getpid());
                      if((semctl (semid, 0, SETVAL, 3))==-1)//sets semaphore to 3 to let process C know its time for C to start.
                      perror("semctl()");
                      printf(">>>>>>>>>>>  Process B(%d) set semaphore value to 3  \n",getpid());                      
                   }
                 exit(0);
               }
               if(i==3)
               {
                  //process C
                  while(1)
                   {
                     sem_cnt = semctl (semid, 0, GETVAL);
                     if(sem_cnt==3)
                      break;
                   }//waits for B to change the shared memory and then change semaphore to 3 for C to start printing.                                                                           
                   if ((shmid = shmget (key, 1024, 0666)) == -1)
                   {
                   perror ("shmget: shmget failed");
                   exit (1);
                   }
                   data = shmat (shmid, (void *) 0, 0);//C attaches the shared memory segment to its address space.
                   if (data == (char *) (-1))
                   perror ("shmat");
                   while(1)//Looping until killed.
                   {
                      while(1)//waits for semaphore to be 2 so that it knows its time.
                       {
                         sem_cnt = semctl (semid, 0, GETVAL);
                         if(sem_cnt==3)
                          break;
                       }
                        //do your work
                       printf(">>>>>>>>>>>  Process C(%d) is going to print  \n",getpid());
                       printf("%s",data);//prints to stdout with pointer.                        
                       if((semctl (semid, 0, SETVAL, 1))==-1)//sets semaphore to 1 to let process A start taking the input for the next cycle.
                       perror("semctl()");
                       printf(">>>>>>>>>>>  Process C(%d) set semaphore value to 1  \n",getpid()); 
                       printf("\n****************** Next String ********************\n");                                     
                   }
                   exit(0);
               }
            break;//if child comes here, stops it from creating further child processes.
        }
    }   
    return 0;
}
