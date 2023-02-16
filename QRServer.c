#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define TRUE 1
#define FALSE 0

struct parameters
{
   int fd;
   struct sockaddr *clad;
};

pthread_mutex_t reqsmutex, connmutex;

int PORT, RATE1, RATE2, MAX_USERS, TIME_OUT, reqs, i, done;

void log_pref_msg(struct sockaddr *clad, char *msg){
	int MAXBUFLEN = 100;
	char output[MAXBUFLEN];
	memset( output, '\0', sizeof(char)*MAXBUFLEN );

	time_t rawtime;
	struct tm * timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	    
	sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d ", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	struct sockaddr_in *their_addr = (struct sockaddr_in *)clad;
	char *ip = inet_ntoa(their_addr->sin_addr);
	
	strcat(output, ip);
	strcat(output, " ");
	strcat(output, msg);
	strcat(output, "\n");
	
	FILE *f;
	f = fopen("server.log", "a+"); 
	if (f == NULL) { exit(-1); }
	fprintf(f, "%s", output);
	fclose(f);
}

void log_msg(char *msg){
	int MAXBUFLEN = 100;
	char output[MAXBUFLEN];
	memset( output, '\0', sizeof(char)*MAXBUFLEN );

	time_t rawtime;
	struct tm * timeinfo;
	
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	    
	sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d ", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	
	strcat(output, msg);
	strcat(output, "\n");
	
	FILE *f;
	f = fopen("server.log", "a+"); 
	if (f == NULL) { exit(-1); }
	fprintf(f, "%s", output);
	fclose(f);
}


//void func(int signum)
//{
//    wait(NULL);
//}

void *handleClient (void *pars) { ////////////////////////////////////////////////////////////////////
	
	struct parameters *p1 = (struct parameters*)pars;
	int fd = p1->fd;
	struct sockaddr *address = p1->clad;
	
	pthread_mutex_unlock(&connmutex);
	
	struct sockaddr_in *their_addr = (struct sockaddr_in *)address;
	char *ip = inet_ntoa(their_addr->sin_addr);
	
	int r = 0;
	struct timeval tv;
	tv.tv_sec = TIME_OUT;
	tv.tv_usec = 0;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	//int *numP = pars;
	//int fd = *numP;
	
	char buffer[BUFSIZ];
	memset( buffer, '\0', sizeof(char)*BUFSIZ );
	
	//fd_set this_fd;
	//FD_ZERO(&this_fd);			/* initialize file descriptor set */
	//FD_SET(fd, &this_fd); /* add current client to watchlist */
	
	while(!done)
	{
		
		/* scan the connection for any activity */
		//r = select(MAX_USERS+1,&this_fd, NULL, NULL, 0);
		//if( r==-1 )
		//{
		//	perror("failed");
		//	exit(1);
		//}
	
		/* read the input buffer for the current fd */
		memset( buffer, '\0', sizeof(char)*BUFSIZ );
		r = recv(fd,buffer,4,0);
		/* if nothing received, disconnect them */
		//printf("err: %d\n",errno);printf("r: %d\n", r);fflush(stdout);
		if( r<1 )
		{
			/* clear the fd and close the connection */
			//FD_CLR(fd, &this_fd);	/* reset in the list */
			if (errno == 11){
				errno = 0;
				uint32_t rCode = 2;
				send(fd, &rCode, sizeof(rCode), 0); //send return code
				char msg[30];
				memset( msg, '\0', sizeof(char)*30 );
				uint32_t sz = 30;
				send(fd, &sz, sizeof(sz), 0); //send size of char array
				strcpy(msg, "Timeout");
				send(fd,msg,30,0);
				log_pref_msg(address, "Timeout");
				/* update the screen */
				printf("%s timed out\n",ip);
			} else {
				log_pref_msg(address, "Client connection closed");
				/* update the screen */
				printf("%s closed\n",ip);
			}
			close(fd);				/* disconnect */
			return(0);
		}
		/* something has been received */
		else
		{
			
			buffer[r] = '\0';		/* terminate the string */
			printf("<%s>", buffer); fflush(stdout);
			/* if 'shutdown' sent... */
			if( strncmp(buffer,"shut", 4)==0 ){
				done = TRUE;		/* terminate the loop */
				log_pref_msg(address, "Shutdown request by user");
			/* otherwise, do something with text */
			} else if (strcmp(buffer,"echo")==0 ) {
				memset( buffer, '\0', sizeof(char)*BUFSIZ );
				r = recv(fd,buffer,BUFSIZ,0);
				send(fd,buffer,strlen(buffer),0);
				log_pref_msg(address, "echoed message");
			} else if (strcmp(buffer,"")==0 ) {
			} else {
				
				uint32_t *sizeP = (uint32_t *)buffer;
				uint32_t size = *sizeP;
				
				uint32_t rCode = 0;
				
				char fileName[100] = "";
				memset( fileName, '\0', sizeof(char)*100 );
				
				char outFile[100] = "";
				memset( outFile, '\0', sizeof(char)*100 );
				
				snprintf(fileName, 100, "%d.png", fd);
				
				snprintf(outFile, 100, "%d.txt", fd);
				
				//read picture
				
				if (size > 5242880){
					rCode = 1;
					write(fd, &rCode, sizeof(rCode)); //send return code
					close(fd);
				} else {
					//Read Picture Byte Array
					char p_array[size];
					memset(p_array, '\0', sizeof(char)*size);

					r = recv(fd,p_array,size,0);
					
					pthread_mutex_lock(&reqsmutex);
					if (++reqs > RATE1) {
						pthread_mutex_unlock(&reqsmutex);
						// Rate limit exceeded
						rCode = 3;
						send(fd, &rCode, sizeof(rCode), 0); //send return code
						char msg[30];
						memset( msg, '\0', sizeof(char)*30 );
						uint32_t sz = 30;
						send(fd, &sz, sizeof(sz), 0); //send size of char array
						strcpy(msg, "Rate limit exceeded");
						send(fd,msg,30,0);
						log_pref_msg(address, "Rate limit exceeded");
					} else {
						pthread_mutex_unlock(&reqsmutex);
						//Convert it Back into Picture
						//printf("Converting Byte Array to Picture\n");
						FILE *image;
						image = fopen(fileName, "w");
						fwrite(p_array, 1, sizeof(p_array), image);
						fclose(image);

						char command[200];
						memset( command, '\0', sizeof(char)*200 );
						
						strcpy( command, "java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner --dump_results " );
						strcat(command, fileName);
						system(command);
						
						bzero(buffer, sizeof(buffer));
						
						if (access(outFile, F_OK) == 0) {
							// file exists
							rCode = 0;
							write(fd, &rCode, sizeof(rCode)); //send return code
								
							FILE *fp;
							uint32_t lSize;

							fp = fopen ( outFile , "r" );
							
							fseek( fp , 0L , SEEK_END);
							lSize = ftell( fp );
							rewind( fp );
					  		
					  		fread(buffer, lSize, 1, fp);
					  		
					  		uint32_t buflen = strlen(buffer);
							//write(fd, &buflen, sizeof(buflen)); //send size of char array
							//write(clientfd, &buffer, sizeof(buffer)); //send URL as char array
							fflush(stdout);
							send(fd, &buflen, sizeof(buflen), 0); //send size of char array
							send(fd,buffer,strlen(buffer),0); //send URL as char array
								
							fclose(fp);
							log_pref_msg(address, "sent URL: ");
							log_pref_msg(address, buffer);
							remove(outFile);
						} else {
							// file doesn't exist
							rCode = 1;
							//write(fd, &rCode, sizeof(rCode)); //send return code
							send(fd, &rCode, sizeof(rCode), 0); //send return code
							uint32_t sz = 0;
							//write(fd, &sz, sizeof(sz)); //send size 0 char array
							send(fd, &sz, 0, 0); //send size 0 char array
							log_pref_msg(address, "file doesn't exist");
						}
						
						remove(fileName);
						//send(fd,buffer,strlen(buffer),0);
					}
					
					
				}
				//send(fd,buffer,strlen(buffer),0);
			}
		}// end of something has been received
	
	}//end of while loop

} ////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	PORT = 2012;
	RATE1 = 3;
	RATE2 = 60;
	MAX_USERS = 3;
	TIME_OUT = 80;
	reqs = 0;
	
	struct timeval tv;
	tv.tv_sec = 0;
    tv.tv_usec = 100;
	
	char *strPORT = malloc( sizeof(char) * (5) );
	strcpy(strPORT, "2012");

	int lastTime = time(NULL);
	
	pthread_mutex_init(&reqsmutex, NULL);
	pthread_mutex_init(&connmutex, NULL);
	
	if( argc > 1 ) {
		//specific arguments
		for (int k = 1; k < argc; k++) {
			if (strcmp(argv[k], "PORT") == 0){
				PORT = atoi(argv[++k]);
				if (PORT < 2000 || PORT > 3000) {
					log_msg("Possible port values are 2000 - 3000");
					exit(-1);
				}
				sprintf(strPORT, "%d", PORT);
			} else if (strcmp(argv[k], "RATE") == 0) {
				RATE1 = atoi(argv[++k]);
				RATE2 = atoi(argv[++k]);
			} else if (strcmp(argv[k], "MAX_USERS") == 0) {
				MAX_USERS = atoi(argv[++k]);
			} else if (strcmp(argv[k], "TIME_OUT") == 0) {
				TIME_OUT = atoi(argv[++k]);
			} else {
				log_msg("Error parsing arguments");
				exit(-1);
			}
		}
	}
	//const char *port = "65001";		/* available port */
	const char *welcome_msg = "Type 'close' to disconnect; 'shutdown' to stop; 'qr' to send a QR code; any message for an echo\n";
	const int hostname_size = 32;
	char hostname[hostname_size];
	char buffer[BUFSIZ];
	memset( buffer, '\0', sizeof(char)*BUFSIZ );
	const int backlog = MAX_USERS;			/* also max connections */
	char connection[backlog][hostname_size];	/* storage for IPv4 connections */
	socklen_t address_len = sizeof(struct sockaddr);
	struct addrinfo hints, *server;
	struct sockaddr address;
	int r,max_connect,fd;
	fd_set main_fd,read_fd;
	int serverfd,clientfd;

	/* configure the server */
	memset( &hints, 0, sizeof(struct addrinfo));	/* use memset_s() */
	hints.ai_family = AF_INET;			/* IPv4 */
	hints.ai_socktype = SOCK_STREAM;	/* TCP */
	hints.ai_flags = AI_PASSIVE;		/* accept any */
	r = getaddrinfo( 0, strPORT, &hints, &server );
	if( r!=0 )
	{
		perror("failed");
		exit(1);
	}

	/* create a socket */
	serverfd = socket(server->ai_family,server->ai_socktype,server->ai_protocol);
	if( serverfd==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* bind to a port */
	r = bind(serverfd,server->ai_addr,server->ai_addrlen);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}

	/* listen for a connection*/
	puts("TCP Server is listening...");
	r = listen(serverfd,backlog);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}
	char *a=malloc(sizeof(char)*50);
	strcpy(a, "TCP Server started listening on PORT ");
	strcat(a, strPORT);
	log_msg(a);
	free(a);
	/* deal with multiple connections */
	max_connect = backlog;		/* maximum connections */
	FD_ZERO(&main_fd);			/* initialize file descriptor set */
	FD_SET(serverfd, &main_fd);	/* set the server's file descriptor */
	
	pthread_t tid[MAX_USERS + 10];
   	i = 0;
	/* endless loop to process the connections */
	done = FALSE;
	while(!done)
	{
		
		/* backup the main set into a read set for processing */
		read_fd = main_fd;

		/* scan the connections for any activity */
		r = select(max_connect+1,&read_fd, NULL, NULL, &tv);
		if( r==-1 )
		{
			perror("failed");
			exit(1);
		} 
		//printf("%d", done);fflush(stdout);
		if (time(NULL) > lastTime + RATE2){
			lastTime = time(NULL);
			pthread_mutex_lock(&reqsmutex);
			reqs = 0;
		}

		/* process any connections */
		for( fd=1; fd<=max_connect; fd++)
		{
			/* filter only active or new clients */
			if( FD_ISSET(fd,&read_fd) )	/* returns true for any fd waiting */
			{
				/* filter out the server, which indicates a new connection */
				if( fd==serverfd )
				{
					/* add the new client */
					clientfd = accept(serverfd,&address,&address_len);
					if( clientfd==-1 )
					{
						perror("failed");
						exit(1);
					}
					/* connection accepted, get name */
					r = getnameinfo(&address,address_len,hostname,hostname_size,0,0,NI_NUMERICHOST);
					/* update array */
					strcpy(connection[clientfd],hostname);
					printf("New connection from %s\n",connection[clientfd]);
					log_pref_msg(&address, "New client connection");

					/* add new client socket to the master list */
					//FD_SET(clientfd, &main_fd); //now done in threads

					/* respond to the connection */
					strcpy(buffer,"Hello to ");
					strcat(buffer,connection[clientfd]);
					strcat(buffer,"!\n");
					strcat(buffer,welcome_msg);
					send(clientfd,buffer,strlen(buffer),0);
					
					pthread_mutex_lock(&connmutex);
					struct parameters p1 = {clientfd, &address};
					if( pthread_create(&tid[i++], NULL, (*handleClient), &p1) != 0 ){
						printf("Failed to create thread\n");
					}
					pthread_mutex_unlock(&connmutex);
				} /* end if, add new client */
				/* the current fd has incoming info - deal with it */
				else
				{
					//added support in threads
				} /* end else to send/recv from client(s) */
			} /* end if */
		} /* end for loop through connections */
		
		if( i >= MAX_USERS)
		{
			i = 0;
			while(i < MAX_USERS)
			{
				pthread_join(tid[i++],NULL);
			}
			i = 0;
		}
		
	} /* end while */

	puts("TCP Server shutting down");
	log_msg("TCP Server shutting down...");
	close(serverfd);
	freeaddrinfo(server);
	i = 0;
	while(i < MAX_USERS)
	{
		if (&tid[i] == NULL)
			pthread_kill(tid[i],1);
		else
			break;
		i++;
	}
	return(0);
}
