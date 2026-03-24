#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define BACKLOG     5

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Cach dung: %s <cong> <tep cau chao> <tep luu noi dung>\n", argv[0]);
        fprintf(stderr, "Vi du:     %s 8080 greeting.txt received.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int         port          = atoi(argv[1]);
    const char *greeting_file = argv[2];
    const char *log_file      = argv[3];
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen()");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[tcp_server] Dang lang nghe tren cong %d ...\n", port);
    printf("  File cau chao : %s\n", greeting_file);
    printf("  File luu log  : %s\n\n", log_file);
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t          client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept()");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);
        printf("[tcp_server] Client ket noi: %s:%d\n", client_ip, client_port);
        FILE *gf = fopen(greeting_file, "r");
        if (gf == NULL) {
            fprintf(stderr, "  Khong mo duoc file chao: %s\n", greeting_file);
        } else {
            char line[BUFFER_SIZE];
            while (fgets(line, sizeof(line), gf) != NULL) {
                if (send(client_fd, line, strlen(line), 0) < 0) {
                    perror("  send() greeting");
                    break;
                }
            }
            fclose(gf);
            printf("  Da gui cau chao.\n");
        }
        FILE *lf = fopen(log_file, "a");
        if (lf == NULL) {
            fprintf(stderr, "  Khong mo duoc file log: %s\n", log_file);
        }

        char   buffer[BUFFER_SIZE];
        int    n;
        size_t total = 0;
        while ((n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
            buffer[n] = '\0';
            printf("  [Nhan] %s", buffer);
            if (lf) {
                fwrite(buffer, 1, n, lf);
                fflush(lf);
            }
            total += n;
        }

        if (lf) fclose(lf);
        close(client_fd);
        printf("  Client %s:%d ngat ket noi. Tong %zu byte da luu vao '%s'.\n\n",
               client_ip, client_port, total, log_file);
    }

    close(server_fd);
    return 0;
}
