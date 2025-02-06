/*
** serverA.c 
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

#define MYPORT "21776"	// the port users will be connecting to, server A

#define MAXBUFLEN 4000

#define FILE_NAME "database.txt"

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

//search link id in database
int search(char *id, char *res)
{
    char buff[MAXBUFLEN];
    char temp[MAXBUFLEN];
    //read the file and get the total num entries
    int flag=0,num_entries=0;
    FILE *file = fopen(FILE_NAME, "r");	
    while(!feof(file))
    {
        fgets(buff,MAXBUFLEN,(FILE*)file);
        strcpy(temp,buff);
        char *p = strtok(temp,"\t");
        if(strcmp(p,id)==0)
        {
            strcpy(res,buff);
            //printf("debug:%s\n",buff);
            fclose(file);
            return atoi(id);
        }
    }
    fclose(file);
    return 0;
}

int writeToDB(int *num_entries, char *paramArr[]){
    char id[100];
    //char *paramArr[4];

    *num_entries += 1;
    sprintf(id,"%d",*num_entries);
    //split(params,paramArr,"#");

    char *entry = (char*) malloc(4000);
	memset(entry,'\0',4000);
    strcat(entry,id);

    for(int i=1;i<5;i++){
        strcat(entry,"\t");
        strcat(entry,paramArr[i]);
    }

    strcat(entry,"\n");

    FILE* file = fopen(FILE_NAME, "a+");	
    fputs(entry,file);    
    fclose(file);
    return *num_entries;
}

int main(void)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
    int id;
    int sr;//search result
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
    char params[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
	char send_data[MAXBUFLEN];
    char function[10];
    char res_buff[4000];
    char id_str[10];
    char size_str[100];
    char power_str[100];

	//char *fileName="backendA.txt";
    FILE* file = NULL;
	char * returnString;
     

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

    //create file if not exist
    file = fopen(FILE_NAME,"a");
    fclose(file);
    //read the file and get the total num entries
    int flag=0,num_entries=0;
    file = fopen(FILE_NAME, "r");	
    while(!feof(file))
    {
        flag = fgetc(file);
        if(flag == '\n')
            num_entries++;
    }
    fclose(file);
    //printf("debug:The file has %d entries\n",num_entries);

	printf("The Server A is up and running using UDP on port <%s>.\n",MYPORT);

	while(1) {

        memset(params,'\0',sizeof(params));

		if ((numbytes = recvfrom(sockfd, params, MAXBUFLEN-1 , 0,
			(struct sockaddr *)&their_addr, &addr_len))==-1) {
    		perror("recv");
	    	exit(1);
    	}

        char *paramArr[5];
        split(params,paramArr,"\t");
        strcpy(function,paramArr[0]);

        params[numbytes] = '\0';
        //printf("debug: params is %s\n", params);

        if(strcmp(function,"write")==0){
            printf("The Server A received input for writing\n");
            id = writeToDB(&num_entries,paramArr); 
            memset(res_buff,'\0',sizeof(res_buff));
            sprintf(res_buff,"%d",id);
            if ((numbytes = sendto(sockfd, res_buff, strlen(res_buff), 0,(struct sockaddr *)&their_addr, addr_len)) == -1) 
            {
                perror("talker: sendto");
                exit(1);
            }
            printf("The Server A wrote link <%d> to database\n",id);
        }else if(strcmp(function,"compute")==0){
            printf("The Server A received input <%s> for computing\n",paramArr[1]);
            memset(res_buff,'\0',sizeof(res_buff));
            strcpy(id_str,paramArr[1]);
            strcpy(size_str,paramArr[2]);
            strcpy(power_str,paramArr[3]);
            //printf("debug:%s\n",paramArr[3]);
            if((sr=search(id_str,res_buff))!=0){
                res_buff[strlen(res_buff)-1]='\0';
                strcat(res_buff,"\t");
                strcat(res_buff,size_str);
                strcat(res_buff,"\t");
                strcat(res_buff,power_str);
                //printf("debug:%s\n",res_buff);
                if ((numbytes = sendto(sockfd, res_buff, strlen(res_buff), 0,(struct sockaddr *)&their_addr, addr_len)) == -1) 
                {
                    perror("talker: sendto");
                    exit(1);
                }
                printf("The Server A finished sending the search result to AWS\n");
            }
            else{
                strcat(res_buff,"0");
                //printf("debug:%s\n",res_buff);
                if ((numbytes = sendto(sockfd, res_buff, strlen(res_buff), 0,(struct sockaddr *)&their_addr, addr_len)) == -1) 
                {
                    perror("talker: sendto");
                    exit(1);
                }
                printf("Link ID not found\n");
            }

        }

		// send back to aws

    	fflush(stdout); //wait for next connect

		//close(sockfd);
	}

	return 0;
}
