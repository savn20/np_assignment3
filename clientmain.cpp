#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>

#include "calcLib.h"

// comment the DEBUG macro to turn off comments in the console
#define DEBUG
#define BUFFER_SIZE 500
#define CONNECT "HELLO 1"
#define OK "OK"

using namespace std;

void sendMessage(int socketConnection, char *message, int n);
int getResponse(int socketConnection, char *message, int n);

int main(int argc, char *argv[]) {
    // disables debugging when there's no DEBUG macro defined
#ifndef DEBUG
    cout.setstate(ios_base::failbit);
    cerr.setstate(ios_base::failbit);
#endif

    /*************************************/
    /*  getting ip and port from args   */
    /***********************************/
    if (argc != 3)
    {
        cerr << "usage: server <ip>:<port> <nickname>\n"
             << "program terminated due to wrong usage" << endl;

        exit(-1);
    }

    char delim[] = ":";
    char *serverIp = strtok(argv[1], delim);
    char *serverPort = strtok(NULL, delim);
    char *nickname = argv[2];

    int sockFd, rv, byteSize;

    /*************************************/
    /*   setting up server metadata     */
    /***********************************/
	addrinfo hints, *servinfo, *ptr;
    memset(&hints, 0, sizeof hints);

    hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

    char response[BUFFER_SIZE];
    char message[BUFFER_SIZE];
	char s[INET6_ADDRSTRLEN];
    char text[256];

    verify((rv = getaddrinfo(serverIp, serverPort, &hints, &servinfo)) != 0);

	for(ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
        // creating socket
		if((sockFd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) {
			cerr << "talker: socket\n";
			continue;
		}

		break;
	}

    if (ptr == NULL) {
		cerr << "talker: failed to create socket\n";
        exit(-1);
	}

    // connecting to established socket
    verify(connect(sockFd,ptr->ai_addr, ptr->ai_addrlen));

    inet_ntop(ptr->ai_family, getSocketAddress((struct sockaddr *)ptr->ai_addr),
            s, sizeof s);
        
    printf("Host %s and port %s\n", s, serverPort);
	freeaddrinfo(servinfo);
    
    /*************************************/
    /*  connect() -> HELLO 1 (response) */
    /***********************************/
    getResponse(sockFd, response, sizeof(response));

    printf("server protocol: %s\n", strtok(response, "\n"));

    if (strcmp(strtok(response, "\n"), CONNECT) != 0)
    {
        cerr << "error: client didn't server protocol "
             << strtok(response, "\n") << endl;
        close(sockFd);
        exit(-1);
    }

    cout << "protocol supported sending nickname:" << nickname << endl;

    /**************************************/
    /*  NICK <nick> protocol -> OK/ERR   */
    /************************************/
    sprintf(message, "NICK %s", nickname);
    sendMessage(sockFd, message, strlen(message));

    getResponse(sockFd, response, sizeof(response));

    fd_set socketSet;

    while (1)
    {
        FD_ZERO(&socketSet);
        FD_SET(0, &socketSet);
        FD_SET(sockFd, &socketSet);

        verify(select(sockFd + 1, &socketSet, NULL, NULL, NULL));

        // socket response
        if (FD_ISSET(sockFd, &socketSet)) {
            byteSize = getResponse(sockFd, response, sizeof(response));
            response[byteSize] = '\0';
            printf("%s\n", strtok(response, "\n"));
            memset(response, 0, BUFFER_SIZE);
        }

        /**************************************/
        /*  MSG <text> protocol ->           */
        /*    MSG nick <text>/ERROR <text>  */
        /***********************************/
        if (FD_ISSET(0, &socketSet)) {
            fgets(message, 255, stdin);
            string inp = strdup(message);
            inp.insert(0, "MSG ");
            strcpy(message, inp.c_str());
            sendMessage(sockFd, message, strlen(message));
        }
    }
}

void sendMessage(int socketConnection, char *message, int n)
{
    if (send(socketConnection, message, n, 0) < 0)
    {
        cerr << "error: failed to send message\n"
             << "program terminated while send()" << endl;
        close(socketConnection);
        exit(-1);
    }
}

int getResponse(int socketConnection, char *response, int n)
{
    int r = recv(socketConnection, response, n, 0);

    if (r <= 0)
    {
        cerr << "error: no byteSize rec from server\n"
             << "program terminated while recv()" << endl;
        close(socketConnection);
        exit(-1);
    }

    return r;
}