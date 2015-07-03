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
struct rshminfo
	{
		int rshmid;
		/*unique id across all systems. created by the
		first system*/
		key_t key; /*key used to create shm segment*/
		int shmid; /*shmid returned by the local system*/
		size_t size;
		void *addr; /*address returned by the local system*/
		int ref_count; /*no of processes attached to*/
		struct sockaddr_in *remote_addrs; /* list of remote end
		points*/
	};
struct message_response
{
	long mtype;
	int rshmid;
  	key_t key;
  	size_t size;
	void * ret_pointer;
	int d_flag;
	int rm_flag;
	int r_flag;
	char op;
};
struct message
{
	long mtype;
	key_t key;
	size_t size;
	int rshmid;	
	pid_t calling_pid;
	int func;
};
int msg_req_id;
int msg_rep_id;
int rshmget(key_t key, size_t size);
void *rshmat(int rshmid, void* addr);
int rshmdt(int rshmid, void* addr);
int rshmctl(int rshmid, int cmd);
void rshmChanged(int rshmid);
int rshmget(key_t key, size_t size)
{
	int rshmid;
	struct message msg;struct message_response msg_rep;
	msg.key=key;
	msg.size=size;
	msg.func=1;
	msg.mtype=1;
	msg.calling_pid=getpid();
	//send message to local TCP server.
	if((msgsnd(msg_req_id,&(msg.mtype),sizeof(struct message),0))==-1)
		perror("msgsnd");
	//wait for receiving the message
	if((msgrcv(msg_rep_id,&(msg_rep.mtype),sizeof(struct message_response),getpid(),0))==-1)
		perror("msgsnd");
	//read rshmid from the message
	return msg_rep.rshmid;
}
void *rshmat(int rshmid, void* addr)
{
	void* point;
	struct message msg;struct message_response msg_rep;
	msg.rshmid=rshmid;
	// msg.size=size;
	msg.func=2;
	msg.mtype=1;
	msg.calling_pid=getpid();
	//send message to local TCP server.
	if((msgsnd(msg_req_id,&(msg.mtype),sizeof(struct message),0))==-1)
		perror("msgsnd");
	//wait for receiving the message
	if((msgrcv(msg_rep_id,&(msg_rep.mtype),sizeof(struct message_response),getpid(),0))==-1)
		perror("msgsnd");
	point=shmat(msg_rep.rshmid,(void *) 0,0);
	if (point == (char *) (-1))
    perror ("shmat");		
	//send back pointer
	return point;
}
int rshmdt(int rshmid,void* addr)
{
	struct message msg;struct message_response msg_rep;
	msg.rshmid=rshmid;
	// msg.size=size;
	msg.func=3;
	msg.mtype=1;
	msg.calling_pid=getpid();
	//send message to local TCP server.
	if((msgsnd(msg_req_id,&(msg.mtype),sizeof(struct message),0))==-1)
		{perror("msgsnd");return -1;}
	//wait for receiving the message
	if((msgrcv(msg_rep_id,&(msg_rep.mtype),sizeof(struct message_response),getpid(),0))==-1)
		{perror("msgrcv");return -1;}
	//send back pointer
	if(msg_rep.d_flag==100)
	{shmdt(addr);return 0;}
	else
	{
		printf("Dettach failed\n");return -1;
	}

}
int rshmctl(int rshmid,int cmd)
{
	if(cmd!=IPC_RMID)
		{printf("Only IPC_RMID is supported");return -1;}
	struct message msg;struct message_response msg_rep;
	msg.rshmid=rshmid;
	msg.func=4;
	msg.mtype=1;
	msg.calling_pid=getpid();
	//send message to local TCP server.
	if((msgsnd(msg_req_id,&(msg.mtype),sizeof(struct message),0))==-1)
		{perror("msgsnd");return -1;}
	//wait for receiving the message
	if((msgrcv(msg_rep_id,&(msg_rep.mtype),sizeof(struct message_response),getpid(),0))==-1)
		{perror("msgrcv");return -1;}
	//send back pointer
	if(msg_rep.rm_flag==100)
	return 0;
	else
	{
		printf("Remove failed\n");return -1;
	}

}
void rshmChanged(int rshmid)
{
	struct message msg;struct message_response msg_rep;
	msg.rshmid=rshmid;
	msg.mtype=1;
	msg.func=5;
	msg.calling_pid=getpid();
	//send message to local TCP server.
	if((msgsnd(msg_req_id,&(msg.mtype),sizeof(struct message),0))==-1)
		{perror("msgsnd");}
	//wait for receiving the message
	if((msgrcv(msg_rep_id,&(msg_rep.mtype),sizeof(struct message_response),getpid(),0))==-1)
		{perror("msgrcv");}
	if(msg_rep.r_flag<=0)
		printf("Server couldnt read\n");
} 
