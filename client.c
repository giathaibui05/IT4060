#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT      9001

/* 4 chunk mẫu từ đề bài (không có ký tự xuống dòng) */
static const char *SAMPLE[] = {
    "S0ICTS0ICT012345678901234567890123456789012345",
    "6789S0ICTS0ICTS0ICT012345678901234567890123456",
    "7890123456789012345678901234567890123456789012",
    "3456789S0ICTS0ICT01234567890123456789012345678",
    NULL
};

static int send_all(int fd, const void *buf, int n) {
    int done = 0;
    while (done < n) {
        int r = send(fd, (const char*)buf + done, n - done, 0);
        if (r <= 0) return -1;
        done += r;
    }
    return done;
}

int main(int argc, char *argv[]) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(sock); return 1;
    }

    (void)argv; /* dùng khi argc>1: chế độ tương tác */
    if (argc > 1) {
        /* Chế độ tương tác: đọc từng dòng từ stdin và gửi */
        char line[4096];
        printf("Type text (Ctrl+D to quit):\n");
        while (fgets(line, sizeof(line), stdin)) {
            int len = strlen(line);
            if (line[len-1] == '\n') line[--len] = '\0';
            if (send_all(sock, line, len) < 0) break;
        }
    } else {
        /* Gửi 4 chunk dữ liệu mẫu từ đề bài */
        for (int i = 0; SAMPLE[i] != NULL; i++) {
            int len = strlen(SAMPLE[i]);
            printf("Sending chunk %d (%d bytes): %.30s...\n", i+1, len, SAMPLE[i]);
            if (send_all(sock, SAMPLE[i], len) < 0) {
                fprintf(stderr, "Send failed at chunk %d\n", i+1);
                break;
            }
            usleep(100000); /* 100ms giữa các chunk để dễ quan sát */
        }
    }

    close(sock);
    printf("Client done.\n");
    return 0;
}
