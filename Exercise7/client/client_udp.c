#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char *argv[])
{
	printf("Starting UDP client...\n");
	int sock, n;
	socklen_t serverlength;					//Variabler initialiseres
	struct sockaddr_in server;
	struct hostent *hp;
	char buf[256];
	
	if (argc != 3) error("USAGE: ./get_measurement <ip-server> <cmd>\n");	//Vi skal modtage 3 argumenter. Programnavn, server, cmd
													
	sock = socket(AF_INET, SOCK_DGRAM, 0);  //Byg socket
	if (sock < 0) error("ERROR, socket");

	server.sin_family = AF_INET;			//Byg server
	hp = gethostbyname(argv[1]);
	if (hp==0) error("ERROR, Unknown host");

	bcopy((char *)hp->h_addr_list[0],(char *)&server.sin_addr,hp->h_length);
	server.sin_port = htons(9000); //Hardcoded port
	serverlength=sizeof(server);

	n=sendto(sock,argv[2],strlen(argv[2]),0,(const struct sockaddr *)&server,serverlength); //Send cmd (argv[2] with the message)
	if (n < 0) error("ERROR, Sendto");
	n = recvfrom(sock,buf,sizeof(buf)-1,0,(struct sockaddr *)&server, &serverlength);
	if (n < 0) error("ERROR, recvfrom");
	buf[n]=0;  //handle null termination
	printf("Got a datagram: %s\n", buf);
	
	close(sock);
	return 0;
}

