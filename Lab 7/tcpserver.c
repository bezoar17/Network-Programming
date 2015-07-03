/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
*************************/
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#define SIZE 100
#define LIMIT 10
int i, maxi, maxfd, listenfd, connectfd, talk_fd;
int nready, clients[FD_SETSIZE];int port_no;
int check[FD_SETSIZE], ola=0;
ssize_t n;
fd_set rset, allset;
char buffer[SIZE];
socklen_t client_len;
struct sockaddr_in client, name;
void SIGALRM_handler()
{
  int q;
  if(ola == 0)
  { 
    printf("Sending HELLO to clients\n");
    for(q=0; q<=maxi; ++q)
    {
      if(clients[q]>0)
      {
        write(clients[q], "hello\n", 6);
        check[q] = 0;
      }
    }
    ola=1;
  }
  else
  { 
    printf("Eliminating clients:\n");
    for(q=0; q<=maxi; ++q)
    {
      if(clients[q]>0 && check[q] == 0)
      { 
        close (clients[q]);
        FD_CLR (clients[q], &allset);
        clients[q] = -1;
        check[q] = -1;
      }
    }
    rset = allset;
    ola = 0;
  }
  alarm(10);
  return;
}
int main (int argc, char const *argv[])
{
  listenfd = socket (AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0)
  {
    perror("socket");
    exit(-1);
  }
  printf("Use 'telnet ip port' on terminal for connecting.\n");
  printf("Enter the port no.\n");
  scanf(" %d",&port_no);
  bzero (&name, sizeof (name));
  name.sin_family = AF_INET;
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  name.sin_port = htons (port_no);
  bind (listenfd, (struct sockaddr *) & name, sizeof (name));
  listen (listenfd, LIMIT);
  printf("Listening for incoming connections on port %d\n",port_no);
  maxfd = listenfd;//starting values
  maxi = -1;
  for (i = 0; i < FD_SETSIZE; i++)
  {
    clients[i] = -1;
    check[i] = -1;
  }
  FD_ZERO (&allset);
  FD_SET (listenfd, &allset);
  ola = 0;
  struct sigaction act;
  memset(&act, '\0', sizeof(act));
  act.sa_sigaction = &SIGALRM_handler;
  act.sa_flags = SA_NODEFER | SA_SIGINFO;
  sigaction(SIGALRM, &act, NULL);
  alarm(20);
  for (;;)
    {
      rset = allset;//update
      nready = select (maxfd + 1, &rset, NULL, NULL, NULL);
      if(nready < 0) continue;
      if (FD_ISSET (listenfd, &rset))//new client
         {    
            client_len = sizeof (client);
            connectfd = accept (listenfd, (struct sockaddr *) & client, &client_len);
            printf("New client connected\n");//save new client.
            for (i = 0; i < FD_SETSIZE; i++)
            {  
                if (clients[i] < 0)
                  {clients[i] = connectfd;check[i] = 1;break;}
            }
            if (i == FD_SETSIZE)
            {printf ("too many clientss");exit(1);}
            FD_SET (connectfd, &allset);
            if (connectfd > maxfd)
              maxfd = connectfd; 
            if (i > maxi)
              maxi = i;  
            if (--nready <= 0)
              continue;   
         }
      for (i = 0; i <= maxi; i++)
          {    
            if ((talk_fd = clients[i]) < 0)
              continue;
            if (FD_ISSET (talk_fd, &rset))
              {
                  memset(buffer, '\0', sizeof(buffer));
                  if ((n = read (talk_fd, buffer, SIZE)) == 0)
                    {
                      //connection closed by client
                      close (talk_fd);
                      FD_CLR (talk_fd, &allset);
                      clients[i] = -1;
                      check[i] = -1;
                    }
                  else
                    {
                      check[i] = 1;
                      int q;
                      for (q = 0; q <= maxi; ++q)
                      {
                        if (clients[q] >= 0 && q!=i )
                          write (clients[q], buffer, n);
                      }
                     }
                  if (--nready <= 0)
                        break;//no more readable descriptors    
              }
           }
    }
}