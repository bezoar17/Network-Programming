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
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include "rshmAPI.c"
#define READ 0
#define WRITE 1
struct tracker
{
  char *point;  
};
char *ptr,*current_ptr;
pthread_t one,two;pthread_mutex_t mutexsum;
struct sockaddr_storage addr;
char ipstr[INET6_ADDRSTRLEN];
char input;char ip[32];char buffer[512];
int temp;struct message msg;
int val=0;int i;char *base,*point;
key_t msg_req_key;
  key_t msg_rep_key;
  int msg_req_id,msg_rep_id;
 char *common;
struct message_response msg_rep;
  struct rshminfo all_shm[10];
  int common_size=sizeof(all_shm);
  struct tracker track[10];
int listenfd,connectfd,talkingfd,cc=0,cm=0,fl=0,no_shm=0;
  struct   sockaddr_in server,remote_server;
void SIGINT_handler()
{
    if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
      msgctl(msg_rep_id,IPC_RMID,NULL);
      msgctl(msg_rep_id,IPC_RMID,NULL);
      pthread_mutex_destroy(&mutexsum);
      pthread_exit(NULL);
      exit(0);
}
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 
int recvall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}
void *controlone()
{
  while(1)
      {   
          
          cc=recv(connectfd,&msg_rep,sizeof(struct message_response),0);
          if(cc >0)
          {
             pthread_mutex_lock (&mutexsum);
            printf("Received message from %s as '%c'\n",ipstr,msg_rep.op);
            switch(msg_rep.op)
            {
              case 'g' :  
                          for(i=0;i<10;i++)
                            {
                              if(all_shm[i].key==msg_rep.key)
                              {
                                fl=1;break;
                              }
                            }
                            if(fl==0)
                            {
                              //create shm
                              printf("Creating shared memory RSHMID %d of size %d with key %d\n",msg_rep.rshmid,(int)msg_rep.size,msg_rep.key);
                              if ((all_shm[no_shm].shmid = shmget (msg_rep.key, msg_rep.size, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
                                       {
                                       perror ("shmget: shmget failed");
                                       exit (1);
                                       }//getting shmid
                              all_shm[no_shm].key=msg_rep.key;
                              all_shm[no_shm].addr=shmat(all_shm[no_shm].shmid,(void *)0,0);
                              if (all_shm[no_shm].addr == (void *) (-1))
                               perror ("shmat");
                              // all_shm[no_shm].remote_addrs=&server;
                              track[no_shm].point=(char *)all_shm[no_shm].addr;
                              all_shm[no_shm].rshmid=msg_rep.rshmid;
                              no_shm++;

                            }
                          fl=0;
                          break;
              case 'a' :  
                         for(i=0;i<10;i++)
                          {
                            if(all_shm[i].rshmid==msg_rep.rshmid)
                            {
                               printf("Increased ref count by one for RSHMID %d\n",msg_rep.rshmid);
                               all_shm[i].ref_count++;
                            }
                          } 
                          // all_shm[no_shm].remote_addrs=&server;
                          break;
              case 'd' :  
                        for(i=0;i<10;i++)
                        {
                          if(all_shm[i].rshmid==msg_rep.rshmid)
                          {
                            all_shm[i].ref_count--;
                            printf("Decreased ref count by one for RSHMID %d\n",msg_rep.rshmid);
                          }
                        } 

                        break;
              case 'c' :  
                        for(i=0;i<10;i++)
                        {
                          if(all_shm[i].rshmid==msg_rep.rshmid)
                          {
                            all_shm[i].ref_count=0;
                            shmdt(all_shm[i].addr);
                            printf("Removed shared memory RSHMID %d\n",msg_rep.rshmid);
                            shmctl(all_shm[i].shmid,IPC_RMID,NULL);
                          }
                        }                    
                        break;
              case 'C' :  
                        for(i=0;i<10;i++)
                        {
                          if(all_shm[i].rshmid==msg_rep.rshmid)
                          {
                            break;
                          }
                        }
                        recvall(connectfd,buffer,&msg_rep.r_flag);
                        ptr=shmat(all_shm[i].shmid,(void *)0,0);
                        current_ptr=ptr;
                        if(strchr(ptr,'\0')!=NULL)
                        	strcpy(strchr(ptr,'\0'),buffer);
                        else
                        	strcpy(ptr,buffer);
                        break;
              default  :
                          break;
            }
             pthread_mutex_unlock (&mutexsum);
          }
      }
   pthread_exit((void*) 0);
}
void *controltwo()
{
    while(1)
      {
        
        cm=msgrcv(msg_req_id,&(msg.mtype),sizeof(struct message),1,0);
        if(cm>0)
        {
           pthread_mutex_lock (&mutexsum);
          printf("Received message from %d as '%d'\n",msg.calling_pid,msg.func);
          //local message received
          switch(msg.func)//check this shit
          {
            case 1: 
                      for(i=0;i<10;i++)
                      {
                        
                        if(all_shm[i].key==msg.key)
                        {
                          fl=1;msg_rep.rshmid=all_shm[i].rshmid;
                         
                          break;
                        }
                      }
                      if(fl==0)
                      {
                        //create shm
                        printf("Creating shared memory with key %d of size%d\n",msg.key,(int)msg.size);
                        if ((all_shm[no_shm].shmid = shmget (msg.key, msg.size, 0666 | IPC_CREAT | IPC_EXCL)) == -1)
                                 {
                                 perror ("shmget: shmget failed");
                                 exit (1);
                                 }//getting shmid
                              all_shm[no_shm].key=msg.key;
                              all_shm[no_shm].size=msg.size;
                              all_shm[no_shm].addr=shmat(all_shm[no_shm].shmid,(void *)0,0);
                              if (all_shm[no_shm].addr == (void *) (-1))
                               perror ("shmat");
                              // all_shm[no_shm].remote_addrs=&server;
                              track[no_shm].point=all_shm[no_shm].addr;
                              while(val==0)
                              {
                                val=rand();
                              }
                              all_shm[no_shm].rshmid=val;
                              msg_rep.rshmid=val;
                              no_shm++;
                      }
                      msg_rep.mtype=msg.calling_pid;
                      msg_rep.op='g';
                      msg_rep.key=msg.key;
                      msg_rep.size=msg.size;
                      send(connectfd,&msg_rep,sizeof(struct message_response),0); 
                      if((msgsnd(msg_rep_id,&(msg_rep.mtype),sizeof(struct message),0))==-1)
                      {perror("msgsnd");}
                      fl=0;
                      break;
            case 2: 
                      for(i=0;i<10;i++)

                        {
                          if(all_shm[i].rshmid==msg.rshmid)
                          {
                            all_shm[i].ref_count++;
                            // all_shm[no_shm].remote_addrs=&server;
                            msg_rep.mtype=msg.calling_pid;
                            msg_rep.op='a';
                            msg_rep.rshmid=all_shm[i].shmid;
                            printf("Attached %d to shared memory RSHMID  %d\n",msg.calling_pid,msg.rshmid);
                            send(connectfd,&msg_rep,sizeof(struct message_response),0);
                            if((msgsnd(msg_rep_id,&(msg_rep.mtype),sizeof(struct message),0))==-1)
                            {perror("msgsnd");}
                          }
                        }
                        
                       break;
            case 3: 
                      for(i=0;i<10;i++)
                      {
                        if(all_shm[i].rshmid==msg.rshmid)
                        {
                          break;
                        }
                      }
                      all_shm[i].ref_count--;
                      msg_rep.mtype=msg.calling_pid;
                      msg_rep.op='d';
                      msg_rep.d_flag=100;
                      printf("Dettached %d from shared memory RSHMID  %d\n",msg.calling_pid,msg.rshmid);
                      send(connectfd,&msg_rep,sizeof(struct message_response),0);
                      if((msgsnd(msg_rep_id,&(msg_rep.mtype),sizeof(struct message),0))==-1)
                      {perror("msgsnd");}
                      break;
            case 4: 
                    for(i=0;i<10;i++)
                    {
                      if(all_shm[i].rshmid==msg.rshmid)
                      {
                        all_shm[i].ref_count=0;
                        shmdt(all_shm[i].addr);
                        if((shmctl(all_shm[i].shmid,IPC_RMID,NULL))==0)
                            msg_rep.rm_flag=100;
                      }
                    }
                    msg_rep.mtype=msg.calling_pid;
                    msg_rep.op='c';
                    printf("Removed shared memory RSHMID  %d\n",msg.rshmid);
                    send(connectfd,&msg_rep,sizeof(struct message_response),0);
                    if((msgsnd(msg_rep_id,&(msg_rep.mtype),sizeof(struct message),0))==-1)
                      {perror("msgsnd");}
                      break;
            case 5: 
                    for(i=0;i<10;i++)
                    {
                      if(all_shm[i].rshmid==msg.rshmid)
                      {
                        break;
                      }
                    }
                    ptr=shmat(all_shm[i].shmid,(void *)0,0);
                    current_ptr=ptr;
                    if(strchr(ptr,'\0')==NULL)
                    	strcpy(buffer,ptr);
                    else
                    {
                    	ptr=strchr(ptr,'\0');
                    	for(ptr=ptr-2;;ptr--)
                    	{
                    		if(*ptr=='\n')
                    			{strcpy(buffer,ptr+1);break;}
                    		else if(current_ptr==ptr)
                    			{strcpy(buffer,ptr);break;}
                    	}
                    }
                    msg_rep.rshmid=msg.rshmid;
                    msg_rep.r_flag=strlen(buffer)+1;
                    msg_rep.op='C';
                    send(connectfd,&msg_rep,sizeof(struct message_response),0);
                    if(sendall(connectfd, buffer, &msg_rep.r_flag)==0)
                    msg_rep.mtype=msg.calling_pid;                  
                    if((msgsnd(msg_rep_id,&(msg_rep.mtype),sizeof(struct message),0))==-1)
                      {perror("msgsnd");}
                        break;
            default :
                      printf("ERRRRRRRRRRRR\n");
                      break;
          }
           pthread_mutex_unlock (&mutexsum);
        }
        
      }
     pthread_exit((void*) 0);
}

int main(int argc, char const *argv[])
{
  if (signal(SIGINT, SIGINT_handler) == SIG_ERR) {
          printf("\n\t#######  SIGINT install error\n");
          exit(1);
     }
  printf("Run rshmServer on two systems connected to a network and connect and listen on either of one.\n");
  printf("Then run client (./client) on both sytems to create shared memory, be sure to mention the same key for shared memory accross systems.\n");
  printf("In case of a msgget failed change the keys(msg_req_key,msg_rep_key) for client and rshmServer on that system and run again.\n");
  socklen_t len;
  key_t msg_req_key=ftok("client.c",'^');
  key_t msg_rep_key=ftok("client.c",'&');
  len = sizeof addr;
  time_t t; 
  srand((unsigned) time(&t));
  if((msg_req_id=msgget(msg_req_key,IPC_CREAT | IPC_EXCL | 0666))==-1)
  { perror("msgget");exit(1);}
  if((msg_rep_id=msgget(msg_rep_key,IPC_CREAT | IPC_EXCL | 0666))==-1)
  { perror("msgget");exit(1);}
  bzero(&server, sizeof(struct   sockaddr_in));
  bzero(&remote_server, sizeof(struct   sockaddr_in));
  int    rlen;
    rlen=sizeof(remote_server);
  server.sin_family = AF_INET;
  server.sin_port = htons(1111);//fixed port_no
  listenfd = socket (AF_INET,SOCK_STREAM,0);
    if(listenfd < 0)
        {perror("socket");exit(1);}
  printf("Connect(Press c) or Listen(Press l).\n");
  scanf(" %c",&input);
  if(input=='c')
  {
    printf("Enter the IP to connect to. Port is default 1111\n");
    scanf("%s",ip);
    inet_pton(AF_INET,ip,&server.sin_addr);
    connect(listenfd,(struct sockaddr*) &server, sizeof(server));
    connectfd=listenfd;
    getpeername(connectfd, (struct sockaddr*)&addr, &len);
      // deal with both IPv4 and IPv6:
      if (addr.ss_family == AF_INET) {
          struct sockaddr_in *s = (struct sockaddr_in *)&addr;
          // port = ntohs(s->sin_port);
          inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
      } else { // AF_INET6
          struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
          // port = ntohs(s->sin6_port);
          inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
      }
    printf("Connected to %s\n",ipstr);
  }
  else if(input=='l')
  {
      server.sin_addr.s_addr = htonl(INADDR_ANY);//ip of host
      if(bind(listenfd, (struct sockaddr*) &server, sizeof(struct sockaddr)) < 0)
        {perror("bind");exit(1);}
      if(listen(listenfd, 5) < 0)
        {perror("listen");exit(1);}
      connectfd=accept(listenfd,(struct sockaddr *)&remote_server, &rlen);
      getpeername(connectfd, (struct sockaddr*)&addr, &len);
      // deal with both IPv4 and IPv6:
      if (addr.ss_family == AF_INET) {
          struct sockaddr_in *s = (struct sockaddr_in *)&addr;
          // port = ntohs(s->sin_port);
          inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
      } else { // AF_INET6
          struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
          // port = ntohs(s->sin6_port);
          inet_ntop(AF_INET6, &s->sin6_addr, ipstr, sizeof ipstr);
      }
     //    ipaddress=inet_ntop(AF_INET,&remote_server.sin_addr, ipaddress, rlen);
     // ipaddress=inet_ntoa(remote_server.sin_addr);
     // printf("client address :%s\n",ipaddress);
      // printf("Peer IP address: %s\n", ipstr);
      printf("Connected to %s\n",ipstr);
      close(listenfd);//not sure
      //printf("remote_server address :%s\n",ipaddress);
    }
    pthread_mutex_init(&mutexsum, NULL);
    pthread_create(&one, NULL, controlone, NULL);
    pthread_create(&two, NULL, controltwo, NULL);
    while(1);
  return 0;
}
