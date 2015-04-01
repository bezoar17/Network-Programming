/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
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
int main(int argc, char const *argv[])
{
	int i=0,sd,port_no,rev,client_len,cc,file_fd,flag=0;
	pid_t pid;
	struct   sockaddr_in name,client ;
	long len,retr;
	char *ret;
	char chunked[16]="chunked=true";
	char conclose[16]="conclose=true";
	char response[2048];
	char buf[8096];
	char file_path[128];
	client_len=sizeof(client);
	printf("Enter the port no. \n");
	scanf("%d",&port_no);
	name.sin_family = AF_INET;
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    name.sin_port = htons(port_no);
    sd = socket (AF_INET,SOCK_STREAM,0);
    if(sd < 0)
        {perror("socket");exit(0);}
    if(bind(sd, (struct sockaddr*) &name, sizeof(struct sockaddr)) < 0)
        {perror("bind");exit(0);}
    if(listen(sd, 5) < 0)
        {perror("listen");exit(0);}
	for(;;)
	{
		printf("Listening on %d\n",port_no);
		rev=accept(sd, (struct sockaddr *)&client, &client_len);
		if((pid=fork())==0)
		{
			//Child
			close(sd);
			cc=recv(rev,buf,sizeof(buf),0) ;
        	if (cc == 0) exit (0);
   			buf[cc] = '\0';
   			if( strncmp(buf,"GET ",4) && strncmp(buf,"get ",4) ) 
   			{
			printf("ONLY GET requests entertained. TRY AGAIN. Exiting\n");
			exit(0);
			}
   			ret=strchr(buf,'/');
   			ret++;
   			while(1)
   			{
   				if(*ret=='?' || *ret==' ')
   					break;
   				file_path[i++]=*ret;
   				ret++;
   			}
   			file_path[i]='\0';
   			i=0;
   			if(( file_fd = open(file_path,O_RDONLY)) == -1) {  /* open the file for reading */
			printf("For file %s\n",file_path);
			perror("open");
			exit(0);
			}
   			len = (long)lseek(file_fd, (off_t)0, SEEK_END);//calculating file length
			(void)lseek(file_fd, (off_t)0, SEEK_SET); 
   			sprintf(response,"HTTP/1.1 200 OK\nServer: Hallelujah\nContent-Length: %ld\n",len);
   			ret=strstr(buf,conclose);
   			if(ret != NULL)   			 				
   			strcat(response,"Connection: close\n");
   			ret=strstr(buf,chunked);
   			if(ret!=NULL)
   			{
   				strcat(response,"Transfer Encoding: chunked\n");
	   			flag=1;
   			}   						
   			strcat(response,"\n");
   			(void)write(rev,response,strlen(response));
   			printf("Sent Response\n%s\n",response);
   			if(flag==1)
			{
				/* send file in 8KB block - last block may be smaller */
				while (	(retr = read(file_fd, buf, 8096)) > 0 ) {
					(void)write(rev,buf,retr);
				}
				printf("Sent file %s\n",file_path);				
			}
			else
			{
				char file_buffer[len];				
				while (	(retr = read(file_fd, file_buffer, len)) > 0 ) {
					(void)write(rev,file_buffer,retr);
				}
				printf("Sent file %s\n",file_path);
			}
   			sleep(1);   			
   			close(file_fd);
   			exit(0);//child exits
		}
		else
		{
			close(rev);//nothing
		}
		
	}
	
	return 0;
}