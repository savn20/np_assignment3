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