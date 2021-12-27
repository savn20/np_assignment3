#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

// comment the DEBUG macro to turn off comments in the console
#define DEBUG
#define BACKLOG 5

using namespace std;

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
        acceptMultipleClients = 1;

    /*************************************/
    /*   setting up server metadata     */
    /***********************************/
    sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    serverAddress.sin_addr.s_addr = inet_addr(serverIp.c_str());

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
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
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

    return 0;
}