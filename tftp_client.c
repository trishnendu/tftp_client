#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 69
#define BUFLEN 1024 

struct sockaddr_in serv_addr;
    
int setup_client(int *sockid){
	struct timeval timeout;
    
	if ((*sockid = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        printf("\n Socket creation error \n");
        return 1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");	//"10.2.88.231"
    memset(serv_addr.sin_zero, '\0', sizeof serv_addr.sin_zero); 
   
    timeout.tv_sec = 5; 
	timeout.tv_usec = 0;  
	setsockopt(*sockid, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(struct timeval));
    
    printf("Connected to server\n");
    return 0;
}

void tftp_get(const int sockid, const char *filename){
	int flag, ssize, cnt = 0, btcnt = 0;
    char ack[4];
    char rrq[50];
    unsigned char recvbuf[1024];
    unsigned short count;
    FILE *fp;
    int addr_len = sizeof(struct sockaddr);
    
    sprintf(rrq,"%c%c%s%coctet%c",0x00, 0x01, filename, 0x00, 0x00);
    
    do{
		flag = sendto(sockid, rrq, 9+strlen(filename), 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
		ssize = recvfrom(sockid, recvbuf, 1024, 0, (struct sockaddr *) &serv_addr,(socklen_t *) &addr_len);
	}while(recvbuf[1] != 3);	    
    
    fp = fopen(filename,"w");            	
    sprintf(ack, "%c%c", 0x00, 0x04);
        
    while(1){
        cnt++;
        ack[2] = (cnt & 0xFF00) >> 8;
        ack[3] = (cnt & 0x00FF);

		do{
			flag = sendto(sockid, ack, 10, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr)); 
		}while((ack[2] != recvbuf[2]) || (ack[3] != recvbuf[3]));
        
        btcnt += fwrite(recvbuf+4, 1, ssize - 4, fp);
        if(ssize != 516){
        	break;
        }		
        ssize = recvfrom(sockid, recvbuf, 1024, 0, (struct sockaddr *) &serv_addr,(socklen_t *) &addr_len);	
    }
    fclose(fp);
    printf("Packets recieved : %d  bytes recieved = %d\n",cnt,btcnt);
}

void tftp_put(const int sockid, const char *filename){
	int flag, btcnt, ssize = 512;
    char packetbuf[520];
    unsigned char recvbuf[520], b[2];
    unsigned short count;
    FILE *fp = fopen(filename, "r");
    int addr_len = sizeof(struct sockaddr);
    
     
    sprintf(packetbuf,"%c%c%s%coctet%c",0x00, 0x02, filename, 0x00, 0x00);
    
    while(1){
    	
    	do{
			flag = sendto(sockid, packetbuf, ssize + 4, 0, (struct sockaddr *) &serv_addr, sizeof(serv_addr));	
			if(flag != ssize + 4)
		        fprintf(stderr,"SENDING FAILED!");

		    memset(recvbuf, 0, 520);
		   	flag = recvfrom(sockid, recvbuf, 516, 0, (struct sockaddr *) &serv_addr,(socklen_t *) &addr_len);
		        
		}while((recvbuf[2] != b[0]) || (recvbuf[3] != b[1]));
        
        if (ssize != 512)
            break;
        
        ssize = fread((char *) packetbuf + 4, 1 , 512, fp);
        count++; 
        btcnt += ssize;
        sprintf(packetbuf, "%c%c", 0x00, 0x03);
        
        b[0] = packetbuf[2] = (count & 0xFF00) >> 8;
        b[1] = packetbuf[3] = (count & 0x00FF);
        
	}
	printf("Packets sent : %d  bytes sent = %d\n",count, btcnt);
	fclose(fp);
}
  
int close_sock(const int sockid){
	if (shutdown(sockid, SHUT_RDWR) < 0){ 
        perror("shutdown error");
    	return 1;    
    }
    if(close(sockid) < 0){
    	perror("close error");
    	return 2;     
	}
	printf("Connection closed\n");
	return 0;
}
 
int main(int argc, char const *argv[]){
    int sock,flag;
    pthread_t rtid, wtid;
    if(argc != 3){
    	fprintf(stderr, "Invalid arguments USAGE: ./tftp_client <get/put> <filename>\n");
    	exit(0);
    }
    if(flag = setup_client(&sock)){
    	printf("Error code %d\n",flag);
    	return 0;
    }
    if(!strcmp(argv[1],"get")){
    	   tftp_get(sock, argv[2]);
    }else if(!strcmp(argv[1],"put")){
    	   tftp_put(sock, argv[2]);
    }else{
    	fprintf(stderr, "Invalid arguments USAGE: ./tftp_client <get/put> <filename>\n");
    	exit(0);
    }
    return 0;
}
