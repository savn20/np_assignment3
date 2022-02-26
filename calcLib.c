#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "calcLib.h"

void verify(int hasError) {
  if (hasError == SOCKET_FAILURE) {
    perror("error: something went wrong dealing with sockets\n");
    exit(-1);
  }
}

void *getSocketAddress(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET)
    return &(((struct sockaddr_in *)sa)->sin_addr);

  if (sa->sa_family == AF_INET6)
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);

  perror("Unknown FAMILY!!!!\n");
  return (0);
}

char *getIpAddress(const struct sockaddr *sa, char *s, size_t maxlen) {
  switch (sa->sa_family)
  {
  case AF_INET:
    inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
              s, maxlen);
    break;

  case AF_INET6:
    inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
              s, maxlen);
    break;

  default:
    strncpy(s, "Unknown AF", maxlen);
    return NULL;
  }

  return s;
}

char* parseMessage(char* text, char* nickname){    
    char* reply = (char*) malloc(strlen(text) + sizeof nickname + 5);
    sprintf(reply, "MSG %s %s\r", nickname, text);
    return reply;
}

void parseServerResponse(char* command, char* text, char* userNickname){
  if (strcmp(command, "ERROR") == 0){
    printf("%s %s\n", command, text);
  }

  if(strcmp(command, "MSG") == 0){
    char *nickname, *message, *sp;
    sp = strchr(text, ' ');
    nickname = strndup(text, sp-text);
    message = sp+1;

    if(strcmp(nickname, userNickname) != 0){
      printf("%s: %s", nickname, message);   
    }  
  }
}