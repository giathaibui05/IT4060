#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT         9001
#define RECV_SIZE    4096
#define PATTERN      "0123456789"
#define PATTERN_LEN  10
#define OVERLAP      (PATTERN_LEN - 1)  /* 9 byte giữ lại giữa 2 chunk */

/* Đếm số lần xuất hiện (không chồng nhau) của pattern trong buf */
static int count_pattern(const char *buf, int len) {
    int count = 0, i = 0;
    while (i <= len - PATTERN_LEN) {
        if (memcmp(buf + i, PATTERN, PATTERN_LEN) == 0) {
            count++;
            i += PATTERN_LEN;   /* bỏ qua phần đã match */
        } else {
            i++;
        }
    }
    return count;
}

int main(void) {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    bind(srv, (struct sockaddr*)&addr, sizeof(addr));
    listen(srv, 5);
    printf("Server listening on port %d ...\n", PORT);

    while (1) {
        int cli = accept(srv, NULL, NULL);
        if (cli < 0) continue;
        printf("Client connected\n");

        /* search_buf: [overlap_tail | new_chunk] */
        char search_buf[OVERLAP + RECV_SIZE + 1];
        char overlap[OVERLAP];   /* 9 byte cuối của lần recv trước */
        int  overlap_len = 0;    /* = 0 ở lần recv đầu tiên */
        int  total_count = 0;

        char recv_buf[RECV_SIZE];
        int  n;

        while ((n = recv(cli, recv_buf, RECV_SIZE, 0)) > 0) {
            /* Ghép overlap + chunk mới vào search_buf */
            memcpy(search_buf, overlap, overlap_len);
            memcpy(search_buf + overlap_len, recv_buf, n);
            int search_len = overlap_len + n;

            /* Đếm và cộng dồn */
            int found = count_pattern(search_buf, search_len);
            total_count += found;
            printf("Received %d bytes | found %d | total = %d\n",
                   n, found, total_count);

            /* Cập nhật overlap: lấy OVERLAP byte cuối của search_buf */
            overlap_len = (search_len >= OVERLAP) ? OVERLAP : search_len;
            memcpy(overlap, search_buf + search_len - overlap_len, overlap_len);
        }

        printf("Connection closed. Final count = %d\n\n", total_count);
        close(cli);
    }

    close(srv);
    return 0;
}
