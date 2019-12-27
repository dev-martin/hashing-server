#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"
#include "optparser.h"
#include "assigment0.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/select.h>

int main(int argc, char *argv[]) {
    
    struct init initial;
	struct hash_request request;
	struct ack ackno;
	struct hash_response response;
    struct client_arguments args = client_parseopt(argc, argv);
    char *message;
    int remaining,selectsend,selectrecv;
    fd_set recvfs,sendfs;
    struct timeval timeout;
	
   

	
	//server port 
	in_port_t servPort = args.port;

	// Create a reliable, stream socket using TCP
	 int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	 if (sock < 0)
		DieWithSystemMessage("socket() failed");

	// Construct the server address structure
	 struct sockaddr_in servAddr; // Server address
	 memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
	 servAddr.sin_family = AF_INET; // IPv4 address family
	 
	// Convert address
	 //IPv4 TCP Client 
	 int rtnVal = inet_pton(AF_INET,args.ip_address, &servAddr.sin_addr.s_addr);
	 if (rtnVal == 0)
		DieWithUserMessage("inet_pton() failed", "invalid address string");
	 else if (rtnVal < 0)
		DieWithSystemMessage("inet_pton() failed");
	 servAddr.sin_port = htons(servPort); // Server port

	 // Establish the connection to the echo server
	 if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		DieWithSystemMessage("connect() failed");
	 
	 FILE *file =fopen(args.filename, "rb");
	 
	 
	 ////INIT
	 initial.type=htonl(1);
	 initial.hashreq_num=htonl(args.hashnum);
	 

	 ssize_t initBytes = send(sock, &initial, sizeof(initial), 0);
	 if (initBytes < 0)
		DieWithSystemMessage("sendinit() failed");
			
	 

	 ////ACK
	 ssize_t ackBytes = recv(sock,&ackno ,sizeof(ackno), 0);
	 
	 ackno.type=ntohl(ackno.type);
	 ackno.hashresp_len=ntohl(ackno.hashresp_len);
	 
	 if (ackBytes < 0)
				DieWithSystemMessage("recvack() failed");
		else if(ackno.type!=2)
				 DieWithSystemMessage("ack failed");
				 
	 int n = 0;
	 
	 while(n<args.hashnum){
		 
		 FD_ZERO(&recvfs);
		 FD_SET(sock,&recvfs);
		 
		 FD_ZERO(&sendfs);
		 FD_SET(sock,&sendfs);
		 
		 timeout.tv_sec = 1;
		 timeout.tv_usec = 0;
		 
		 ////REQUEST
		 selectsend = select(sock+1,NULL,&sendfs,NULL,&timeout);
		 
		 if(n<args.hashnum && selectsend==1 ){
			 
			 
			 request.type=3;
			 request.data_len = (rand() % (args.smax - args.smin + 1)) + args.smin;
			 
			 message = malloc(8+request.data_len);
			 
			 int datalen = request.data_len;
			 char buffer[datalen];
			 
			 ssize_t readbytes = fread(buffer,1, request.data_len, file);
			 if(readbytes<0)
				DieWithSystemMessage("read() failed");
			
			request.type=htonl(request.type);
			request.data_len = htonl(request.data_len);
			
			memcpy(&message[0],&request.type,4); 
			memcpy(&message[4],&request.data_len,4);
			memcpy(&message[8],&buffer,datalen);
			
			do{
				remaining = 8+datalen;
				ssize_t reqbytes = send(sock, message, 8+datalen, 0);
				if (reqbytes < 0)
					DieWithSystemMessage("sendreq() failed");
				remaining = remaining - reqbytes;
				
			}while(remaining>0);
			
			
			
			free(message);
			message=NULL;
			n++;
		}
		
		////RESPONSE 
		selectrecv = select(sock+1,&recvfs,NULL,NULL,&timeout);
		if(n<args.hashnum+1 && selectrecv==1 ){		
			do{
				remaining = 40;
				ssize_t resbytes = recv(sock,&response ,40, 0);
				
				if (resbytes < 0)
						DieWithSystemMessage("recvrsponse() failed");
						
				else if(ntohl(response.type)!=4)
						DieWithSystemMessage("response type incorrect");
						
				remaining=remaining-resbytes;
				
			}while(remaining>0);
					
			printf("%d: 0x",ntohl(response.index)+1);
			for(size_t i = 0; i < 32; ++i) {
				printf("%02x", response.data[i]);
			}
			putchar('\n');	
		}	 
		
	}
	 fclose(file);
	 close(sock);
	 exit(0);
	 
	 
 }
