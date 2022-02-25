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
#define BACKLOG 5
#define USER_LIMIT 50
#define BUFFER_SIZE 500
#define NICKNAME "NICK"
#define MSG "MSG"

using namespace std;

struct chatUser
{
    char nickname[20];
    int socket;
};

bool canAcceptName(char *nickname, chatUser users[USER_LIMIT]);

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
        bytes = -1,
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

    char nickname[20];
    char text[256];
    char buffer[BUFFER_SIZE];
    char reply[BUFFER_SIZE];
    string connectProtocol = "HELLO 1\n";
    string response("");

    // initialise users
    for (int i = 0; i < USER_LIMIT; i++)
    {
        users[i].socket = 0;
        users[i].nickname[0] = 0;
    }

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
                                       (struct sockaddr *)&address, (socklen_t *)&addrLen)) < 0)
            {
                cerr << "error: can't accept client\n"
                     << "program terminated while accept()" << endl;
                close(serverSocket);
                exit(-1);
            }

            printf("client connected from %s:%d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            /*************************************/
            /*  connect() protocol -> HELLO 1   */
            /***********************************/
            if (send(clientSocket, connectProtocol.c_str(), strlen(connectProtocol.c_str()), 0) < 0)
            {
                cerr << "error: failed to send text to client\n"
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

        for (int i = 0; i < USER_LIMIT; i++)
        {
            clientSocket = users[i].socket;

            if (FD_ISSET(clientSocket, &socketSet))
            {

                if ((bytes = read(clientSocket, buffer, sizeof buffer)) == 0)
                {
                    getpeername(clientSocket, (struct sockaddr *)&address,
                                (socklen_t *)&addrLen);

                    cout << "user disconnected , ip" << inet_ntoa(address.sin_addr)
                         << " , port " << ntohs(address.sin_port) << endl;

                    //Close the socket and mark as 0 in list for reuse
                    close(clientSocket);
                    users[i].socket = 0;
                    memset(users[i].nickname, 0, sizeof users[i].nickname);
                }

                else
                {
                    buffer[bytes] = '\0';
                    char command[10];
                    sscanf(buffer, "%s %s", command, text);

                    /**************************************/
                    /*  NICK <nick> protocol -> OK/ERR   */
                    /************************************/

                    if (strcmp(command, NICKNAME) == 0)
                    {
                        bool accept = canAcceptName(text, users);

                        if (!accept)
                        {
                            response = "ERR invalid nickname\n";
                            send(clientSocket, response.c_str(), strlen(response.c_str()), 0);
                            printf("Nickname %s is invalid\n", text);
                            close(clientSocket);
                            users[i].socket = 0;
                            continue;
                        }

                        strcpy(users[i].nickname, text);
                        response = "OK\n";
                        send(clientSocket, response.c_str(), strlen(response.c_str()), 0);
                        printf("Nickname %s is accepted\n", text);
                    }

                    /**************************************/
                    /*  MSG <text> protocol ->           */
                    /*    MSG nick <text>/ERROR <text>  */
                    /***********************************/

                    else if (strcmp(command, MSG) == 0)
                    {
                        if (users[i].nickname[0] == 0)
                        {
                            response = "ERR nickname not set\n";
                            send(clientSocket, response.c_str(), strlen(response.c_str()), 0);
                            cerr << "nickname not set" << endl;
                            continue;
                        }

                        int sendSocket = 0;
                        response = buffer;
                        strcpy(buffer, response.substr(3).c_str());
                        sprintf(reply, "%s:%s\r", users[i].nickname, buffer);

                        for (int j = 0; j < USER_LIMIT; j++)
                        {
                            if (users[j].socket == 0)
                                continue;

                            sendSocket = users[j].socket;
                            if (clientSocket == sendSocket)
                                continue;

                            cout << "broadcasting: " << reply;

                            send(sendSocket, reply, strlen(reply), 0);
                        }
                    }

                    // wrong command
                    else
                    {
                        response = "ERR invalid command\n";
                        send(clientSocket, response.c_str(), strlen(response.c_str()), 0);
                        printf("wrong command\n");
                    }
                }
            }
        }
    }

    return 0;
}

// helper function that tests nicknames
bool canAcceptName(char *nickname, chatUser users[USER_LIMIT])
{
    cout << "Testing nickname: " << nickname << endl;
    if (strlen(nickname) > 12)
        return false;

    // check if there's user with same nickname
    for (int i = 0; i < USER_LIMIT; i++)
        if (strcmp(nickname, users[i].nickname) == 0)
            return false;

    string expression = "^[A-Za-z_]+$";
    regex_t regularexpression;
    int reti;

    reti = regcomp(&regularexpression, expression.c_str(), REG_EXTENDED);
    if (reti)
    {
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