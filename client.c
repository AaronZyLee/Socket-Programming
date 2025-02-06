/*
** client.c 
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
#define PORT "24776" // the TCP port of aws that client connect to

#define MAXDATASIZE 5000 // max number of bytes we can get at once 
#define MAXBUFLEN 4000
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
//convert params to send_data format
//format:seperate parameters with tab
char* paramsToSend(int argc, char *argv[])
{
	char *params = (char*) malloc(1000);
	memset(params,'\0',1000);
	for(int i=1;i<argc;i++){
		strcat(params,argv[i]);
		if(i!=argc-1){
			strcat(params,"\t");
		}	
	}
	return params;
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char send_data[MAXBUFLEN];
	char *params;

	if (argc != 5 && argc != 6) {
	    fprintf(stderr,"usage: client write <BW> <LENGTH> <VELOCITY> <NOISEPOWER> OR client compute <LINK_ID> <SIZE> <SIGNALPOWER>\n");
	    exit(1);
	}

	if(!(strcmp(argv[1],"write")==0 || strcmp(argv[1],"compute")==0)) {
		fprintf(stderr, "function should be write OR compute\n");
		return 1;
	}

	if(strcmp(argv[1],"write")==0 && argc != 6){
		fprintf(stderr, "usage: client write <BW> <LENGTH> <VELOCITY> <NOISEPOWER>\n");
		return 1;
	}

	if(strcmp(argv[1],"compute")==0 && argc != 5){
		fprintf(stderr, "client compute <LINK_ID> <SIZE> <SIGNALPOWER>\n");
		return 1;		
	}

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
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("The client is up and running.\n");

	freeaddrinfo(servinfo); // all done with this structure
	
	//send write function to AWS
	params = paramsToSend(argc,argv);
	strcpy(send_data,params);
	if(strcmp(argv[1],"write")==0){
		if ((numbytes = send(sockfd, send_data, strlen(send_data), 0)) == -1) {
			perror("send");
			exit(1);
		}
		printf("The client sent <%s> operation to AWS.\n",argv[1]);
		if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		buf[numbytes] = '\0';
		printf("The %s operation has been completed successfully\n",argv[1]);
	}
	//send compute function to AWS
	else if(strcmp(argv[1],"compute")==0){
		if ((numbytes = send(sockfd, send_data, strlen(send_data), 0)) == -1) {
			perror("send");
			exit(1);
		}
		printf("The client sent ID=<%s>, size=<%s>, and power=<%s> to AWS.\n",argv[2],argv[3],argv[4]);	
		if ((numbytes = recv(sockfd, buf, MAXBUFLEN, 0)) == -1) {
			perror("recv");
			exit(1);
		}
		buf[numbytes]='\0';
		if(strcmp(buf,"0")!=0){
			char *recv_res[5];
			split(buf,recv_res,"\t");
			printf("The delay for link <%s> is <%s>ms\n",argv[2],recv_res[2]);
		}else{
			printf("Link ID not found\n");
		}

	}
	close(sockfd);

	return 0;
}

