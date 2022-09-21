/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];

    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    while(1) {

	/* get a message from the user */
	bzero(buf, BUFSIZE);
	printf("commands:\n");
	printf(" -get [file_name]\n");
	printf(" -put [file_name]\n");
	printf(" -delete [file_name]\n");
	printf(" -ls\n");
	printf(" -exit\n");
	printf("client: ");
	fgets(buf, BUFSIZE, stdin);
	
    	/* send the message to the server */
    	serverlen = sizeof(serveraddr);
    	n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    	if (n < 0) 
    	  error("ERROR in sendto");
    	
	bzero(buf, BUFSIZE);
 
	/* print the server's reply */
    	n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    	if (n < 0) 
    	  error("ERROR in recvfrom");
    	printf("server: %s\n", buf);

	// exit
	if (strncmp(buf, "Goodbye!", 8) == 0) {
	    break;
	}
	// ls
	else if (strncmp(buf, "ls", 2) == 0) {

	    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
            if (n < 0)
              error("ERROR in recvfrom");
            
	    printf("%s\n",buf);
        }
	// get
	else if (strncmp(buf, "get", 3) == 0){

	    char f1[BUFSIZE];
	    strncpy(f1,&buf[4],BUFSIZE);

	    char filename[strlen(f1) - 1];
	    strncpy(filename, f1, strlen(f1) - 1);
	    printf("%s\n",filename);

	    FILE *fp;
	    char test[BUFSIZE];
	    fp = fopen(filename, "wb");

	    while (1) {
		bzero(test, BUFSIZE);

		n = recvfrom(sockfd, test, BUFSIZE, 0, &serveraddr, &serverlen);
            	if (n < 0)
             	    error("ERROR in recvfrom");

		if (strncmp(test, "DONEDONE", 8) == 0) {
		    printf("DONEDONE\n");
		    break;
		}
		else if (strncmp(test, "File not found",14) == 0) {
		    remove(filename);
		    printf("%s\n",test);
		    break;
		}

		fwrite(test, 1, BUFSIZE, fp);
	        bzero(test, BUFSIZE);
	    }
	    fclose(fp);
	}
	// put
	else if (strncmp(buf, "put", 3) == 0) {

		char f1[BUFSIZE];
		strncpy(f1,&buf[4],BUFSIZE);
		printf("Sending file: %s",f1);

		char filename[strlen(f1) - 1];
		strncpy(filename, f1, strlen(f1) - 1);

		FILE *fp;
		char test[BUFSIZE];
		fp = fopen(filename, "rb");
		int read = 1;

		if (access(filename, F_OK) == 0) {
		    while (1) {
			bzero(test, BUFSIZE);

			read = fread(test, 1, BUFSIZE, fp);

			if (read < 1) break;

			n = sendto(sockfd, test, BUFSIZE, 0, &serveraddr, serverlen);
			if (n < 0)
			    error("ERROR in sendto");

			bzero(test, BUFSIZE);
			usleep(1);
		    }
		    n = sendto(sockfd, "DONEDONE", BUFSIZE, 0, &serveraddr, serverlen);
		    if (n < 0)
			error("ERROR in sendto");

		    printf("File sent\n\n");
		}
		else {
		    printf("Bad\n");
		    strcpy(test, "File not found");
		    n = sendto(sockfd, test, BUFSIZE, 0, &serveraddr, serverlen);
		    if (n < 0)
			error("ERROR in sendto");
		}
    	}
	// delete
	else if (strncmp(buf, "delete", 7) == 0) {

            printf("deleted\n");
        }

    }
    return 0;
}
