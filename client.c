#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
	int PORT = 2012;
	char *strPORT = malloc( sizeof(char) * (5) );
	strcpy(strPORT, "2012");
	if( argc<2 )
	{
		printf("optional:add 'PORT $(num)'\n");
		fprintf(stderr,"Format: ./client hostname\n");
		exit(1);
	}

	if( argc >= 2 ) {
		//specific PORT
		for (int k = 2; k < argc; k++) {
			if (strcmp(argv[k], "PORT") == 0){
				PORT = atoi(argv[++k]);
				if (PORT < 2000 || PORT > 3000) {
					printf("Possible port values are 2000 - 3000");
					exit(-1);
				}
				sprintf(strPORT, "%d", PORT);
			} else {
				printf("Error parsing arguments");
				exit(-1);
			}
		}
	}
	
	char *server;
	server = argv[1];
	struct addrinfo hints,*host;
	int r,sockfd;
	char buffer[BUFSIZ];
	uint32_t bsize = 0;

	/* obtain and convert server name and port */
	printf("Looking for server on %s...",server);
	memset( &hints, 0, sizeof(hints) );		/* use memset_s() */
	hints.ai_family = AF_INET;				/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;		/* TCP */
	r = getaddrinfo( server, strPORT, &hints, &host );
	if( r!=0 )
	{
		perror("failed");
		exit(1);
	}
	puts("found");

	/* create a socket */
	sockfd = socket(host->ai_family,host->ai_socktype,host->ai_protocol);
	if( sockfd==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* connect to the socket */
	r = connect(sockfd,host->ai_addr,host->ai_addrlen);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}


	memset( buffer, '\0', sizeof(char)*BUFSIZ );
	r= recv(sockfd,buffer,BUFSIZ,0);
	if( r<1 )
	{
		puts("Connection closed by peer");
		close(sockfd);
		return (0);
	}
	buffer[r] = '\0';
	for (int i = 0; i < r; i ++){
		printf("%c", buffer[i]);
	}

	/* loop to interact with the server */
	while(1)
	{	
		
		/* local input  */
		memset( buffer, '\0', sizeof(char)*BUFSIZ );
		printf("Command: ");
		fflush(stdout);
		fgets(buffer,BUFSIZ,stdin);
		if( strncmp(buffer,"close",5)==0 )
		{
			puts("Closing connection");
			break;
		} else if (strncmp(buffer,"shut",4)==0) {
			send(sockfd,buffer,4,0);
			break;
		} else if (strncmp(buffer,"qr",2)==0 || strncmp(buffer,"QR",2)==0) { //SEND QRCODE
			memset( buffer, '\0', sizeof(char)*BUFSIZ );

			do {
				printf("Enter Valid Filename: ");
				fgets(buffer,BUFSIZ,stdin);
				//printf("\"%s\"", buffer);
				buffer[strlen(buffer)-1] = '\0';
			} while (access(buffer, F_OK) != 0);
			
			/////////////////////////////////////////////////
			//Get Picture Size
			FILE *picture;
			picture = fopen(buffer, "r");
			uint32_t size;
			fseek(picture, 0, SEEK_END);
			size = ftell(picture);
			fseek(picture, 0, SEEK_SET);
			
			//Send Picture Size
			write(sockfd, &size, sizeof(size));
			//Send Picture as Byte Array
			printf("Sending Picture as Byte Array\n");
			char send_buffer[size];
			while(!feof(picture)) {
				unsigned long int amt = fread(send_buffer, 1, sizeof(send_buffer), picture);
				if (amt != 0){
					write(sockfd, send_buffer, amt);
				}
				bzero(send_buffer, sizeof(send_buffer));
			}
			fclose(picture);
			/////////////////////////////////////////////////
			memset( buffer, '\0', sizeof(char)*BUFSIZ );
			bsize = 0;
			r= recv(sockfd,&bsize,sizeof(bsize),0); //RECEIVE return code as uint32_t
			if( r<1 )
			{
				puts("Connection closed by peer");
				break;
			}
			//printf("<%x>",bsize);
			//printf("{%d}", r);
			if (bsize == 1){
				printf("FAILURE");
				close(sockfd);
				return(1);
			} else if (bsize == 2){
				memset( buffer, '\0', sizeof(char)*BUFSIZ );
				r= recv(sockfd,buffer,BUFSIZ,0);
				if( r<1 )
				{
					puts("Connection closed by peer");
					break;
				}
				buffer[r] = '\0';
				//printf("<%s>",buffer);
				for (int i = 0; i < r; i ++){
					printf("%c", buffer[i]);
				}
				close(sockfd);
				return(2);
			} else if (bsize == 3){ //rate limit exceeded
				r= recv(sockfd,&bsize,sizeof(bsize),0); //RECEIVE SIZE OF RETURN BUFFER AS uint32_t
				if( r<1 )
				{
					puts("Connection closed by peer");
					break;
				}
				memset( buffer, '\0', sizeof(char)*BUFSIZ );
				r= recv(sockfd,buffer,bsize,0);
				if( r<1 )
				{
					puts("Connection closed by peer");
					break;
				}
				buffer[r] = '\0';
				for (int i = 0; i < r; i ++){
					printf("%c", buffer[i]);
				}
				printf("\n"); fflush(stdout);
			} else if (bsize == 0) { //SUCCESS
				r= recv(sockfd,&bsize,sizeof(bsize),0); //RECEIVE SIZE OF RETURN BUFFER AS uint32_t
				if( r<1 )
				{
					puts("Connection closed by peer");
					break;
				}
				memset( buffer, '\0', sizeof(char)*BUFSIZ );
				r= recv(sockfd,buffer,bsize,0);
				if( r<1 )
				{
					puts("Connection closed by peer");
					break;
				}
				buffer[r] = '\0';
				for (int i = 0; i < r; i ++){
					printf("%c", buffer[i]);
				}
				printf("\n"); fflush(stdout);
			}
			
			
		} else {
			int msglen = strlen(buffer);
			char msg[msglen + 4];
			memset( msg, '\0', sizeof(char)*(msglen + 4) );
			strcpy(msg, "echo");
			strcat(msg,buffer);
			send(sockfd,msg,msglen + 4,0);
			memset( buffer, '\0', sizeof(char)*BUFSIZ );
			r= recv(sockfd,buffer,BUFSIZ,0);
			if( r<1 )
			{
				puts("Connection closed by peer");
				break;
			}
			printf("%s", buffer);
		}
	}	/* end while loop */

	/* all done, clean-up */
	//freeaddrinfo(host);
	close(sockfd);
	puts("Disconnected");

	return(0);
}
