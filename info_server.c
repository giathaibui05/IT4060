#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9000
static int recv_all(int fd, void *buf, int n) {
    int done = 0;
    while (done < n) {
        int r = recv(fd, (char*)buf + done, n - done, 0);
        if (r <= 0) return -1;
        done += r;
    }
    return done;
}

int main(void) {
    int srv = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    listen(srv, 5);
    printf("info_server listening on port %d ...\n", PORT);

    while (1) {
        int cli = accept(srv, NULL, NULL);
        if (cli < 0) continue;

        uint32_t total_net;
        if (recv_all(cli, &total_net, 4) < 0) { close(cli); continue; }
        uint32_t total = ntohl(total_net);

        uint8_t *buf = malloc(total);
        if (!buf || recv_all(cli, buf, total) < 0) {
            free(buf); close(cli); continue;
        }

        int off = 0;

        uint16_t dir_len = ntohs(*(uint16_t*)(buf+off)); off += 2;
        char dir[1024] = {0};
        memcpy(dir, buf+off, dir_len); off += dir_len;

        uint16_t num_files = ntohs(*(uint16_t*)(buf+off)); off += 2;

        printf("\nDirectory: %s\n", dir);
        for (int i = 0; i < num_files; i++) {
            uint8_t nl = buf[off++];
            char name[256] = {0};
            memcpy(name, buf+off, nl); off += nl;
            uint32_t size = ntohl(*(uint32_t*)(buf+off)); off += 4;
            printf("  %s - %u bytes\n", name, size);
        }
        printf("Total: %d file(s)\n\n", num_files);

        free(buf);
        close(cli);
    }

    close(srv);
    return 0;
}
