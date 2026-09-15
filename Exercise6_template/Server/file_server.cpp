/* A simple server in the internet domain using TCP
The port number is passed as an argument 
Based on example: https://www.linuxhowtos.org/C_C++/socket.htm 

Modified: Michael Alrøe
Extended to support file server!
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "iknlib.h"
#include <string>
#include <fcntl.h>

#define BUFSIZE_RX 200
#define BUFSIZE_TX 256

/**
 * @brief Sends a file to a client socket
 * @param clientSocket Socket stream to client
 * @param fileName Name of file to be sent to client
 * @param fileSize Size of file
 */
void sendFile(int clientSocket, const char* fileName, long fileSize)
{
	printf("Sending: %s, size: %li\n", fileName, fileSize);

	// Åbn fil
	int fd = open(fileName, O_RDONLY);
	if (fd < 0) error("failed to open file");

	// Lav buffer og fyld med 0'er
	int bufferSize = 1000;
	uint8_t buffer[bufferSize];
	memset(buffer, 0, sizeof(buffer));

	// count holder styr på antallet af tegn der er læst
	int bytesRead = 0;

	while ((bytesRead = read(fd, buffer, bufferSize)) > 0) {
		ssize_t bytesWritten = write(clientSocket, buffer, bytesRead);
        if (bytesWritten < 0) {
            perror("failed to write to socket");
            break;
        }
	}
    
	// Husk at lukke fil igen
	close(fd);
}

void error(const char* msg) {
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	printf("Starting server...\n");

	// sockfd: file descriptor til socket
	// newsockfd: file descriptor til den nye socket der bliver oprettet når klient forbinder
	// portno: til at holde porten
	int sockfd, newsockfd, portno;

	// clilen: brugt til at holde størelsen af cli_addr (bytes)
	socklen_t clilen;
	// bufferRx: buffer til modtaget data
	// bufferTx: buffer til det data der skal sendes
	uint8_t bufferRx[BUFSIZE_RX];
	uint8_t bufferTx[BUFSIZE_TX];

	// serv_addr: struct til at holde servers adresse information
	// cli_addr: struct til at holde klients adresse information
	struct sockaddr_in serv_addr, cli_addr;

	// Hvis der ikke er tilstrækkelige argumenter, smid en fejl
	if (argc < 2) {
		error("ERROR USAGE: need to specify port");
	}

	// Lav kommunikations kanal og gem file descriptor til den i sockfd
	sockfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = IPv4, SOCK_STREAM = TCP (SOCK_DGRAM = UDP), 0 = default protocol (TCP for IPv4)
	// Tjek om socket() fejlede
	if (sockfd < 0) error("ERROR opening socket");

	printf("Binding...\n");

	// Fyld serv_addr med 0'er for at sikre der ikke er nogen garbage values
	bzero((char *) &serv_addr, sizeof(serv_addr));
	// Indstil port til brugers ønske
	portno = atoi(argv[1]);
	// Specificer at server adressen skal bruge IPv4
	serv_addr.sin_family = AF_INET;
	// Indstil server til at lytte på alle network interfaces (Wifi, ethernet, osv.)
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	// Indstil port nummeret. htons sikrer at port nummeret er oversat til standard network byte order.
	serv_addr.sin_port = htons(portno);
	// Bind socket file descriptor til ip og port fra serv_addr
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
		error("ERROR on binding");

	printf("Listen...\n");
	// Indstil socket til at lytte efter nye klient requests
	listen(sockfd,5); // 5 = der kan være 5 klienter i kø

	// Sæt clilen til længde af cli_addr
	clilen = sizeof(cli_addr);

	// Serverens loop
	for (;;)
	{
		printf("Accept...\n");
		// accept() er en blokerende funktion, koden stopper her indtil en klient forsøger at forbinde
		// når en klient forbinder, fyldes cli_addr med klientens ip og port
		// accept() returnerer en ny socket file descriptor
		// - som er dedikeret til samtalen mellem serveren og de nuværende klient
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		// fang fejl
		if (newsockfd < 0) error("ERROR on accept");
		else printf("Accepted\n");

		// Klargør RX buffer
		bzero(bufferRx,sizeof(bufferRx));

		// læs dataen sendt fra klienten og indsæt i bufferRx
		readTextTCP(newsockfd,(char*)bufferRx,sizeof(bufferRx));
		// Fjern newline symbol hvis det eksisterer
		bufferRx[strcspn((char*)bufferRx, "\r\n")] = '\0';
		
		// print inhold af RX buffer
		printf("Client requesting file: %s\n",(char*)bufferRx);
		
		// Check if file exists and store size in fileSize
		long fileSize = getFilesize((char*)bufferRx);

		if (fileSize == 0) {
			printf("Requested file doesn't exist\n");
			// snprintf indsætter tekst sikkert ind i TX buffer
			snprintf((char*)bufferTx, sizeof(bufferTx), "0");
		} else {
			printf("File was found - Size of file is %s bytes.\n", std::to_string(fileSize).c_str());
			
			// konverter filstørrelse til c-string
			std::string fileSizeStr = std::to_string(fileSize);
			char fileSizeArr[fileSizeStr.length() + 1];
			strcpy(fileSizeArr, fileSizeStr.c_str());

			// snprintf indsætter tekst sikkert ind i TX buffer
			snprintf((char*)bufferTx, sizeof(bufferTx), "%s",fileSizeArr);
		}

		// skriv indhold af TX buffer til til newsockfd
		writeTextTCP(newsockfd, (char*)bufferTx);
		
		// Husk at lukke midlertidig socket
		close(newsockfd);
	}
	// Husk at lukke socket
	close(sockfd);


	return 0; 
}