// ab -n 1000 -c 10 -g temp1.txt http://localhost:4000/index.html
#include <sys/epoll.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
extern int errno;
#define LISTENQ 5
#define MAX_BUF 200		/* Maximum bytes fetched by a single read() */
#define MAX_EVENTS 10		/* Maximum number of events to be returned from
				   a single epoll_wait() call */
#define MAX 1024
#define READING_REQUEST 1
#define HEADER_PARSING 2
#define READING_DISKFILE 3
#define WRITING_HEADER 4
#define WRITING_BODY 5
#define DONE 6
pthread_t main_thread,process_thread; int msqid;int listenfd, clilen;
struct sockaddr_in cliaddr, servaddr;
void errExit (char *s)
{
  perror (s);
  exit (-1);
}
struct msg_custom
{
  long mtype;
  int request_id;
};
struct request_data
{
  int client_fd;
  char header[512];
  char request[2048];
  char filebuffer[8192];
};
struct request_data all_requests[MAX];
void new_request(int fd)
{
  struct msg_custom msg;
  int q;char buffer[2048];int flag=0;
  for (q= 0; q< MAX; ++q)
  {
    if(all_requests[q].client_fd==-1)
      {flag=1;break;}
  }
  if(flag==0)
  {printf("Requests full");exit(-1);}
  flag=0;
  int s = read (fd, buffer, sizeof(buffer));
  buffer[s] = '\0';
  if (s == -1)
  errExit ("read");
  if (s == 0)
    {//eof received, close connection
      close (fd);
    }
    if (s > 0)
    {
     //data available on fd write it to message queue
     printf("Reading request.\n");
     if( strncmp(buffer,"GET ",4) && strncmp(buffer,"get ",4) ) 
      {
      printf("ONLY GET requests entertained. TRY AGAIN. Exiting\n");
      exit(-1);
      }
     msg.mtype=READING_REQUEST;
     msg.request_id=q;printf("Request id assigned is %d\n",msg.request_id);
     all_requests[msg.request_id].client_fd=fd;
     printf("fd saa is %d\n",fd);
     memcpy(&all_requests[msg.request_id].request, buffer, sizeof(buffer));
     bzero(&all_requests[msg.request_id].header, sizeof(all_requests[msg.request_id].header));
     bzero(&all_requests[msg.request_id].filebuffer, sizeof(all_requests[msg.request_id].filebuffer));
     if (msgsnd (msqid, &(msg.mtype), sizeof(struct msg_custom), 0) == -1)
     errExit("msgsnd");
     }
}
void* controlmain()
{
  int epfd,ready,fd,s,j;
  struct epoll_event ev;
  struct epoll_event evlist[MAX_EVENTS];
  epfd = epoll_create (20);
  if (epfd == -1)
   errExit ("epoll_create");
   ev.events = EPOLLIN;    /* Only interested in input events */
   ev.data.fd = listenfd;
  if (epoll_ctl (epfd, EPOLL_CTL_ADD, listenfd, &ev) == -1)
    errExit ("epoll_ctl");
  for (;;)
    {
      ready=epoll_wait(epfd, evlist, MAX_EVENTS, -1);
      if (ready == -1)
        {
          if (errno == EINTR)
            continue;   /* Restart if interrupted by signal */
          else
            errExit ("epoll_wait");
        }     
      for (j = 0; j < ready; j++)
          {
            if (evlist[j].events & EPOLLIN)
              {
                if (evlist[j].data.fd == listenfd)
                      {
                      // accepting new connection
                        clilen = sizeof (cliaddr);
                        char ip[128];
                        memset (ip, '\0', 128);
                        int connfd =accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
                        ev.events = EPOLLIN;  /* Only interested in input events */
                        ev.data.fd = connfd;
                        printf("new connection with %d\n",connfd);
                        if (epoll_ctl (epfd, EPOLL_CTL_ADD, connfd, &ev) == -1)
                          errExit ("epoll_ctl");
                      }
                else
                      {// input data on an open connection
                        new_request(evlist[j].data.fd);}
              }
          }
    }
    pthread_exit((void*) 0);
}
void* controlprocess()
{
  struct msg_custom msg,msg_send;
  while (msgrcv (msqid, &(msg.mtype), sizeof(struct msg_custom), 0, 0) != -1)
    {
         char* filename;
         int fdrequest;long len;
         char add[28];
         printf("Request id is %d\n",msg.request_id);
         switch(msg.mtype)
          {
            case READING_REQUEST:
              //parse header now
              printf("%s\n",all_requests[msg.request_id].request);
              printf("Parsing header\n");
              msg_send.mtype = HEADER_PARSING;
              msg_send.request_id = msg.request_id;
              sprintf(all_requests[msg.request_id].header,"HTTP/1.1 200 OK\nServer: Hallelujah\n");
              if (msgsnd (msqid, &(msg_send.mtype), sizeof(struct msg_custom), 0) == -1)
                      errExit("msgsnd");
              break;
            case HEADER_PARSING:
              //read diskfile now
              printf("Reading diskfile\n");
              filename = strtok(all_requests[msg.request_id].request, "/");
              filename = strtok(NULL, " ?");
              fdrequest = open(filename, O_RDONLY);
              if(fdrequest < 0)
              { 
                close(all_requests[msg.request_id].client_fd);
                errExit("open");
              }
              len = (long)lseek(fdrequest, (off_t)0, SEEK_END);//calculating file length
              (void)lseek(fdrequest, (off_t)0, SEEK_SET); 
              sprintf(add,"Content-Length: %ld\n\n",len);
              strcat(all_requests[msg.request_id].header,add);
              if((read(fdrequest, all_requests[msg.request_id].filebuffer, 8192))<0)
                {errExit("read");}
              msg_send.mtype = READING_DISKFILE;
              msg_send.request_id = msg.request_id;
                close(fdrequest);
              if (msgsnd (msqid, &(msg_send.mtype), sizeof(struct msg_custom), 0) == -1)
                      errExit("msgsnd");
              break;
            case READING_DISKFILE:
              //write header now
                printf("writing header as %s\n",all_requests[msg.request_id].header);
                msg_send.mtype = WRITING_HEADER;
                msg_send.request_id = msg.request_id;
                printf("fd is %d\n",all_requests[msg.request_id].client_fd);
                if((write(all_requests[msg.request_id].client_fd, all_requests[msg.request_id].header, sizeof(all_requests[msg.request_id].header)))<0)
                    errExit("write");
                if (msgsnd (msqid, &(msg_send.mtype), sizeof(struct msg_custom), 0) == -1)
                      errExit("msgsnd");
              break;
            case WRITING_HEADER:
              //write body now
               printf("writing body\n");
               msg_send.mtype = WRITING_BODY;
               msg_send.request_id = msg.request_id;
                // write(msq.clifd, bod1, sizeof(bod1));
                if((write(all_requests[msg.request_id].client_fd, all_requests[msg.request_id].filebuffer, sizeof(all_requests[msg.request_id].filebuffer)))<0)
                  errExit("write");
                // write(msq.clifd, bod2, sizeof(bod2));
                if (msgsnd (msqid, &(msg_send.mtype), sizeof(struct msg_custom), 0) == -1)
                      errExit("msgsnd");
              break;
            case WRITING_BODY:
                printf("Processed request %s\n", all_requests[msg.request_id].request);
                msg_send.mtype=DONE;
                if (msgsnd (msqid, &(msg_send.mtype), sizeof(struct msg_custom), 0) == -1)
                      errExit("msgsnd");
                break;
            case DONE :
                bzero(&all_requests[msg.request_id].header, sizeof(all_requests[msg.request_id].header));
                bzero(&all_requests[msg.request_id].filebuffer, sizeof(all_requests[msg.request_id].filebuffer));
                bzero(&all_requests[msg.request_id].request, sizeof(all_requests[msg.request_id].request));
                all_requests[msg.request_id].client_fd=-1;
                printf("done\n");
                break;
          }
    }
    pthread_exit((void*) 0);
}
int main (int argc, char *argv[])
{
  if (argc < 2 || strcmp (argv[1], "--help") == 0)
    {printf ("Usage: %s <port>\n", argv[0]);exit(0);}
  int i;
  for (i = 0; i < MAX; ++i)
  {
    all_requests[i].client_fd=-1;
    bzero(&all_requests[i].header, sizeof(all_requests[i].header));
    bzero(&all_requests[i].request, sizeof(all_requests[i].request));
    bzero(&all_requests[i].filebuffer, sizeof(all_requests[i].filebuffer));
  }
  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  bzero (&servaddr, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (atoi (argv[1]));
  int acc = 1;
  if (setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&acc,sizeof acc) <0 )
    errExit("setsockopt()\n");
  if (bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0)
    errExit("bind");
  listen (listenfd, 15);
  if ((msqid = msgget (IPC_PRIVATE,IPC_CREAT | IPC_EXCL | 0666)) == -1)
    {perror ("msgget");exit (1);}
  pthread_create(&main_thread, NULL, controlmain, NULL);
  pthread_create(&process_thread, NULL, controlprocess, NULL);
  while(1);
    return 0;
}
