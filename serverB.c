/*
** serverB.c 
** Zeyu Li
** ID: 8349389776
based on datagram sockets "server" demo, listener.c in beej
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <math.h>

#define MYPORT "22776"  // the port users will be connecting to, Server B

#define MAXBUFLEN 4000

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

double dbmToWatt(double dbm)
{
    return pow(10,(dbm-30)/10.0);
}

double computeTp(double length,double velocity)
{
    return length*1000/velocity;
}
double computeTt(double bw,double size,double power,double noise)
{
    double P=dbmToWatt(power);
    double N=dbmToWatt(noise);
    double C=bw*log(1+P/N)/log(2);
    return size/(1000*C);
}
double round2Decimal(double x)
{
    return round(x*100)/100;
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

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    char params[MAXBUFLEN];
    char function[10];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char send_data[MAXBUFLEN];

    double bw,length,velocity,noise,size,power,tt,tp,delay;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    addr_len = sizeof their_addr;

    printf("The Server B is up and running using UDP on port <%s>.\n", MYPORT);

    while(1) {
        
        memset(params,'\0',sizeof(params));
        
		if ((numbytes = recvfrom(sockfd, params, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len))==-1) {
    		perror("recv");
	    	exit(1);
    	}
        params[numbytes] = '\0';
        //printf("debug: params is %s\n", params);

        char* paramArr[10];
        split(params,paramArr,"\t");

        printf("The Server B received link information: link <%s>, file size <%s>, and signal power <%s>\n",paramArr[0],paramArr[5],paramArr[6]);
        bw=atof(paramArr[1]);length=atof(paramArr[2]);velocity=atof(paramArr[3]);noise=atof(paramArr[4]);size=atof(paramArr[5]);power=atof(paramArr[6]);
        //printf("debug: params are %f, %f, %f, %f, %f, %f\n",bw,length,velocity,noise,size,power);
        tp=computeTp(length,velocity);
        tt=computeTt(bw,size,power,noise);
        delay=round2Decimal(tp+tt);
        tp=round2Decimal(tp);
        tt=round2Decimal(tt);
        char delay_str[50],tt_str[50],tp_str[50];
        sprintf(delay_str,"%.2f",delay);
        sprintf(tp_str,"%.2f",tp);
        sprintf(tt_str,"%.2f",tt);
        //printf("debug:result is %s, %s, %s\n",tp_str,tt_str,delay_str);
        printf("The Server B finished the calculation for link <%s>\n",paramArr[0]);
        memset(send_data,'\0',sizeof(send_data));
        strcat(send_data,tt_str);strcat(send_data,"\t");
        strcat(send_data,tp_str);strcat(send_data,"\t");
        strcat(send_data,delay_str);
        if((numbytes = sendto(sockfd,send_data,strlen(send_data),0,(struct sockaddr *)&their_addr, addr_len))==-1){
            perror("send to aws:");
            exit(1);
        }
        printf("The Server B finished sending the output to AWS\n");


        fflush(stdout); //wait for next connect

        //close(sockfd);
    }

    return 0;
}
