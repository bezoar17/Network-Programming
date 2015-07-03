/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
*************************/
//USE OF SIGNALs,-PID;
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h> 
#define READ 0
#define WRITE 1
#define SIZE 1048576
/* When a SIGUSR1 signal arrives, set this variable. */
volatile sig_atomic_t usr_interrupt = 0;
int semid,shmid,i;
sem_t * acc;int clients=0,flag=0;
char sem_acc[12]="ccc";
struct message *data,*point;
struct counter
{
	pid_t pidc;
	int msg_count;
};
struct counter all_msg[10];
struct message
{
	int type;
	int pid; //client's pid
	int slno; //incremented for every message
	int a; //any number
	int b; //any number
	int total;//total of a and b, processed by server
};
void SIGINT_handler()
{
	for(i=0;i<10;i++)
	{
		if(all_msg[i].pidc!=0)
			printf("PID: %d\tMessages: %d\n",all_msg[i].pidc,all_msg[i].msg_count);
	}
	semctl (semid,0,IPC_RMID);//creator of semaphore clears the semaphore.	
	shmdt(data);//clearing attachment from shared memory.
	shmctl(shmid, IPC_RMID, NULL);//clear the shared memory segment.
	sem_close(acc);
	sem_unlink(sem_acc); 
	exit(0);
}
void SIGUSR1_handler()
{
	if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
     usr_interrupt = 1;
     // sleep(1);
}
int main(int argc, char const *argv[])
{
	printf("\t\tServer with pid %d started\n",getpid());
	printf("\t\tRun client by opening a new terminal and running ./shmclient in current directory.\n\n");
	sigset_t mask, oldmask;
	sigemptyset (&mask);
	sigaddset (&mask, SIGUSR1);
 	if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
    if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
	int sem_cnt,k=0;
	for(i=0;i<10;i++)
			{
				all_msg[i].pidc=0;
				all_msg[i].msg_count=0;
			}
	key_t shmkey= ftok("shmclient.c",'3');
	key_t semkey= ftok("shmclient.c",'3');
	sem_unlink(sem_acc);
	if ((acc = sem_open("ccc", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
    perror("semaphore initilization");
    exit(1);
  	}	 
	struct message msg_server; struct message * msg_point=&msg_server;	
	if ((semid = semget(semkey, 1, IPC_CREAT | 0666 | IPC_EXCL)) == -1) 
    {
      perror("semget: semget failed"); 
      exit(1); 
    }
	if ((shmid = shmget (shmkey, SIZE, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
                   {
                   perror ("shmget: shmget failed");
                   exit (1);
                   }//creating shared memory segment of 1 KB with read and write permissions to the whole of Group processes.
    data = shmat (shmid, (void *) 0, 0);//getting the pointer to the shared memory. data is the pointer to the shared segment.
    if (data == (struct message *) (-1))
    perror ("shmat");//checking if shared memory attachment succeeded or not.
	point=data;
	if ((semctl (semid, 0, SETVAL,0)) == -1)
 					perror ("semctl()");
    sem_post(acc);
	while(1)
	{
		printf("Waiting for client to end writing.\n");
		//wait for client to give control to shmsn
		sigprocmask (SIG_BLOCK, &mask, &oldmask);
		 while (!usr_interrupt)
		   sigsuspend (&oldmask);
		 sigprocmask (SIG_UNBLOCK, &mask, NULL);
		 usr_interrupt = 0;
		printf("Waiting for access to shared memory\n"); 
		sem_wait(acc);
		printf("Got access to shared memory\n");
		memcpy(msg_point,data,sizeof(struct message));//reading		
		if(msg_server.type==1)
		{
			msg_server.total = msg_server.a + msg_server.b;
			msg_server.type = msg_server.pid;
			memcpy(data,msg_point,sizeof(struct message));//writing back			
			printf("SERVER (%d)  PID:%d  Sl.no: %d\tA: %d\tB: %d\tTOTAL: %d\tShmid: %d\tSem_Val: %d\n",getpid(),msg_server.pid,msg_server.slno,msg_server.a,msg_server.b,msg_server.total,shmid,1); 
			data++;
			sleep(0.5);		    
		}
		for(i=0;i<10;i++)
			{
				if(all_msg[i].pidc==msg_server.pid && msg_server.pid!=0) 
					{
						all_msg[i].msg_count++;flag=1;
					}
				
			}
		if(flag==0)
		{
			all_msg[clients].pidc=msg_server.pid;
			all_msg[clients++].msg_count=1;
		}
		if(msg_server.pid!=0)
			kill(msg_server.pid,SIGUSR1);	
		printf("Access given to client with pid %d\n",msg_server.pid);
	}
	return 0;
}