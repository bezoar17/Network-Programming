/*************************
*   Vaibhav Kashyap      *     
*    2012A3PS143P        *   
**************************
**************************
*   Bhavin Senjaliya     *     
*    2012A8PS274P        *   
*************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#define SERVERPORT 8080
#define MAXCONN 200
#define MAXEVENTS 100
#define MAXLEN 255

struct EchoEvent
{
    int fd;
    uint32_t event;
    char data[MAXLEN];
    int length;
    int offset;

};


int epollfd;

void modifyEpollContext(int epollfd, int operation, int fd, uint32_t events, void* data)
{
    struct epoll_event server_listen_event;

    server_listen_event.events = events;

    server_listen_event.data.ptr = data;

    if(epoll_ctl(epollfd, operation, fd, &server_listen_event)== -1)
    {
        printf("Failed to add an event for socket%d Error:%s", fd, strerror(errno));
        exit(1);        
    }
    
}
void* handle(void* ptr)
{
    struct EchoEvent* echoEvent = ptr;

    if(EPOLLIN == echoEvent->event)
    {
        int n = read(echoEvent->fd, echoEvent->data, MAXLEN);
    
        if(n == 0)
        {
            /*
             * Client closed connection.
             */
            printf("\nClient closed connection.\n");
            close(echoEvent->fd);
            free(echoEvent);
        }
        else if(n == -1)
        {
            
            close(echoEvent->fd);
            free(echoEvent);
        }
        else
        {
            echoEvent->length = n;
        
            printf("\nRead data:%s Length:%d", echoEvent->data , n);            
            printf("\nAdding write event.\n");
           
            modifyEpollContext(epollfd, EPOLL_CTL_ADD, echoEvent->fd, EPOLLOUT, echoEvent);
        }        
    
    }
    else if(EPOLLOUT == echoEvent->event)
    {
        int ret;

         ret = write(echoEvent->fd, (echoEvent->data) + (echoEvent->offset), echoEvent->length);
        
        if( (ret == -1 && EINTR == errno) || ret < echoEvent->length)
        {
            // got EINTR or write only sent partial data.
            // Adding an write event,Still need to write data.
                             
            
            modifyEpollContext(epollfd, EPOLL_CTL_ADD, echoEvent->fd, EPOLLOUT, echoEvent);
            
            if(ret!= -1)
            {
               
                echoEvent->length = echoEvent->length - ret;
                echoEvent->offset = echoEvent->offset + ret;    
            }
            
        }
        else if(ret == -1)
        {
            close(echoEvent->fd);
            free(echoEvent);
        }
        
        else
        {    
          //Data was written. Adding read event to read more data
          printf("\nAdding Read Event.\n");    
          modifyEpollContext(epollfd, EPOLL_CTL_ADD, echoEvent->fd, EPOLLIN, echoEvent);
        }
    }

}


void makeSocketNonBlocking(int fd)
{
    int flags;
    
    flags = fcntl(fd, F_GETFL, NULL);

    if(-1 == flags)
    {
        printf("fcntl F_GETFL failed.%s", strerror(errno));
        exit(1);
    }

    flags |= O_NONBLOCK;

    if(-1 == fcntl(fd, F_SETFL, flags))    
    {
        printf("fcntl F_SETFL failed.%s", strerror(errno));
        exit(1);
    }        
}


int main(int argc, char** argv)
{
    
    int serverfd;

    struct sockaddr_in server_addr;

    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    serverfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if(-1 == serverfd)
    {
        printf("Failed to create socket.%s", strerror(errno));
        exit(1);
    }    
    
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVERPORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if( bind(serverfd, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1)
    {
        printf("Failed to bind.%s", strerror(errno));
        exit(1);
    }
    if(listen(serverfd, MAXCONN)==-1)
    {
        printf("Failed to listen.%s", strerror(errno));
        exit(1);
    }

    epollfd = epoll_create(MAXCONN);

    if(-1 == epollfd)
    {
        printf("Failed to create epoll context.%s", strerror(errno));
        exit(1);
    }

   //read event for server socket.
   modifyEpollContext(epollfd, EPOLL_CTL_ADD, serverfd, EPOLLIN, &serverfd);

   //Main loop that listens for event.
   struct epoll_event *events = calloc(MAXEVENTS, sizeof(struct epoll_event));
    while(1)
    {
        int n = epoll_wait(epollfd, events, MAXEVENTS, -1);

        if(n==-1)                
        {
            printf("Failed to wait.%s", strerror(errno));
            exit(1);
        }
        
        int i;
        for(i = 0; i < n; i++)
        {
            if(events[i].data.ptr == &serverfd)
            {
                if(events[i].events & EPOLLHUP || events[i].events & EPOLLERR)
                {
                  /*
                   * EPOLLHUP and EPOLLERR are always monitored.
                   */
                    close(serverfd);
                     exit(1);
                }    

               
               //New client Connected
                int connfd = accept(serverfd, (struct sockaddr*)&clientaddr, &clientlen);

                if(-1 == connfd)
                {
                    printf("Accept failed.%s", strerror(errno));
                    exit(1);
                }
                else
                {
                    printf("Accepted connection.Sleeping for minute.\n");
                    
                    makeSocketNonBlocking(connfd);
                
                    sleep(6);
                
                    printf("Adding a read event\n");

                    struct EchoEvent* echoEvent = calloc(1, sizeof(struct EchoEvent));

                    echoEvent->fd = connfd;
                    
                    //read
                    modifyEpollContext(epollfd, EPOLL_CTL_ADD, echoEvent->fd, EPOLLIN, echoEvent);
                }    
            }
            else
            {
                

                if(events[i].events & EPOLLHUP || events[i].events & EPOLLERR)
                {
                    struct EchoEvent* echoEvent = (struct EchoEvent*) events[i].data.ptr;
                    printf("\nClosing connection socket\n");
                    close(echoEvent->fd);
                    free(echoEvent);
                }
                else if(EPOLLIN == events[i].events)    
                {
                    struct EchoEvent* echoEvent = (struct EchoEvent*) events[i].data.ptr;
                    
                    echoEvent->event = EPOLLIN;
                 //delete read event   
                    modifyEpollContext(epollfd, EPOLL_CTL_DEL, echoEvent->fd, 0, 0);

                    handle(echoEvent);
                }
                else if(EPOLLOUT == events[i].events)    
                {
                    struct EchoEvent* echoEvent = (struct EchoEvent*) events[i].data.ptr;
                    
                    echoEvent->event = EPOLLOUT;
                    
                    //delete write event        
                    modifyEpollContext(epollfd, EPOLL_CTL_DEL, echoEvent->fd, 0, 0);

                    handle(echoEvent);
                }
            }
            
        }
    }

    free(events);
    exit(0);
}
