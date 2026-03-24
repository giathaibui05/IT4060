#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 9002
#define BUF_SIZE     65535

int main(int argc, char *argv[]) {
    int port = (argc > 1) ? atoi(argv[1]) : DEFAULT_PORT;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(sock); return 1;
    }
    printf("udp_echo listening on UDP port %d ...\n", port);

    char buf[BUF_SIZE];
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    while (1) {
        /* Nhận datagram, lưu địa chỉ người gửi */
        int n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
                         (struct sockaddr*)&cli_addr, &cli_len);
        if (n < 0) { perror("recvfrom"); continue; }

        buf[n] = '\0';
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, ip, sizeof(ip));
        printf("Recv %d bytes from %s:%d : %s\n",
               n, ip, ntohs(cli_addr.sin_port), buf);

        /* Echo: gửi nguyên xi về đúng địa chỉ nguồn */
        if (sendto(sock, buf, n, 0,
                   (struct sockaddr*)&cli_addr, cli_len) < 0)
            perror("sendto");
    }

    close(sock);
    return 0;
}
