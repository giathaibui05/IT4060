#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT      9000
#define BUF_SIZE  65536

typedef struct { char name[256]; uint32_t size; } FileInfo;
static int send_all(int fd, const void *buf, int n) {
    int done = 0;
    while (done < n) {
        int r = send(fd, (const char*)buf + done, n - done, 0);
        if (r <= 0) return -1;
        done += r;
    }
    return done;
}

int main(void) {
    char dir[1024];
    if (!getcwd(dir, sizeof(dir))) { perror("getcwd"); return 1; }
    DIR *dp = opendir(dir);
    if (!dp) { perror("opendir"); return 1; }

    FileInfo files[1024];
    int num_files = 0;
    struct dirent *ent;
    while ((ent = readdir(dp)) != NULL) {
        if (ent->d_type != DT_REG) continue;
        char path[2048];
        snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
        struct stat st;
        if (stat(path, &st) == 0 && num_files < 1024) {
            snprintf(files[num_files].name, sizeof(files[num_files].name), "%s", ent->d_name);
            files[num_files].size = (uint32_t)st.st_size;
            num_files++;
        }
    }
    closedir(dp);

    uint8_t buf[BUF_SIZE];
    int off = 4; 

    uint16_t dl = htons((uint16_t)strlen(dir));
    memcpy(buf+off, &dl, 2);              off += 2;
    memcpy(buf+off, dir, strlen(dir));    off += strlen(dir);

    uint16_t nf = htons((uint16_t)num_files);
    memcpy(buf+off, &nf, 2);              off += 2;

    for (int i = 0; i < num_files; i++) {
        uint8_t nl = (uint8_t)strlen(files[i].name);
        buf[off++] = nl;
        memcpy(buf+off, files[i].name, nl); off += nl;
        uint32_t sz = htonl(files[i].size);
        memcpy(buf+off, &sz, 4);          off += 4;
    }

    uint32_t total = htonl((uint32_t)(off - 4));
    memcpy(buf, &total, 4);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(sock); return 1;
    }

    if (send_all(sock, buf, off) < 0)
        fprintf(stderr, "Send failed\n");
    else
        printf("Sent %d bytes for dir=%s (%d files)\n", off-4, dir, num_files);

    close(sock);
    return 0;
}
