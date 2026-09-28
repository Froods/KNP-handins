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
	printf("Starting UDP server...\n");
	int sock, n;
	socklen_t fromlen;
	struct sockaddr_in server;
	struct sockaddr_in from;
	char buf[256];

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) error("ERROR, socket");

	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(9000);

	printf("Binding...\n");
	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
		error("ERROR, binding");

	fromlen = sizeof(from);

	while (1)
	{
		printf("Receive...\n");
		n = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from, &fromlen);

		if (n < 0)
			error("ERROR, recvfrom");
		buf[n] = 0; // handle null termination
		printf("Received a command: %c\n", buf[0]);

		// Initialiserer et tomt array hvor svaret til klienten kan overføres til
		char reply[512];

		// Returnerer information til klient eller relevant fejlbesked
		if (buf[0] == 'u' || buf[0] == 'U') // load proc/uptime til reply
		{
			FILE *f = fopen("/proc/uptime", "r"); // Fil åbnes (read)
			if (f == NULL)
			{
				strcpy(reply, "ERROR: could not open /proc/uptime\n"); // String copy kommando
			}
			else
			{
				int len = fread(reply, 1, sizeof(reply) - 1, f); // Kopierer bytes til reply
				reply[len] = '\0';								 // Tilføjer til slut af array for at undgå garbage værdier i beskeden
				fclose(f);										 // Lukker filen
			}
		}
		else if (buf[0] == 'l' || buf[0] == 'L') // load proc/loadavg til reply
		{
			FILE *f = fopen("/proc/loadavg", "r"); // Samme som før med ny adresse
			if (f == NULL)
			{
				strcpy(reply, "ERROR: could not open /proc/loadavg\n");
			}
			else
			{
				int len = fread(reply, 1, sizeof(reply) - 1, f);
				reply[len] = '\0';
				fclose(f);
			}
		}
		else // load error message til reply (ved alt andet end: "u", "U", "l", "L")
		{
			strcpy(reply, "ERROR: unknown command\n");
		}

		// Send returbesked til klient
		n = sendto(sock, reply, strlen(reply), 0, (struct sockaddr *)&from, fromlen);
		if (n < 0) error("ERROR, sendto");
	}

	return 0;
}
