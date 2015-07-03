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
#include <netdb.h>
#include <limits.h> 
#include <errno.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#define SIZE 100
#define LIMIT 10
int i, maxi, maxfd, listenfd, connectfd, talk_fd;
struct pollfd   clients[FD_SETSIZE];
int nready,port;
int control[FD_SETSIZE], foo=0;
ssize_t n;
fd_set rset, allset;
char buffer[SIZE];
socklen_t client_len;
struct sockaddr_in client, name;
void SIGALRM_handler()
{
  int q;
  if(foo == 0)
  { 
    printf("Sending HELLO to clients\n");
    for(q=0; q<=maxi; ++q)
    {
      if(clients[q].fd>0)
      {
        write(clients[q].fd, "hello\n", 6);
        control[q] = 0;
      }
    }
    foo=1;
  }
  else
  { 
    printf("Eliminating clients:\n");
    for(q=0; q<=maxi; ++q)
    {
      if(clients[q].fd>0 && control[q] == 0)
      { 
        close (clients[q].fd);
        // FD_CLR (clients[q], &allset);
        clients[q] = -1;
        control[q] = -1;
      }
    }
    // rset = allset;
    foo = 0;
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
  scanf(" %d",&port);
  bzero (&name, sizeof (name));
  name.sin_family = AF_INET;
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  name.sin_port = htons (port);
  bind (listenfd, (struct sockaddr *) & name, sizeof (name));
  listen (listenfd, LIMIT);
  printf("Listening for incoming connections on port %d\n",port);
  maxfd = listenfd;//starting values
  for (i = 1; i < FD_SETSIZE; i++)
  {
    clients[i].fd = -1;
    control[i] = -1;
  }
  clients[0].fd=listenfd;
  clients[0].events=POLLRDNORM;
  maxi = 0;
  foo = 0;
  struct sigaction act;
  memset(&act, '\0', sizeof(act));
  act.sa_sigaction = &SIGALRM_handler;
  act.sa_flags = SA_NODEFER | SA_SIGINFO;
  sigaction(SIGALRM, &act, NULL);
  alarm(20);
  for (;;)
    {
      nready = poll(client,maxi+1,100000);
      // if(nready < 0) continue;
      if (clients[0].revents & POLLRDNORM)//new client
         {    
            client_len = sizeof (client);
            connectfd = accept (listenfd, (struct sockaddr *) & client, &client_len);
            printf("New client connected\n");//save new client.
            for (i = 1; i < FD_SETSIZE; i++)
            {  
                if (clients[i].fd < 0)
                  {clients[i].fd = connectfd;control[i] = 1;break;}
            }
            if (i == FD_SETSIZE)
            {printf ("too many clientss");exit(1);}
            // FD_SET (connectfd, &allset);
            if (connectfd > maxfd)
              maxfd = connectfd; 
            clients[i].events=POLLRDNORM;
            if (i >= maxi)
              maxi = i;  
            if (--nready <= 0)
              continue;   
         }
      for (i = 1; i <= maxi; i++)
          {    
            if ((talk_fd = clients[i].fd) < 0)
              continue;
            if (clients[i].revents & (POLLRDNORM | POLLERR))
              {
                  memset(buffer, '\0', sizeof(buffer));
                  if ((n = read (talk_fd, buffer, SIZE)) == 0)
                    {
                      //connection closed by client
                      // close (talk_fd);
                      close(clients[i].fd);
                      clients[i].fd = -1;
                      control[i] = -1;
                    }
                  else if (n<0) 
                  {
                    if(errno == ECONNRESET)
                    {
                      printf("Connection closed by a client.\n");
                    }
                    else 
                      printf("Reading error\n");
                  }
                  else
                    {
                      control[i] = 1;
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