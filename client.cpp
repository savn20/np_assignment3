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

// comment the DEBUG macro to turn off comments in the console
#define DEBUG
#define BUFFER_SIZE 500
#define CONNECT "HELLO 1"
#define OK "OK"

using namespace std;

void sendMessage(int socketConnection, char *message, int n);
int getResponse(int socketConnection, char *message, int n);

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
    if (argc != 3)
    {
        cerr << "usage: server <ip>:<port> <nickname>\n"
             << "program terminated due to wrong usage" << endl;

        exit(-1);
    }

    char seperator[] = ":";
    char *serverIp = strtok(argv[1], seperator);
    char *destPort = strtok(NULL, seperator);
    int serverPort = atoi(destPort);

    char *nickname = argv[2];

    int serverSocket = -1,
        bytes = -1;

    /*************************************/
    /*   setting up server metadata     */
    /***********************************/
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(serverPort);
    address.sin_addr.s_addr = inet_addr(serverIp);

    char response[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    char text[256];

    cout << "establishing connection..." << endl;

    /*************************************/
    /*   creating socket connection     */
    /***********************************/
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr << "client: failed to create socket\n"
             << "program terminated while socket()" << endl;

        exit(-1);
    }

    /******************************************/
    /*  establishing connection with server  */
    /****************************************/
    if (connect(serverSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        close(serverSocket);
        cerr << "error: failed to connect to socket\n"
             << "program terminated while connect()" << endl;

        exit(-1);
    }

    printf("connected to server %s:%d\n", serverIp, serverPort);

    /*************************************/
    /*  connect() -> HELLO 1 (response) */
    /***********************************/
    getResponse(serverSocket, response, sizeof(response));

    printf("server protocol: %s\n", strtok(response, "\n"));

    if (strcmp(strtok(response, "\n"), CONNECT) != 0)
    {
        cerr << "error: client didn't server protocol "
             << strtok(response, "\n") << endl;
        close(serverSocket);
        exit(-1);
    }

    cout << "protocol supported sending nickname:" << nickname << endl;

    /**************************************/
    /*  NICK <nick> protocol -> OK/ERR   */
    /************************************/
    sprintf(message, "NICK %s", nickname);
    sendMessage(serverSocket, message, strlen(message));

    getResponse(serverSocket, response, sizeof(response));

    if (strcmp(strtok(response, "\n"), OK) != 0)
    {
        perror("error: something wrong with your nickname\n");
        close(serverSocket);
        exit(-1);
    }

    fd_set socketSet;

    while (1)
    {
        FD_ZERO(&socketSet);
        FD_SET(0, &socketSet);
        FD_SET(serverSocket, &socketSet);

        if (select(serverSocket + 1, &socketSet, NULL, NULL, NULL) == -1)
        {
            perror("select:");
            exit(1);
        }

        // socket response
        if (FD_ISSET(serverSocket, &socketSet))
        {
            bytes = getResponse(serverSocket, response, sizeof(response));
            response[bytes] = '\0';
            printf("%s\n", strtok(response, "\n"));
            memset(response, 0, BUFFER_SIZE);
        }

        /**************************************/
        /*  MSG <text> protocol ->           */
        /*    MSG nick <text>/ERROR <text>  */
        /***********************************/
        if (FD_ISSET(0, &socketSet))
        {
            fgets(message, 255, stdin);
            string inp = strdup(message);
            inp.insert(0, "MSG ");
            strcpy(message, inp.c_str());
            sendMessage(serverSocket, message, strlen(message));
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
        cerr << "error: no bytes rec from server\n"
             << "program terminated while recv()" << endl;
        close(socketConnection);
        exit(-1);
    }

    return r;
}