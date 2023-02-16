#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>

#define MAXBUFLEN 100
pthread_mutex_t mutex;
int RATE1;

void log_pref_msg(struct sockaddr *clad, char *msg){
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

void handleClient (int pars, struct sockaddr *clad) {
	uint32_t rCode = 0;
	int clientfd = pars;
	
	char fileName[100] = "";
	memset( fileName, '\0', sizeof(char)*100 );
	
	char outFile[100] = "";
	memset( outFile, '\0', sizeof(char)*100 );
	
	snprintf(fileName, 100, "%d.png", clientfd);
	
	snprintf(outFile, 100, "%d.txt", clientfd);
	
	//if not under allowed rate
	pthread_mutex_lock(&mutex);
	FILE* file = fopen ("reqs.txt", "r");
	int reqs = 0;
 
	fscanf (file, "%d", &reqs);  
	fflush(stdout);
	fclose (file);
	
	if (reqs >= RATE1) {
		pthread_mutex_unlock(&mutex);
		rCode = 3;
		write(clientfd, &rCode, sizeof(rCode)); //send return code
		log_pref_msg(clad, "WARNING: RATE LIMIT EXCEEDED");
		
		char sen[30];
		memset( sen, '\0', sizeof(char)*30);
		strcpy(sen, "WARNING: RATE LIMIT EXCEEDED");
		uint32_t sz = 30;
		write(clientfd, &sz, sizeof(sz)); //send size of char array
		write(clientfd, &sen, sizeof(sen)); //send char array
	} else { //else read picture and acquire mutex to increment rate limit value
		reqs++;
		FILE* file = fopen ("reqs.txt", "w"); //update shared file
		fprintf (file, "%d", reqs);   
		fclose (file);
		pthread_mutex_unlock(&mutex);
		uint32_t size;
		int msglen = read(clientfd, &size, sizeof(int));
		//printf("%d\n", size);
		
		if( msglen==-1 ) { //if message failed to open
			perror("failed");
			exit(1);
		}
		
		//if image specified too large (5MB)
		if (size > 5242880){
			rCode = 3;
			write(clientfd, &rCode, sizeof(rCode)); //send return code
			log_pref_msg(clad, "WARNING: FILE TOO LARGE");
			
			char sen[30];
			memset( sen, '\0', sizeof(char)*30);
			strcpy(sen, "WARNING: FILE TOO LARGE");
			uint32_t sz = 30;
			write(clientfd, &sz, sizeof(sz)); //send size of char array
			write(clientfd, &sen, sizeof(sen)); //send char array
			close(clientfd);
			return;
		}
		
		//Read Picture Byte Array
		char p_array[size];
		read(clientfd, p_array, size);
		
		//Convert it Back into Picture
		//printf("Converting Byte Array to Picture\n");
		FILE *image;
		image = fopen(fileName, "w");
		fwrite(p_array, 1, sizeof(p_array), image);
		fclose(image);
		
		
		/* do something with the result*/
		pid_t pid2;
		if ((pid2 = fork()) == 0) {
			char *args[7];
			// java -cp javase.jar:core.jar com.google.zxing.client.j2se.CommandLineRunner resources.png
			args[0] = "java";
			args[1] = "-cp";
			args[2] = "javase.jar:core.jar";
			args[3] = "com.google.zxing.client.j2se.CommandLineRunner";
			args[4] = "--dump_results";
			args[5] = fileName;
			args[6] = NULL;
			
			execvp(args[0], args);
			
		} else {
			int st2;
			//puts("waiting");
			//fflush(stdout);
			wait(&st2);
			//puts("done waiting");
			//fflush(stdout);
		}
		
		
		if (access(outFile, F_OK) == 0) {
			// file exists
			rCode = 0;
			write(clientfd, &rCode, sizeof(rCode)); //send return code
			    
			FILE *fp;
			uint32_t lSize;
			char *buffer;

			fp = fopen ( outFile , "rb" );
			
			fseek( fp , 0L , SEEK_END);
			lSize = ftell( fp );
			rewind( fp );
			
			buffer = calloc( 1, lSize+1 );
			if( !buffer ) fclose(fp),log_pref_msg(clad, "memory alloc failed"),exit(1);
			
			if( 1!=fread( buffer , lSize, 1 , fp) )
	  		fclose(fp),free(buffer),log_pref_msg(clad, "read failed"),exit(1);
	  		
	  		uint32_t buflen = strlen(buffer);
			write(clientfd, &buflen, sizeof(buflen)); //send size of char array
			write(clientfd, &buffer, sizeof(buffer)); //send URL as char array
				
			fclose(fp);
			free(buffer);
			log_pref_msg(clad, "sent URL: ");
			log_pref_msg(clad, buffer);
			remove(outFile);
		} else {
			// file doesn't exist
			rCode = 1;
			write(clientfd, &rCode, sizeof(rCode)); //send return code
			uint32_t sz = 0;
			write(clientfd, &sz, sizeof(sz)); //send size 0 char array
			log_pref_msg(clad, "file doesn't exist");
		}
		
		remove(fileName);
	
	}
	
	
	
	/* close the client socket */
	close(clientfd);
	return;
}

int main(int argc, char* argv[])
{
	pthread_mutex_init(&mutex, NULL);

	int PORT = 2012;
	RATE1 = 3;
	int RATE2 = 60;
	int MAX_USERS = 3;
	int TIME_OUT = 80;
	char *strPORT = "2012";
	
	FILE* file = fopen ("reqs.txt", "w"); //initialize shared file
	fprintf (file, "0");   
	fclose (file);
	
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

	int clientfd;
	socklen_t client_len;
	struct sockaddr client_address;
	
	char buf[MAXBUFLEN];
	memset( buf, '\0', sizeof(char)*MAXBUFLEN );

	struct addrinfo hints,*server;
	int r,sockfd;

	/* configure the host */
	//printf("Configuring host...");
	memset( &hints, 0, sizeof(struct addrinfo) );	/* use memset_s() */
	hints.ai_family = AF_INET;			/* IPv4 connection */
	hints.ai_socktype = SOCK_STREAM;	/* TCP, streaming */
	/* connection with localhost (zero) on specified PORT (default 2012) */
	r = getaddrinfo( 0, strPORT, &hints, &server );
	if( r!= 0 )
	{
		perror("failed");
		exit(1);
	}
	//puts("done");
	log_msg("Configured host");

	/* create the socket */
	//printf("Assign a socket...");
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
	//puts("done");
	log_msg("Assigned socket");

	/* bind - name the socket */
	//printf("Binding socket...");
	r = bind(sockfd,
			server->ai_addr,
			server->ai_addrlen
			);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}
	//puts("done");
	log_msg("Bound socket");

	/* listen for incoming connections */
	//printf("Listening...");
	r = listen(sockfd,1);
	if( r==-1 )
	{
		perror("failed");
		exit(1);
	}
	//puts("done");
	log_msg("Listening for incoming connections");
	
	pid_t pid2;
	//CHILD PROCESS TO TRACK RATE LIMITER // cleaned up later after main loop
	if ((pid2 = fork()) == 0) {
		struct timeval current, last;
	    	gettimeofday(&current, 0);
	    	double elapsed = 0;

		while(1){
			last = current;
			gettimeofday(&current, 0);
			long seconds = current.tv_sec - last.tv_sec;
	    		long microseconds = current.tv_usec - last.tv_usec;
	    		elapsed += seconds + microseconds*1e-6;
	    		if (elapsed >= RATE2){
	    			elapsed = 0;
	    			pthread_mutex_lock(&mutex);
	    			FILE* file = fopen ("reqs.txt", "w");
				fprintf (file, "0");   
				fclose (file);
	    			pthread_mutex_unlock(&mutex);
	    		}
		}
	}
	int st2;
    	
	pid_t pid[MAX_USERS + 10];
   	int i = 0;
	while(1) {
		//Accept call creates a new socket for the incoming connection
		//printf("Accepting new connection ");
		client_len = sizeof(client_address);
		clientfd = accept(sockfd, &client_address, &client_len);
		if( clientfd==-1 ) {
			perror("failed");
			exit(1);
		}
		int pid_child = 0;
		//printf("on file descriptor %d\n",clientfd);
		log_pref_msg(&client_address, "accepting new connection");

		//for each client request creates a child process and assign the client request to it to process
	        //so the main thread can entertain next request
		if ((pid_child = fork())==0) {
			handleClient(clientfd, &client_address);
		} else {
			pid[i++] = pid_child;
			if( i >= MAX_USERS -1) {
			   	i = 0;
				while(i < MAX_USERS)
					waitpid(pid[i++], NULL, 0);
				i = 0;
			}
		}
	}


	wait(&st2);//WAIT FOR RATE LIMITER CHILD
	
	/* free allocated memory */
	freeaddrinfo(server);
	/* close the socket */
	close(sockfd);
	puts("Socket closed, done");
	return(0);
}
