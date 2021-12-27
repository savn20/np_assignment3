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

// comment the DEBUG macro to turn off comments in the console
#define DEBUG
#define BACKLOG 5
#define USER_LIMIT 50

using namespace std;

struct chatUser
{
    char nickname[20];
    int socket;
};

int main(int argc, char *argv[])
{
    // disables debugging when there's no DEBUG macro defined
#ifndef DEBUG
    cout.setstate(ios_base::failbit);
    cerr.setstate(ios_base::failbit);
#endif

    /*************************************/
    /*  getting ip and port from args   */
    /***********************************/
    if (argc != 2)
    {
        cerr << "usage: server <ip>:<port>\n"
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

    char nickname[15];
    char message[256];
    string connectProtocol = "HELLO 1\n";

    // initialise users
    for (int i = 0; i < USER_LIMIT; i++)
        users[i].socket = 0;

    /*************************************/
    /*   creating socket connection     */
    /***********************************/
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr << "server: failed to create socket\n"
             << "program terminated while socket()" << endl;

        exit(-1);
    }

    /*************************************/
    /*     accept multiple clients      */
    /***********************************/
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &acceptMultipleClients,
                   sizeof(int)) < 0)
    {
        cerr << "server: failed to accept multiple connections\n"
             << "program terminated while setsockopt()" << endl;
        close(serverSocket);
        exit(-1);
    }

    /*************************************/
    /*  binding server to ip and port   */
    /***********************************/
    if (bind(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        close(serverSocket);
        cerr << "error: failed to bind socket\n"
             << "program terminated while bind()" << endl;

        exit(-1);
    }

    /*************************************/
    /*  listening for incoming clients  */
    /***********************************/
    if (listen(serverSocket, BACKLOG) < 0)
    {
        cerr << "error: failed to listen to socket\n"
             << "program terminated while listen()" << endl;
        exit(-1);
    }

    printf("server started listening on %s:%d\n", serverIp.c_str(), serverPort);
    int addrLen = sizeof(address);

    while (1)
    {

        /**********************************************/
        /*   setup to monitor all the active client  */
        /********************************************/
        FD_ZERO(&socketSet);

        FD_SET(serverSocket, &socketSet);
        maxSocket = serverSocket;

        for (int i = 0; i < USER_LIMIT; i++)
        {
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
        if (FD_ISSET(serverSocket, &socketSet))
        {
            if ((clientSocket = accept(serverSocket,
					(struct sockaddr *)&address, (socklen_t*)&addrLen))<0)
            {
                cerr << "error: can't accept client\n"
                     << "program terminated while accept()" << endl;
                close(serverSocket);
                exit(-1);
            }

            //inform user of socket number - used in send and receive commands
            printf("client connected from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            /*************************************/
            /*  connect() protocol -> HELLO 1   */
            /***********************************/
            if (send(clientSocket, connectProtocol.c_str(), strlen(connectProtocol.c_str()), 0) < 0)
            {
                cerr << "error: failed to send message to client\n"
                     << "program terminated while send()" << endl;
                close(clientSocket);
                exit(-1);
            }

            printf("server protocol: HELLO 1\n");

            //add new socket to array of sockets
            for (int i = 0; i < USER_LIMIT; i++)
            {

                if (users[i].socket == 0)
                {
                    users[i].socket = clientSocket;
                    cout << "Adding client to list at " << i + 1 << endl;
                    break;
                }
            }
        }

        //TODO: read, check and accept nickname
    }

    return 0;
}