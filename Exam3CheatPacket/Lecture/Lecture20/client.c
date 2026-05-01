#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT "3100"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <hostname> <word>\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];
    const char *word = argv[2];

    struct addrinfo hints, *res, *p;
    int sockfd, rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;   // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    if ((rv = getaddrinfo(host, PORT, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Walk the results and connect to the first one we can
    for (p = res; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) { perror("socket"); continue; }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("connect");
            continue;
        }
        break; // connected
    }
    freeaddrinfo(res);

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 1;
    }

    printf("client: connected, sending \"%s\"\n", word);

    if (send(sockfd, word, strlen(word), 0) == -1)
        perror("send");

    close(sockfd);
    printf("client: done\n");
    return 0;
}
