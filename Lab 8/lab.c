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
#include <sys/select.h>
#include <fcntl.h>
#include <poll.h>
#include <limits.h>	
int					i, maxi, listenfd, connfd, sockfd;
int					nready,maxfd;
int 				port_no;
char				buf[100];
socklen_t			clilen;
struct pollfd		client[10];
int 				check[10];
int 				ola;
fd_set 				rset,allset;

void send_all(char *val)
{
	//printf("In send_all\n");
	for(i=1;i<=maxi;i++)
	{
		if(!(client[i].fd<0))
			write(client[i].fd,val,strlen(val));
	}
	//printf("Sent\n");
}

void SIGALRM_handler(int s)
{	
	alarm(10);
	//printf("In SIGALRM_handler\n");	
	if(ola==0)
		{
			ola=1;
			for(i=0;i<=maxi;i++)
			{
				if(check[i]==0)
					{
						//connection closed;
						close(client[i].fd);
						client[i].fd=-1;
						check[i]=-1;
					}
			}
		}
	else
		{
			ola=0;
			for(i=0;i<=maxi;i++)
			{
				if(!(check[i]<0))
					check[i]=0;
			}
			send_all("hello\n");
		}
	if (signal(SIGALRM, SIGALRM_handler) == SIG_ERR) 
	{
          printf("SIGINT install error\n");
          exit(1);
    }
	//alarm(10);
    
}
int main(int argc, char **argv)
{
	
	struct sockaddr_in	cliaddr, servaddr;
	printf("Enter the port no. \n");
	scanf("%d",&port_no);
	if (signal(SIGALRM, SIGALRM_handler) == SIG_ERR) {
          printf("SIGINT install error\n");
          exit(1);
     }

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port_no);
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	listen(listenfd, 5);
	maxfd=listenfd;

	for (i = 1; i < 10; i++)
	{
		client[i].fd=-1;
		check[i]=-1;
	}	
	
	client[0].fd = listenfd;
	client[0].events = POLLRDNORM;
	maxi = 0;					/* max index into client[] array */

	for ( ; ; ) 
	{
		nready = poll(client, maxi+1, 100000);

		if (client[0].revents & POLLRDNORM)
		 {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
 			printf("Client Connected.\n");
 			
			for (i = 1; i < 10; i++)
				if (client[i].fd < 0) 
				{
					client[i].fd = connfd;	/* struct sockaddrve descriptor */
					check[i]=0;break;
				}
			if (i == 10)
				printf("Error : Too many clients\n");
			if(connfd>maxfd)
    			maxfd=connfd;
			client[i].events = POLLRDNORM;
			if (i >= maxi)
			{
				if(maxi==1)
					alarm(10);
				maxi = i;
				ola  = 1;
				// if(maxi==0)
				// 	alarm(10);
			}			
			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}
		
		for (i = 1; i <= maxi; i++) 
		{	/* check all clients for data */
			if ( (sockfd = client[i].fd) < 0)
				continue;

			if (client[i].revents & (POLLRDNORM | POLLERR)) 
			{
				int n;
				if ( (n = read(sockfd, buf, 100)) < 0) 
				{
					if (errno == ECONNRESET) 
					{
							/*connection reset by client */
					printf("client[%d] aborted connection\n", i);	
					} 
					else
						printf("read error");

				} 
				else if (n == 0) 
				{
											/*connection closed by client */
					close(client[i].fd);
					client[i].fd=-1;
					check[i]=-1;
				} 
				else
				{
					buf[n] = '\0';
					check[i]=1;
					//printf("3***********%d\n", n);
					send_all("hello\n");
				}
				//printf("------> n = %d\n", n);
				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
		//}
	}
}