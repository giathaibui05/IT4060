#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Cach dung: %s <dia chi IP> <cong>\n", argv[0]);
        fprintf(stderr, "Vi du:     %s 127.0.0.1 8080\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip   = argv[1];
    int         port = atoi(argv[2]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Dia chi IP khong hop le: %s\n", ip);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("[tcp_client] Da ket noi den %s:%d\n", ip, port);
    char recv_buf[BUFFER_SIZE];
    int  n;
    struct timeval tv = {1, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    n = recv(sockfd, recv_buf, BUFFER_SIZE - 1, 0);
    if (n > 0) {
        recv_buf[n] = '\0';
        printf("[Server]: %s", recv_buf);
    }
    tv.tv_sec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    char send_buf[BUFFER_SIZE];
    printf("Nhap du lieu (Ctrl+D de thoat):\n");
    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(send_buf, BUFFER_SIZE, stdin) == NULL) {
            printf("\n[tcp_client] Dong ket noi.\n");
            break;
        }

        int len = (int)strlen(send_buf);
        if (send(sockfd, send_buf, len, 0) < 0) {
            perror("send()");
            break;
        }
    }

    close(sockfd);
    return 0;
}