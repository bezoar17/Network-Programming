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
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#define MAX_LEN 100
int sock, rsock, n;
struct in_addr multicast_addr;
struct hostent *h;
socklen_t cliLen;
socklen_t servLen;
struct ip_mreq mreq;
struct sockaddr_in client_addr, server_addr;
char buffer[MAX_LEN];char msg[MAX_LEN];char buf[MAX_LEN];
char* str;
int pid,ola,flag,pfd1[2],pfd2[2];
time_t time_val;
struct tm* ptm;
void SIGINT_handler() 
{
	time(&time_val);
	ptm = localtime(&time_val);
	strftime(msg, 1024, "bye-%d/%m/%Y %H:%M:%S",ptm);
	if(sendto(sock,msg,MAX_LEN,0,(struct sockaddr *) &server_addr, sizeof(server_addr))<0)
	{perror("Sending datagram message error");}
	exit(0);
}
int main(int argc, char *argv[])
{
		if(argc!=3)
		{printf("Usage: <mcast address> <port>\n");exit(0);}
		pipe(pfd1);pipe(pfd2);
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 50000;
		if(setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof tv) < 0)
			{perror("setsockopt");}
		h=gethostbyname(argv[1]);
		if(h == NULL) 
			{printf("unknown group '%s'\n", argv[1]);exit(1);}
		memcpy(&multicast_addr, h->h_addr_list[0],h->h_length);
		struct sigaction act;
		memset(&act, '\0', sizeof(act));
		act.sa_sigaction = &SIGINT_handler;
		act.sa_flags = SA_NODEFER | SA_SIGINFO;
		sigaction(SIGINT, &act, NULL);
		if(!IN_MULTICAST(ntohl(multicast_addr.s_addr)))
			{printf("given address '%s' is not a multicast address\n",inet_ntoa(multicast_addr));exit(1);}
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family      = AF_INET;
		server_addr.sin_addr.s_addr = multicast_addr.s_addr;
		server_addr.sin_port        = htons(atoi(argv[2]));
		if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{perror("Socket creation failed");exit(1);}
		/* Enable SO_REUSEADDR to allow multiple instances of this */
				/* application to receive copies of the multicast datagrams. */
				{
				int reuse = 1;
					if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0)
					{
					perror("Setting SO_REUSEADDR error");
					close(sock);
					exit(1);
					}
				else
				printf("Setting SO_REUSEADDR...OK.\n");
				}
				unsigned char loop;
				loop=1;
				// Got to have this to get replies from clients on same machine
				if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0){
				    perror("setsockopt loop");
				    exit(EXIT_FAILURE);
				}
		if(bind(sock,(struct sockaddr *) &server_addr, sizeof(server_addr))<0)
		{printf("Bind Error on port%d\n",atoi(argv[2]));exit(1);}
		mreq.imr_multiaddr.s_addr=multicast_addr.s_addr;
		mreq.imr_interface.s_addr=htonl(INADDR_ANY);
		if((rsock = setsockopt(sock,IPPROTO_IP,IP_ADD_MEMBERSHIP, (void *) &mreq, sizeof(mreq)))<0)
		{printf("Unable to join multicast group '%s'\n",inet_ntoa(multicast_addr));exit(1);}
		else 
		{	
			strcpy(buffer,"FALSE");
			write(pfd1[1], &buffer,sizeof buffer);
			printf("Listening in multicast group %s:%d\n",inet_ntoa(multicast_addr), atoi(argv[2]));
			if((pid = fork())== 0)
			{
				read(pfd2[0],&buf,sizeof buf);
				while(1)
				{	
					cliLen=sizeof(client_addr);
					if((n=recvfrom(sock,msg,MAX_LEN,0,(struct sockaddr *) &client_addr,&cliLen))<0)
					{
						if((flag ==1) && ((time(NULL) -time_val)>5))
						{
							flag = 0;
							strcpy(buffer,"FALSE");
							write(pfd1[1],&buffer,sizeof buffer);
							continue;
						}
					}
					else
					{
						printf("From %s:%d on %s %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), argv[1], msg);
						read(pfd1[0],&buffer, sizeof buffer);
						puts(buffer);
						if((flag == 1) && ((time(NULL) - time_val) > 5))
						{
							flag = 0;ola = 1;
							strcpy(buffer,"FALSE");write(pfd1[1],&buffer,sizeof buffer);
							continue;
						}
						if(strcmp(buffer,"FALSE") == 0)
						{
							strcpy(buffer,"FALSE");write(pfd1[1],&buffer,sizeof buffer);
							str = strtok(msg,"-");
							puts(str);
							if(strcmp(str,"bye") != 0)
							{
								if((n = sendto(sock,msg,MAX_LEN,0,(struct sockaddr *) &client_addr, sizeof(client_addr)))<0)
								{printf("cannot send data to %s\n",inet_ntoa(client_addr.sin_addr));}
								else
								{
									if(flag == 1)
									{ ola+=1;printf("Count is %d\n",ola);}
								}	
							}
							else{printf("Client Just Left\n");}
						}
						else
						{
							flag = 1;ola=1;
							time(&time_val);
							strcpy(buffer,"FALSE");write(pfd1[1],&buffer,sizeof buffer);
						}
						
					}
				}
			}
			else
			{
				strcpy(buf,"START");write(pfd2[1],&buf,sizeof buf);
				while(1)
				{	
					sleep(15);
					read(pfd1[0],&buffer, sizeof buffer);
					strcpy(buffer,"TRUE");write(pfd1[1],buffer,sizeof buffer);
					time(&time_val);
					ptm = localtime(&time_val);
					strftime(msg, 1024, "hello-%d/%m/%Y %H:%M:%S",ptm);
					if((n = sendto(sock,msg,MAX_LEN,0,(struct sockaddr *) &server_addr,sizeof server_addr))<0)
					{printf("cannot send data\n");continue;}
				}
			}
		}
	return 0;
} 
