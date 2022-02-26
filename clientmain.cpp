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

int main(int argc, char *argv[]) {
    // disables debugging when there's no DEBUG macro defined
#ifndef DEBUG
    cout.setstate(ios_base::failbit);
    cerr.setstate(ios_base::failbit);
#endif

    /*************************************/
    /*  getting ip and port from args   */
    /***********************************/
    if (argc != 3) {
        cerr << "usage: cchat <ip>:<port> <nickname>\n"
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

    char *readBuffer = (char *)malloc(MAXDATASIZE);
    char *writeBuffer = (char *)malloc(MAXDATASIZE);
	char s[INET6_ADDRSTRLEN];

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
    verify(byteSize = recv(sockFd, readBuffer, MAXDATASIZE-1, 0));
    readBuffer[byteSize] = '\0';

    printf("server protocol: %s\n", strtok(readBuffer, "\n"));

    if (strcmp(strtok(readBuffer, "\n"), CONNECT) != 0) {
        cerr << "error: client didn't support the server protocol "
             << strtok(readBuffer, "\n") << endl;
        close(sockFd);
        exit(-1);
    }

    cout << "server protocol supported, now sending nickname:" << nickname << endl;

    /**************************************/
    /*  NICK <nick> protocol -> OK/ERR   */
    /************************************/
    sprintf(writeBuffer, "NICK %s", nickname);
    verify(send(sockFd, writeBuffer, strlen(writeBuffer), 0));

    verify(byteSize = recv(sockFd, readBuffer, MAXDATASIZE-1, 0));
    readBuffer[byteSize] = '\0';

    if (strcmp(strtok(readBuffer, "\n"), OK) != 0) {
        cerr << "error: nickname not supported "
             << strtok(readBuffer, "\n") << endl;
        close(sockFd);
        exit(-1);
    }

    printf("nickname %s accepted\n", nickname);

    fd_set socketSet;

    while (1) {
        FD_ZERO(&socketSet);
        FD_SET(0, &socketSet);
        FD_SET(sockFd, &socketSet);

        verify(select(sockFd + 1, &socketSet, NULL, NULL, NULL));

        // socket response
        if (FD_ISSET(sockFd, &socketSet)) {
            verify(byteSize = recv(sockFd, readBuffer, MAXDATASIZE-1, 0));
            readBuffer[byteSize] = '\0';
            
            char *command, *text, *sp;
            sp = strchr(readBuffer, ' ');
            command = strndup(readBuffer, sp-readBuffer); /* Copy chars until space */
            text = sp+1; /* Skip the space */

            /// TODO: parse the MSG nickname <text> to nickname:<text>
            parseServerResponse(command, text, nickname);
        }

        /**************************************/
        /*  MSG <text> protocol ->           */
        /*    MSG nick <text>/ERROR <text>  */
        /***********************************/
        if (FD_ISSET(0, &socketSet)) {
            fgets(writeBuffer, 255, stdin);
            string inp = strdup(writeBuffer);
            inp.insert(0, "MSG ");
            strcpy(writeBuffer, inp.c_str());
            verify(send(sockFd, writeBuffer, strlen(writeBuffer), 0));
        }
    }

    close(sockFd);
    free(writeBuffer);
    free(readBuffer);
    return 0; 
}