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
	printf("Please enter msg: ");
	fgets(buf, BUFSIZE, stdin);
	
    	/* send the message to the server */
    	serverlen = sizeof(serveraddr);
    	n = sendto(sockfd, buf, BUFSIZE, 0, &serveraddr, serverlen);
    	if (n < 0) 
    	  error("ERROR in sendto");
    	
	bzero(buf, BUFSIZE);
    	/* print the server's reply */
    	n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
    	if (n < 0) 
    	  error("ERROR in recvfrom");

    	printf("Server: %s\n", buf);

	if (strncmp(buf, "Goodbye!", 8) == 0) {
	    break;
	} 
	else if (strncmp(buf, "ls", 2) == 0) {
	    
	    n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
            if (n < 0)
              error("ERROR in recvfrom");
            
	    printf("%s\n",buf);
        }
	else if (strncmp(buf, "get", 3) == 0){

	    char f1[BUFSIZE];
	    strncpy(f1,&buf[4],BUFSIZE);

	    char filename[strlen(f1) - 1];
	    strncpy(filename, f1, strlen(f1) - 1);
	    printf("%s\n",filename);

	    FILE *fp;
	    char test[BUFSIZE];
	    fp = fopen(filename, "w");

	    while (1) {

		n = recvfrom(sockfd, test, BUFSIZE, 0, &serveraddr, &serverlen);
            	if (n < 0)
             	    error("ERROR in recvfrom");

		fputs(test, fp);
		printf("%s",test);

		if (strncmp(test, "", BUFSIZE) == 0) {
		    break;
		}
	        bzero(test, BUFSIZE);
	    }
	    fclose(fp);
	    printf("Received file: %s\n",filename);
	}
    }

    return 0;
}
