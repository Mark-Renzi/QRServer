/* connect with a TCP server */
/* this is the TCP client code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>


#define MAXBUFLEN 100

int main(int argc, char* argv[])
{	
	char buf[MAXBUFLEN];
	memset( buf, '\0', sizeof(char)*MAXBUFLEN );
	
	int PORT = 2012;
	char *strPORT = "2012";
	
	if( argc > 1 ) {
		//specific PORT
		for (int k = 1; k < argc; k++) {
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
	
	struct addrinfo hints,*server;
	int r,sockfd;

	/* configure the host */
	printf("Configuring host...");
	memset( &hints, 0, sizeof(struct addrinfo) );	/* use memset_s() */
	hints.ai_family = AF_INET;			/* IPv4 connection */
	hints.ai_socktype = SOCK_STREAM;	/* TCP, streaming */
	/* connection with localhost (zero) on port 8080 */
	r = getaddrinfo( 0, strPORT, &hints, &server );
	if( r!= 0 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* create the socket */
	printf("Assign a socket...");
	sockfd = socket(
			server->ai_family,		/* domain, TCP here */
			server->ai_socktype,	/* type, stream */
			server->ai_protocol		/* protocol, IP */
			);
	if( sockfd==-1 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* bind - name the socket */
	printf("Connecting socket...");
	r = connect(sockfd,
			server->ai_addr,
			server->ai_addrlen
			);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}
	puts("done");

	/* at this point you can interact with the TCP server */
	
	int res = -1;
	
	//strcpy(buf, "Hello World!\n");
	
	//Get Picture Size
	FILE *picture;
	picture = fopen("resources.png", "r");
	uint32_t size;
	fseek(picture, 0, SEEK_END);
	size = ftell(picture);
	fseek(picture, 0, SEEK_SET);
	
	//Send Picture Size
	write(sockfd, &size, sizeof(size));
	sleep(5);
	//Send Picture as Byte Array
	printf("Sending Picture as Byte Array\n");
	char send_buffer[size];
	while(!feof(picture)) {
	    fread(send_buffer, 1, sizeof(send_buffer), picture);
	    write(sockfd, send_buffer, sizeof(send_buffer));
	    bzero(send_buffer, sizeof(send_buffer));
	}

	sleep(5);

	/* free allocated memory */
	freeaddrinfo(server);
	/* close the socket */
	close(sockfd);
	puts("Socket closed, done");
	return(0);
}
