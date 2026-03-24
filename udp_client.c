#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_IP   "127.0.0.1"
#define DEFAULT_PORT 9002
#define BUF_SIZE     65535

int main(int argc, char *argv[]) {
    const char *ip   = (argc > 1) ? argv[1] : DEFAULT_IP;
    int         port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in srv = {0};
    srv.sin_family = AF_INET;
    srv.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &srv.sin_addr);

    /* Đặt timeout cho recvfrom để không bị treo mãi */
    struct timeval tv = {3, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char line[BUF_SIZE], reply[BUF_SIZE];
    printf("Type messages (Ctrl+D to quit):\n");

    while (fgets(line, sizeof(line), stdin)) {
        int len = strlen(line);
        if (line[len-1] == '\n') line[--len] = '\0';
        if (len == 0) continue;

        /* Gửi datagram tới server */
        sendto(sock, line, len, 0, (struct sockaddr*)&srv, sizeof(srv));

        /* Đợi echo trả về */
        int n = recvfrom(sock, reply, BUF_SIZE-1, 0, NULL, NULL);
        if (n < 0) {
            fprintf(stderr, "Timeout or error waiting for echo\n");
        } else {
            reply[n] = '\0';
            printf("Echo: %s\n", reply);
        }
    }

    close(sock);
    return 0;
}
