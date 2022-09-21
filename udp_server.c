/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

// Kasper Seglem

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h> 

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  /* 
   * check command line arguments 
   */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* 
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");

    /* 
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
	   hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

    // exit
    if (strncmp(buf, "exit", 4) == 0) {
        
	char e1[BUFSIZE];
	bzero(e1, BUFSIZE);
	strcpy(e1, "Goodbye!");
	n = sendto(sockfd, e1, BUFSIZE, 0,
               (struct sockaddr *) &clientaddr, clientlen);
	if (n < 0)
          error("ERROR in sendto");
    }
    // ls
    else if (strncmp(buf, "ls", 2) == 0) {

        n = sendto(sockfd, buf, BUFSIZE, 0,
               (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");

    
	char l1[BUFSIZE];
	bzero(l1, BUFSIZE);
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	if (d) {
	    while ((dir = readdir(d)) != NULL) {
      		strcat(l1,dir->d_name);
		strcat(l1,"\n");
    	    }
    	    closedir(d);
        }

	n = sendto(sockfd, l1, BUFSIZE, 0, 
            (struct sockaddr *) &clientaddr, clientlen);
	if (n < 0)
          error("ERROR in sendto");

    }
    // get
    else if (strncmp(buf, "get", 3) == 0) {

	char f1[BUFSIZE];
	strncpy(f1,&buf[4],BUFSIZE);
	printf("Sending file: %s",f1);

	n = sendto(sockfd, buf, BUFSIZE, 0,
               (struct sockaddr *) &clientaddr, clientlen);
        if (n < 0)
          error("ERROR in sendto");
	bzero(buf, BUFSIZE);

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

	        n = sendto(sockfd, test, BUFSIZE, 0, &clientaddr, clientlen);
                if (n < 0)
                    error("ERROR in sendto");
          
	        bzero(test, BUFSIZE);
		usleep(1);
	    }
	    n = sendto(sockfd, "DONEDONE", BUFSIZE, 0, &clientaddr, clientlen);
            if (n < 0)
                error("ERROR in sendto");
	    
	    printf("File sent\n\n");
        }
	else {
	    printf("Bad\n");
            strcpy(test, "File not found");
            n = sendto(sockfd, test, BUFSIZE, 0,
                (struct sockaddr *) &clientaddr, clientlen);
            if (n < 0)
                error("ERROR in sendto");
        }
    }
    // put
    else if (strncmp(buf, "put", 3) == 0){

            char f1[BUFSIZE];
            strncpy(f1,&buf[4],BUFSIZE);

	    n = sendto(sockfd, buf, BUFSIZE, 0,
               (struct sockaddr *) &clientaddr, clientlen);
            if (n < 0)
               error("ERROR in sendto");
	    bzero(buf, BUFSIZE);

            char filename[strlen(f1) - 1];
            strncpy(filename, f1, strlen(f1) - 1);

            FILE *fp;
            char test[BUFSIZE];
            fp = fopen(filename, "wb");

            while (1) {
		bzero(test, BUFSIZE);

                n = recvfrom(sockfd, test, BUFSIZE, 0,
                 (struct sockaddr *) &clientaddr, &clientlen);
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
    // delete
    else if (strncmp(buf, "delete", 6) == 0) {

	n = sendto(sockfd, buf, strlen(buf), 0, &clientaddr, clientlen);
        if (n < 0)
            error("ERROR in sendto");

	char f1[BUFSIZE];
	strncpy(f1,&buf[7],BUFSIZE);

	char filename[strlen(f1) - 1];
	strncpy(filename, f1, strlen(f1) - 1);

	// remove(filename);

    }
    else {
    
	/* 
	 * sendto: echo the input back to the client 
	 */
	char unable[BUFSIZE];
	strcpy(unable, "command not understood\n");
	n = sendto(sockfd, unable, BUFSIZE, 0, 
            (struct sockaddr *) &clientaddr, clientlen);
	if (n < 0) 
	  error("ERROR in sendto");
    }
  }
}

