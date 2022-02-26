#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <regex.h>
#include "calcLib.h"

// comment the DEBUG macro to turn off comments in the console
#define DEBUG
#define BACKLOG 5
#define USER_LIMIT 150
#define MAXDATASIZE 1500
#define NICKNAME "NICK"
#define MSG "MSG"

using namespace std;

struct chatUser {
    char nickname[12];
    int socket;
};

char* parseMessage(char* text, char* nickname){    
    char* reply = (char*) malloc(strlen(text) + sizeof nickname + 5);
    sprintf(reply, "MSG %s %s\r", nickname, text);
    return reply;
}

// helper function that tests nicknames
bool canAcceptName(char *nickname, chatUser users[USER_LIMIT]) {
    cout << "Testing nickname: " << nickname << endl;
    if (strlen(nickname) > 12)
        return false;

    // check if there's user with same nickname
    for (int i = 0; i < USER_LIMIT; i++)
        if (strcmp(nickname, users[i].nickname) == 0)
            return false;

    char expression[] = "^[A-Za-z_]+$";
    regex_t regularexpression;
    int reti;

    reti = regcomp(&regularexpression, expression, REG_EXTENDED);
    if (reti) {
        fprintf(stderr, "Could not compile regex.\n");
        exit(1);
    }

    int matches;
    regmatch_t items;

    reti = regexec(&regularexpression, nickname, matches, &items, 0);

    if (!reti)
        return true;
    else
        return false;
}

int main(int argc, char *argv[]) {
    // disables debugging when there's no DEBUG macro defined
#ifndef DEBUG
    cout.setstate(ios_base::failbit);
    cerr.setstate(ios_base::failbit);
#endif

    /*************************************/
    /*  getting ip and port from args   */
    /***********************************/
    if (argc != 2) {
        cerr << "usage: cserverd <ip>:<port>\n"
             << "program terminated due to wrong usage" << endl;

        exit(-1);
    }

    char seperator[] = ":";
    string serverIp = strtok(argv[1], seperator);
    string destPort = strtok(NULL, seperator);
    int serverPort = atoi(destPort.c_str());

    int serverSocket = -1,
        clientSocket = -1,
        maxSocket = -1,
        interrupt = -1,
        bytes = -1,
        sendSocket = 0,
        acceptMultipleClients = 1;

    /*************************************/
    /*   setting up server metadata     */
    /***********************************/
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(serverPort);
    address.sin_addr.s_addr = inet_addr(serverIp.c_str());

    fd_set socketSet;           // stores multiple sockets
    chatUser users[USER_LIMIT]; //users who wants to chat

    char *readBuffer = (char *)malloc(MAXDATASIZE);
    char *writeBuffer = (char *)malloc(MAXDATASIZE);

    // initialise users
    for (int i = 0; i < USER_LIMIT; i++) {
        users[i].socket = 0;
        users[i].nickname[0] = 0;
    }

    /****************************/
    /*   setting up socket     */
    /**************************/
    verify(serverSocket = socket(AF_INET, SOCK_STREAM, 0));

    verify(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &acceptMultipleClients,
                   sizeof(int)));

    /* bind the socket with the server address and port */
    verify(bind(serverSocket, (struct sockaddr *)&address, sizeof(address)));

    /* listen for connection from client */
    verify(listen(serverSocket, BACKLOG));

    printf("server started listening on %s:%d\n", serverIp.c_str(), serverPort);
    int addrLen = sizeof(address);

    while (1) {
        /**********************************************/
        /*   setup to monitor all the active client  */
        /********************************************/
        FD_ZERO(&socketSet);

        FD_SET(serverSocket, &socketSet);
        maxSocket = serverSocket;

        for (int i = 0; i < USER_LIMIT; i++) {
            clientSocket = users[i].socket;

            if (clientSocket > 0)
                FD_SET(clientSocket, &socketSet);

            if (clientSocket > maxSocket)
                maxSocket = clientSocket;
        }

        /***************************************************/
        /*  listen for interrupts from server and client  */
        /*************************************************/
        interrupt = select(maxSocket + 1, &socketSet, NULL, NULL, NULL);

        if ((interrupt < 0) && (errno != EINTR))
            cerr << "error: failed to listen interrupts" << endl;

        // new client connected
        if (FD_ISSET(serverSocket, &socketSet)) {
            if ((clientSocket = accept(serverSocket,
                                       (struct sockaddr *)&address, (socklen_t *)&addrLen)) < 0) {
                cerr << "error: can't accept client\n"
                     << "program terminated while accept()" << endl;
                close(serverSocket);
                exit(-1);
            }

            printf("client connected from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            /*************************************/
            /*  connect() protocol -> HELLO 1   */
            /***********************************/
            writeBuffer = strdup("HELLO 1\n");
            verify(send(clientSocket, writeBuffer, strlen(writeBuffer), 0));

            printf("server protocol: HELLO 1\n");

            //add new socket to array of sockets
            for (int i = 0; i < USER_LIMIT; i++) {
                if (users[i].socket == 0) {
                    users[i].socket = clientSocket;
                    cout << "adding client to list at " << i + 1 << endl;
                    break;
                }
            }
        }

        for (int i = 0; i < USER_LIMIT; i++) {
            clientSocket = users[i].socket;

            if (FD_ISSET(clientSocket, &socketSet)) {
                // when user exits chat - remove him
                if ((bytes = read(clientSocket, readBuffer, MAXDATASIZE)) == 0) {
                    getpeername(clientSocket, (struct sockaddr *)&address,
                                (socklen_t *)&addrLen);

                    cout << "user disconnected , ip" << inet_ntoa(address.sin_addr)
                         << " , port " << ntohs(address.sin_port) << endl;

                    //Close the socket and mark as 0 in list for reuse
                    close(clientSocket);
                    users[i].socket = 0;
                    memset(users[i].nickname, 0, sizeof users[i].nickname);
                }

                else {
                    readBuffer[bytes] = '\0';
                    char *command, *text, *sp;
                    sp = strchr(readBuffer, ' ');
                    command = strndup(readBuffer, sp-readBuffer); /* Copy chars until space */
                    text = sp+1; /* Skip the space */

                    /**************************************/
                    /*  NICK <nick> protocol -> OK/ERR   */
                    /************************************/
                    if (strcmp(command, NICKNAME) == 0) {
                        bool accept = canAcceptName(text, users);

                        if (!accept) {
                            writeBuffer = strdup("ERR invalid_nickname\n");
                            verify(send(clientSocket, writeBuffer, strlen(writeBuffer), 0));
                            printf("Nickname %s is invalid\n", text);
                            close(clientSocket);
                            users[i].socket = 0;
                            continue;
                        }

                        strcpy(users[i].nickname, text);
                        writeBuffer = strdup("OK\n");
                        send(clientSocket, writeBuffer, strlen(writeBuffer), 0);
                        printf("Nickname %s is accepted\n", text);
                    }

                    /**************************************/
                    /*  MSG <text> protocol ->           */
                    /*    MSG nick <text>/ERROR <text>  */
                    /***********************************/
                    else if (strcmp(command, MSG) == 0)
                    {
                        if (users[i].nickname[0] == 0) {
                            writeBuffer = strdup("ERR no_nickname_set\n");
                            send(clientSocket, writeBuffer, strlen(writeBuffer), 0);
                            cerr << "no_nickname_set" << endl;
                            continue;
                        }

                        if(strlen(text) > 255){
                            writeBuffer = strdup("ERR msg_overflow\n");
                            send(clientSocket, writeBuffer, strlen(writeBuffer), 0);
                            continue;
                        }

                        writeBuffer = parseMessage(text, users[i].nickname);

                        for (int j = 0; j < USER_LIMIT; j++) {
                            if (users[j].socket == 0)
                                continue;

                            sendSocket = users[j].socket;
                            cout << "broadcasting: " << writeBuffer;
                            send(sendSocket, writeBuffer, strlen(writeBuffer), 0);
                        }
                    }

                    // wrong command
                    else {
                        writeBuffer = strdup("ERR invalid command\n");
                        send(clientSocket, writeBuffer, strlen(writeBuffer), 0);
                        printf("wrong command\n");
                    }
                }
            }
        }
    }

    free(writeBuffer);
    free(readBuffer);
    close(serverSocket);

    return 0;
}