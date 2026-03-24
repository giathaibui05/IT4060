/*
 * sv_server.c
 * Bài 4: Nhận thông tin sinh viên từ sv_client,
 * in ra màn hình và ghi vào file log kèm IP + thời gian.
 *
 * Cách chạy: ./sv_server <cổng> <tên file log>
 * Ví dụ:     ./sv_server 9090 sv_log.txt
 *
 * Định dạng mỗi dòng trong file log:
 * <IP> <YYYY-MM-DD HH:MM:SS> <MSSV> <HoTen> <NgaySinh> <DiemTB>
 * Ví dụ:
 * 127.0.0.1 2023-04-10 09:00:00 20201234 Nguyen Van A 2002-04-10 3.99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 512
#define BACKLOG     10

/* Lấy timestamp hiện tại dạng "YYYY-MM-DD HH:MM:SS" */
static void get_timestamp(char *buf, size_t buf_size) {
    time_t     now = time(NULL);
    struct tm *t   = localtime(&now);
    strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", t);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Cach dung: %s <cong> <file log>\n", argv[0]);
        fprintf(stderr, "Vi du:     %s 9090 sv_log.txt\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int         port     = atoi(argv[1]);
    const char *log_file = argv[2];

    /* ---- Tạo socket ---- */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket()"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* ---- Bind ---- */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        close(server_fd); exit(EXIT_FAILURE);
    }

    /* ---- Listen ---- */
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen()");
        close(server_fd); exit(EXIT_FAILURE);
    }

    printf("[sv_server] Dang lang nghe tren cong %d\n", port);
    printf("[sv_server] File log: %s\n\n", log_file);

    /* ---- Vòng lặp chấp nhận kết nối ---- */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t          client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept()");
            continue;
        }

        /* Địa chỉ IP client */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        /* Thời điểm nhận kết nối */
        char timestamp[32];
        get_timestamp(timestamp, sizeof(timestamp));

        /* Nhận dữ liệu */
        char   buffer[BUFFER_SIZE];
        int    n = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        close(client_fd);

        if (n <= 0) {
            fprintf(stderr, "[sv_server] Khong nhan duoc du lieu tu %s\n", client_ip);
            continue;
        }

        buffer[n] = '\0';
        /* Xóa ký tự xuống dòng cuối (nếu có) để log gọn */
        buffer[strcspn(buffer, "\n")] = '\0';

        /* Tạo dòng log: "<IP> <timestamp> <nội dung>" */
        char log_line[BUFFER_SIZE + 64];
        snprintf(log_line, sizeof(log_line), "%s %s %s", client_ip, timestamp, buffer);

        /* In ra màn hình */
        printf("[sv_server] Nhan tu %s luc %s:\n  %s\n\n",
               client_ip, timestamp, buffer);

        /* Ghi vào file log */
        FILE *lf = fopen(log_file, "a");
        if (lf == NULL) {
            perror("fopen() log_file");
        } else {
            fprintf(lf, "%s\n", log_line);
            fflush(lf);
            fclose(lf);
        }
    }

    close(server_fd);
    return 0;
}
