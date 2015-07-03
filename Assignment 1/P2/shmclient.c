/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
*************************/
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
struct tracker
{
  int offset;
};
struct message
{
  int type;
  int pid; //client's pid
  int slno; //incremented for every message
  int a; //any number
  int b; //any number
  int total;//total of a and b, processed by server
};
void SIGUSR1_handler()
{
  if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
     usr_interrupt = 1;
     //sleep(1);
}
int main(int argc, char const *argv[])
{
  printf("\t\t Client pid is %d \n",getpid());
  if (signal(SIGUSR1, SIGUSR1_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
  struct tracker track[100];
  sigset_t mask, oldmask;
  sigemptyset (&mask);
  sigaddset (&mask, SIGUSR1);
  int i=0,shmid,semid,sem_cnt,msg_count=0;pid_t server;
  sem_t * acc;
  key_t shmkey= ftok("shmclient.c",'3');
  key_t semkey= ftok("shmclient.c",'3');
  struct shmid_ds shm_check;
  char *data;//pointers for the shared memory  
  struct message msg_client; struct message * msg_point,*point;
  if ((semid = semget(semkey, 1, 0666)) == -1) 
    {
      perror("semget: semget failed"); 
      exit(1); 
    }//getting semaphore
  if ((shmid = shmget (shmkey, SIZE, 0666)) == -1)
                   {
                   perror ("shmget: shmget failed");
                   exit (1);
                   }//getting shmid
  msg_point = shmat (shmid, (void *) 0, 0);//getting the pointer to the shared memory. data is the pointer to the shared segment.
  point=msg_point;
  if (msg_point == (struct message *) (-1))
    perror ("shmat");//checking if shared memory attachment succeeded or not.
  if(shmctl(shmid,IPC_STAT,&shm_check)==-1)
    {perror("shmctl");exit(1);}
  server=shm_check.shm_cpid;
  if ((acc = sem_open("ccc", O_CREAT)) == SEM_FAILED) {
    perror("semaphore initilization");
    exit(1);
    }  
  while(1)
  {
    char check;
    printf("For writing to memory press y , to exit press n\n");
    scanf("%c",&check);   
    if(check=='n')
      break;
    else if (check=='y')
    { 
        printf("Waiting for access to shared memory\n");
        sem_wait(acc);
        printf("Got access to shared memory\n");
        printf("\t Enter any number for a\n");scanf("%d",&(msg_client.a));
        printf("\t Enter any number for b\n");scanf("%d",&(msg_client.b));
        check=getchar();
        msg_client.type=1;//Constant properties for msg.
        msg_client.pid=getpid();//Constant properties for msg.
        sem_cnt = semctl (semid, 0, GETVAL);
        msg_point=point+sem_cnt;
        msg_client.slno=sem_cnt+1;        
        track[msg_count].offset=(msg_point-point);
        memcpy(msg_point,&msg_client,sizeof(struct message));
        printf("\t\t Written message no. %d to shared memory\n",sem_cnt+1);
        msg_count++;
        if ((semctl (semid, 0, SETVAL,sem_cnt+1)) == -1)
              perror ("semctl()");
        //leave acc of shm
        sem_post(acc);        
    }
    else
    {
      printf("Try Again\n");
    } 
  }
  kill(server,SIGUSR1); //acc to server
  printf("\t\t Given acc to server %d\n",server);
  while(1)
  {
    if(msg_count==0)
      {
        shmdt(msg_point);//clearing attachment from shared memory.
        printf("\t\t All messages printed , exiting\n"); 
        sem_close(acc);
        exit(0);  
      }
    printf("Waiting for return of access from server\n");
    //WAIT FOR READ acc
    sigprocmask (SIG_BLOCK, &mask, &oldmask);
     while (!usr_interrupt)
       sigsuspend (&oldmask);
    sigprocmask (SIG_UNBLOCK, &mask, NULL);
    usr_interrupt = 0;
    memcpy(&msg_client,point+track[i].offset,sizeof(struct message));
    if(msg_client.type==getpid())
    {
        printf("\t\t Message\n");
        printf("\t PID: %d\tSl.no: %d\ta: %d\tb: %d\tshmid: %d\tsem_val: %d\n",msg_client.pid,msg_client.slno,msg_client.a,msg_client.b,shmid,0);           
        i++;            
        kill(server,SIGUSR1);
        sem_post(acc);
        printf("\t\t Given acces to server %d\n",server);
        msg_count--;
    }
    sleep(1);
  }
  return 0;
}