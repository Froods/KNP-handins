/* A simple client in the internet domain using TCP
The ip adresse and port number on server is passed as arguments 
Based on example: https://www.linuxhowtos.org/C_C++/socket.htm 

Modified: Michael Alrøe
Extended to support file client!
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h> 
#include "iknlib.h"
#include <fcntl.h>

#define BUFFSIZE 256

/**
 * @brief Receives a file from a server socket
 * @param serverSocket Socket stream to server
 * @param fileName Name of file. Might include path on server!
 */

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void receiveFile(int serverSocket, const char* fileName, long fileSize)
{
	const char* localFileName = extractFileName(fileName);
	printf("Receiving: '%s', size: %li\n", localFileName, fileSize);

	// Åbn fil
	// =O_TRUNC gør at filen tømmes hvis den allerede eksisterer
	// 0666 giver alle tilladelse til at læse filen
	int fd = open(localFileName, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) error("open() failed");

	// Lav buffer og fyld med 0'er
	int bufferSize = 1000;
	uint8_t buffer[bufferSize];
	memset(buffer, 0, sizeof(buffer));

	// bytesRead holder styr på antallet af tegn der er læst hver gang
	int bytesRead = 0;
	// totalBytesRead holder styr på det samlede antal af bytes der er læst
	int totalBytesRead = 0;

	while (totalBytesRead < fileSize) {
		bytesRead = read(serverSocket, buffer, bufferSize);
		totalBytesRead += bytesRead;
		printf("Fetched %i/%li bytes\n", totalBytesRead, fileSize);
		ssize_t bytesWritten = write(fd, buffer, bytesRead);
        if (bytesWritten < 0) error("failed to write to socket");
	}

	close(fd);

}

int main(int argc, char *argv[])
{
	printf("Starting client...\n");

	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	uint8_t buffer[BUFFSIZE];
    
	if (argc < 3)
	    error( "ERROR usage: ""hostname"",  ""filename""");

	portno = 9000;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
	    error("ERROR opening socket");

	server = gethostbyname(argv[1]);
	if (server == NULL) 
	    error("ERROR no such host");

	printf("Server at: %s, port: %s\n",argv[1], argv[2]);

	printf("Connect...\n");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
	    error("ERROR connecting");

	char* fileName = argv[2];
	fileName[strcspn(fileName, "\r\n")] = '\0';
	writeTextTCP(sockfd, fileName);  // socket write
	
	long fileSize = readFileSizeTCP(sockfd);

	if (fileSize == 0) {
		printf("Requested file is nonexistent.\n");
		printf("Closing client...\n\n");
		close(sockfd);
		return 0;
	} else {
		printf("\nSize of file: %ld bytes\n\n",fileSize);
	}

	receiveFile(sockfd, fileName, fileSize);

    printf("Closing client...\n\n");
	close(sockfd);
	return 0;
}

