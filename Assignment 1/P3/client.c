/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
**************************
**************************
*   Bhavin Senjaliya     *     
*    2012A8PS274P        *   
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
#include "rshmAPI.c"
int main(int argc, char const *argv[])
{
	key_t msg_req_key=ftok("client.c",'^');
  	key_t msg_rep_key=ftok("client.c",'&');
	if((msg_req_id=msgget(msg_req_key,IPC_CREAT | 0666))==-1)
	{perror("msgget");exit(1);}
	if((msg_rep_id=msgget(msg_rep_key,IPC_CREAT | 0666))==-1)
	{perror("msgget");exit(1);}
	key_t key;int rshmid;int s;
	char input;
	char input_w[512];
	size_t size;
	char * pointer,*base;
	printf("Enter the key you want to use to create shared memory.\n");
	scanf("%d",&key);
	printf("Enter size for shared memory.\n");
	scanf("%d",&s);size=s;
	rshmid=rshmget(key,size);
	printf("get succeded %d is RSHMID\n",rshmid);
	pointer=rshmat(rshmid,(void *) 0);
	printf("attach succeded,%p is pointer\n",pointer);
	base=pointer;
	while(1)
	{
		printf("If you want to read press r , for write press w or e to exit.\n");
		scanf(" %c",&input);getchar();
		if(input=='r')
		{
			//read
			strcpy(input_w,base);
			printf("The following are the contents of shared memory %d\n",rshmid);
			printf("%s\n",input_w);
			// pointer+=sizeof(input_w)+2;
		}
		else if(input=='w')
		{
			//write
			printf("Enter you input\n");
			fflush (stdin);
			fgets (input_w, sizeof(input_w), stdin);
		    /* Remove trailing newline, if there. */
		    // if ((strlen(input_w)>0) && (input_w[strlen (input_w) - 1] == '\n'))
		    //     input_w[strlen (input_w) - 1] = '\0';
		    if((strchr(base,'\0'))!=NULL)
		    	strcpy(strchr(pointer,'\0'),input_w);
			else 
				strcpy(pointer,input_w);
			rshmChanged(rshmid);
			// pointer+=sizeof(input_w)+2;
		}
		else if(input=='e')
		{
			break;
		}
		else
		{
			printf("Try again\n");
			continue;
		}
	}
	rshmdt(rshmid,pointer);
	printf("Detach succeded\n");
	rshmctl(rshmid,IPC_RMID);
	printf("IPC_RMID succeded\n");
	return 0;
}
