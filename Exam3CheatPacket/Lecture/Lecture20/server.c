#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT "3100"
#define BACKLOG 10
#define BUFSIZE 256

// Return the IPv4 or IPv6 address struct from a sockaddr
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET)
    return &((struct sockaddr_in *)sa)->sin_addr;
  return &((struct sockaddr_in6 *)sa)->sin6_addr;
}

int main(void) {
  int sockfd, clientfd;
  struct addrinfo hints, *res, *p;
  struct sockaddr_storage client_addr;
  socklen_t addrlen;
  char buf[BUFSIZE];
  char client_ip[INET6_ADDRSTRLEN];
  int yes = 1;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE;     // bind to local address

  if ((rv = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // Walk the results and bind to the first one we can
  for (p = res; p != NULL; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      perror("socket");
      continue;
    }

    // Avoid "address already in use" on restart
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("bind");
      continue;
    }
    break; // bound successfully
  }
  freeaddrinfo(res);

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return 1;
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    return 1;
  }

  printf("server: listening on port %s ...\n", PORT);

  // Accept loop — runs forever
  while (1) {
    addrlen = sizeof client_addr;
    clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addrlen);
    if (clientfd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(client_addr.ss_family,
              get_in_addr((struct sockaddr *)&client_addr), client_ip,
              sizeof client_ip);
    printf("server: connection from %s\n", client_ip);

    int n = recv(clientfd, buf, sizeof buf - 1, 0);
    if (n > 0) {
      buf[n] = '\0';
      printf("server: received \"%s\"\n", buf);
    } else if (n == -1) {
      perror("recv");
    }

    close(clientfd);
  }

  close(sockfd);
  return 0;
}
