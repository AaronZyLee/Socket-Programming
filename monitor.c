/*
** monitor.c 
** Zeyu Li
** ID: 8349389776
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define PORT "25776" // the TCP port of aws that monitor connect to

#define MAXDATASIZE 4000 // max number of bytes we can get at once 
#define IPADDRESS "127.0.0.1"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//split string s by spliter and put the result in res
int split(char *s,char *res[],char *spliter)
{
  char temp[MAXDATASIZE];
  strcpy(temp,s);
  char *p=strtok(temp,spliter);
  int i=0;
  while(p!=NULL){
    res[i++]=p;
    p=strtok(NULL,spliter);
  }
  return 0;
}

int main(void)
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char function[10];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(IPADDRESS, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("monitor : socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("monitor : connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "monitor : failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("The monitor is up and running.\n");

	freeaddrinfo(servinfo); // all done with this structure
	
	while (1){
		memset(buf,'\0',sizeof(buf));
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
	    	perror("recv");
	    	exit(1);
		}
		//printf("debug: receive from aws <%s>\n",buf);

		char *paramArr[10];
		split(buf,paramArr,"\t");
		strcpy(function,paramArr[0]);
		if(strcmp(function,"write")==0)
		{
			printf("The monitor received BW = <%s>, L = <%s>, V = <%s> and P = <%s> from the AWS\n",paramArr[1],paramArr[2],paramArr[3],paramArr[4]);
			memset(buf,'\0',sizeof(buf));
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			printf("The write operation has been completed successfully\n");

		}
		else if(strcmp(function,"compute")==0)
		{
			char link_id[10];
			printf("The monitor received link ID=<%s>, size=<%s>, and power=<%s> from the AWS\n", paramArr[1],paramArr[2],paramArr[3]);
			strcpy(link_id,paramArr[1]);
			memset(buf,'\0',sizeof(buf));
			if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
				perror("recv");
				exit(1);
			}
			if(strcmp(buf,"0")!=0)
			{
				split(buf,paramArr,"\t");
				printf("The result for link <%s>:\nTt=<%s>ms,\nTp=<%s>ms,\nDelay=<%s>ms\n",link_id,paramArr[0],paramArr[1],paramArr[2]);
			}else{
				printf("Link ID not found\n");
			}
		}
	}
	close(sockfd);

	return 0;
}

