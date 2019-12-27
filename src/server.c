#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"
#include "assigment0.h"
#include "optparser.h"
#include <fcntl.h>
#include <inttypes.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "hash.h"

static const int MAXPENDING = 5; // Maximum outstanding connection requests

int main(int argc, char *argv[]) {
  struct server_arguments args = server_parseopt(argc, argv);
  size_t saltlen = 0;
  struct init initial;
  struct hash_request request;
  struct ack ackno;
  struct hash_response response;
  int remaining,selectsend,selectrecv;
  char *msgrequest ;
  char *msgresponse ;
  fd_set recvfs,sendfs;
  struct timeval timeout;  

  in_port_t servPort = args.port; // First arg:  local port

  // Create socket for incoming connections
  int servSock; // Socket descriptor for server
  if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithSystemMessage("socket() failed");

  // Construct local address structure
  struct sockaddr_in servAddr;                  // Local address
  memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure
  servAddr.sin_family = AF_INET;                // IPv4 address family
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
  servAddr.sin_port = htons(servPort);          // Local port

  // Bind to the local address
  if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0)
    DieWithSystemMessage("bind() failed");

  // Mark the socket so it will listen for incoming connections
  if (listen(servSock, MAXPENDING) < 0)
    DieWithSystemMessage("listen() failed");

  for (;;) { // Run forever
    struct sockaddr_in clntAddr; // Client address
    // Set length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(clntAddr);

    // Wait for a client to connect
    int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
    if (clntSock < 0)
      DieWithSystemMessage("accept() failed");

    // clntSock is connected to a client!

    char clntName[INET_ADDRSTRLEN]; // String to contain client address
    if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
        sizeof(clntName)) == NULL)
      puts("Unable to get client address");
    
    if(args.salt != NULL)
		saltlen = sizeof(args.salt)-1;	
	struct checksum_ctx *ctx = checksum_create((const unsigned char*)args.salt, saltlen);
    
	ssize_t numbytes = 0;
    /* RECIEVE INITIALIZATION*/
    if ((numbytes = recv(clntSock, &initial, 8, 0)) < 0)
        DieWithSystemMessage("recvinit() failed");
	else if(ntohl(initial.type) !=1)
		DieWithSystemMessage("wrong initialization");
		
	initial.type = ntohl(initial.type);
	initial.hashreq_num = ntohl(initial.hashreq_num);
	
	/*SEND ACK*/
	ackno.type = htonl(2);
	ackno.hashresp_len = htonl(initial.hashreq_num*40);
	numbytes = send(clntSock, &ackno, 8, 0);
	 if (numbytes < 0)
		DieWithSystemMessage("sendack() failed");
			
	
	int n = 0 ;	
	
    while(n<initial.hashreq_num){
		 
		 FD_ZERO(&recvfs);
		 FD_SET(clntSock,&recvfs);
		 
		 FD_ZERO(&sendfs);
		 FD_SET(clntSock,&sendfs);
		 
		 timeout.tv_sec = 1;
		 timeout.tv_usec = 0;
		 
		 /*RECIEVE HASH REQUEST*/ 
		selectrecv = select(clntSock+1,&recvfs,NULL,NULL,&timeout);
		if(n<initial.hashreq_num+1 && selectrecv==1 ){	
			msgrequest = malloc(8);
				
			numbytes = recv(clntSock,msgrequest ,8, 0);
				
			if (numbytes < 0 || numbytes < 8)
				DieWithSystemMessage("recvrequest() failed");
				
			memcpy(&request.type,&msgrequest[0],4);
			memcpy(&request.data_len,&msgrequest[4],4);  
				
			request.type = ntohl(request.type);
				
			if(request.type != 3)
				DieWithSystemMessage("request type incorrect");
						
			request.data_len = ntohl(request.data_len);
				
			remaining = request.data_len;
			
			free(msgrequest);
			msgrequest=NULL;
			
			
			msgrequest = malloc(request.data_len);
			
			do{
				
				numbytes = recv(clntSock,msgrequest ,request.data_len, 0);
				
				if (numbytes < 0)
						DieWithSystemMessage("recvrsponse() failed");						
				
				remaining=remaining-numbytes;
				
			}while(remaining>0);
			
			
		    
		}
		
		 /*SEND HASH RESPONSE*/
		 
		 selectsend = select(clntSock+1,NULL,&sendfs,NULL,&timeout);
		 if(n<initial.hashreq_num && selectsend==1 ){
			msgresponse=malloc(40);
			
			response.type=htonl(4);
			response.index=htonl(n);
			
			memcpy(&msgresponse[0],&response.type,4);
			memcpy(&msgresponse[4],&response.index,4);
			
		    uint8_t checksum[32];
			checksum_finish(ctx, (uint8_t*)msgrequest, request.data_len, checksum);
			
			memcpy(&msgresponse[8],&checksum,32);
			
			remaining = 40;
			do{
				
				numbytes = send(clntSock, msgresponse, 40, 0);
				if (numbytes < 0)
					DieWithSystemMessage("sendresponse() failed");
				remaining = remaining - numbytes;
				
			}while(remaining>0);
			
			
			
			free(msgresponse);
			msgresponse=NULL;
			free(msgrequest);
			msgrequest=NULL;
			memset(checksum,0,32);
			checksum_reset(ctx);
			n++;
		}
		
		
		
	}
	 checksum_destroy(ctx);
	 close(clntSock);
	 

  }
  // NOT REACHED
}
