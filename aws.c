/*
** aws.c
** Zeyu Li
** ID: 8349389776
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORTCLIENT "24776"  // the port for TCP with client
#define PORTMONITOR "25776"  // the port for TCP with monitor
#define MYUDPPORT "23776" // the port for UDP of AWS
#define SERVERPORTA "21776"	// the port users will be connecting to 
							// Backend-Server (A)
#define SERVERPORTB "22776"	// the port users will be connecting to 
							// Backend-Server (B)
#define IPADDRESS "127.0.0.1" // local IP address

#define BACKLOG 10	 // how many pending connections queue will hold
#define MAXBUFLEN 4000

// used in clear_dead_process()
void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// use sigaction to clear dead process
void clear_dead_process()
{
	struct sigaction sa; //sigaction() code is responsible for reaping zombie processes 
						//that appear as the fork()ed child processes exit.
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}

//split string s by spliter and put the result in res
int split(char *s,char *res[],char *spliter)
{
  char temp[MAXBUFLEN];
  strcpy(temp,s);
  char *p=strtok(temp,spliter);
  int i=0;
  while(p!=NULL){
    res[i++]=p;
    p=strtok(NULL,spliter);
  }
  return 0;
}

// setup TCP at port
int setupTCP(char* port)
{
	int rv; // use for error checking of getaddrinfo()
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int yes=1;
	memset(&hints, 0, sizeof hints); //Zero the whole structure before use
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype = SOCK_STREAM; //TCP SOCK_STREAM sockets
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
		// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}
		break;
	}
	freeaddrinfo(servinfo); // all done with this structure
	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	clear_dead_process();
	return sockfd;
}

/////////////////////////////////////////////////////////////////
// setupUDP and send function,parameters to specific port
// only used in udpQuery function
////////////////////////////////////////////////////////////////
int setupUDP(char* function, char *params, char* port){
	int sockfd;
	int rv;
	int numbytes;
	struct addrinfo hints, *servinfo, *p;
	socklen_t addr_len;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(IPADDRESS, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}
		// send function to server
	if ((numbytes = sendto(sockfd, params, strlen(params), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	char *paramArr[10];
	if (strcmp(port,SERVERPORTA)==0) {
		printf("The AWS sent operation <%s> to Backend-Server A using UDP over port <%s>\n", function, MYUDPPORT);
	} else if (strcmp(port,SERVERPORTB)==0) {
		split(params,paramArr,"\t");
		printf("The AWS sent link ID=<%s>, size=<%s>, power=<%s>, and link information to Backend-Server B using UDP over port <%s>\n",paramArr[0],paramArr[5],paramArr[6],MYUDPPORT);
	}

	freeaddrinfo(servinfo); // done with servinfo
	return sockfd;
}

char* udpQuery(char* function, char *params, char* port)
{
	int sockfd;
	
	sockfd=setupUDP(function,params,port);

	char * return_recv_data=(char *) malloc(4000); // return to main
	memset(return_recv_data,'\0',100);
	char recv_data[4000]=""; // save the udp return result
	int bytes_recv;
	
	bytes_recv = recvfrom(sockfd,recv_data,sizeof recv_data,0, NULL, NULL);
	if ( bytes_recv == -1) {
	    perror("recv");
	    exit(1);
	}
   	recv_data[bytes_recv]= '\0';
   	//printf("debug in udpQuery: Received :%s, bytes_recv is %d\n",recv_data,bytes_recv);
	close(sockfd);
	strcpy(return_recv_data,recv_data);
	return return_recv_data;
}

int main(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	int sockfd_monitor,new_fd_monitor; //listen on sock_fd_monitor, new connection on new_fd_monitor
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size; // used for accept()
	char s[INET6_ADDRSTRLEN];
	char buf[100]; // temp saving place for receiving
	char params[100];
	char *paramArr[10];
	char function[10];
	int numbytes; //using in receive or send

	sockfd = setupTCP(PORTCLIENT);
	sockfd_monitor = setupTCP(PORTMONITOR);

	printf("The AWS is up and running.\n");

	int monitorOn=0;
	while (monitorOn == 0){
		// child socket connect with monitor
		sin_size = sizeof their_addr;
		new_fd_monitor = accept(sockfd_monitor, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd_monitor == -1) {
			perror("accept");
			continue;
		}

		if (!fork()) { // this is the child process
			close(sockfd_monitor); // child doesn't need the listener
			monitorOn=1;
			continue;		
		}
		close(new_fd_monitor);  // parent doesn't need this
	}

	while(1) 
	{  	// main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}


		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener
			
			memset(params,'\0',sizeof(params));

			// receive function and word from client
			if ((numbytes = recv(new_fd, params, sizeof params, 0)) == -1) {
				perror("recv");
				exit(1);
			}

			// printf("debug: params is %s\n", params);
			split(params,paramArr,"\t");
			strcpy(function,paramArr[0]);

			printf("The AWS received operation <%s> and from the client using TCP over port <%s>\n",function,PORTCLIENT);
			
			//send to monitor
			if((numbytes = send(new_fd_monitor,params,strlen(params),0)==-1)){
				perror("send to monitor");
				exit(1);
			}
			printf("The AWS sent operation <%s> and arguments to monitor using TCP over port <%s>\n", function,PORTMONITOR);

			char *recv_udp;
			if(strcmp(function,"write")==0){
				recv_udp = udpQuery(function,params,SERVERPORTA);
				//printf("debug:%s\n",recv_udp);
				printf("The AWS received response from Backend-Server A for writing using UDP over port <%s>\n",MYUDPPORT);
				//send result to monitor
				if((numbytes = send(new_fd_monitor,recv_udp,strlen(recv_udp),0)==-1)){
					perror("send to monitor");
					exit(1);
				}
				printf("The AWS sent write response to the monitor using TCP over port <%s>\n",PORTMONITOR);
			}else if(strcmp(function,"compute")==0){
				recv_udp = udpQuery(function,params,SERVERPORTA);
				//printf("debug:%s\n",recv_udp);
				//Link not found
				if(strcmp(recv_udp,"0")==0)
				{
					printf("Link ID not found\n");
				}
				//If found, send to serverB to compute
				else
				{
					printf("The AWS received link information from Backend-Server A using UDP over port <%s>\n",MYUDPPORT);
					recv_udp = udpQuery(function,recv_udp,SERVERPORTB);
					printf("The AWS received outputs from Backend-Server B using UDP over port <%s>\n",MYUDPPORT);
				}
				//send result to monitor
				if((numbytes = send(new_fd_monitor,recv_udp,strlen(recv_udp),0)==-1)){
					perror("send to monitor");
					exit(1);
				}
				printf("The AWS sent compute results to the monitor using TCP over port <%s>\n",PORTMONITOR);
			}

			//send result to client
			if((numbytes = send(new_fd,recv_udp,strlen(recv_udp),0)==-1)){
				perror("send to client");
				exit(1);
			}
			printf("The AWS sent result to client for operation <%s> using TCP over port <%s>\n",function,PORTCLIENT);

			
			
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this

	} // end of while(1)
	return 0;
}