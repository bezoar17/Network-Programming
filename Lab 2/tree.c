/*************************
*	Vaibhav Kashyap      *     
*	 2012A3PS143P        *   
*************************/
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
int main()
{
	int n;	
	printf("Enter n\n");
	scanf("%d",&n);
	if(n==0)
	{
		printf("N cannot be 0. Enter another no.\n");
		scanf("%d",&n);
	}	
	printf("********* Master pid %d *********\n",getpid());
	printf("Position pid \t ppid \t Level  dots \n");
	create_node(n,n,1);//calling recursive node creating function
	printf("********* All Children Executed *********\n");
	return 0;
}
create_node(int k,int n,int pos)
{
	int status,i,j;pid_t pid;//initialization
	if(k!=1) //recursive cases
	{

		for(i=1;i<=k;i++)//creates k no. of nodes wher each have k-1 childs.
		{	
			
			if((pid=fork())<0)
				perror ("fork");
			else if(pid==0)//child process goes futher to create more nodes(acts both like parent and child).
			{
				create_node(k-1,n,1);				
				printf("%d \t %d \t %d \t %d \t",pos,(int)getpid(),(int)getppid(),n+1-k); //recursive case child prints AFTER creation of smaller nodes(based on no. of child processes), then prints and exits.
				for(j=1;j<=pos;j++) printf("."); // level is calculated by no.of children in node i.e k; position is calculated with no. of similar nodes.
				printf("\n");
				exit(0);
			}
			else
			{
				wait(&status);	//parent behaviour of node is to wait for child processes to get over.	
		                pos++;	       //after each child process next child's position is incremented.			
				if(i==k)      //all parallel nodes have been created.			
				return ;	
							
			}
		}

	}
	else //base case where k=1
	{
			if((pid=fork())<0)
				perror ("fork");
			else if(pid==0)      //base case child directly exits after printing.
			{
										
				printf("%d \t %d \t %d \t %d \t.\n",1,(int)getpid(),(int)getppid(),n);
				exit(0);
				
			}
			else
			{
			wait(&status); //base case parent waits for child to finish then returns.
			return;
			}	
		
	}			
}




