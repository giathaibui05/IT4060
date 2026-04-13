#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    int sockfd;
    int registered;
    char client_id[50];
} client_t;

client_t clients[MAX_CLIENTS];

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = -1;
        clients[i].registered = 0;
    }
}

void add_client(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd == -1) {
            clients[i].sockfd = sockfd;
            clients[i].registered = 0;
            return;
        }
    }
}

void remove_client(int sockfd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd == sockfd) {
            close(sockfd);
            clients[i].sockfd = -1;
            clients[i].registered = 0;
            memset(clients[i].client_id, 0, sizeof(clients[i].client_id));
            return;
        }
    }
}

char* get_time_str() {
    static char buf[64];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", t);
    return buf;
}

int parse_registration(char *msg, char *client_id) {
    // format: client_id: client_name (không ch?a space)
    char *colon = strchr(msg, ':');
    if (!colon) return 0;

    *colon = '\0';
    char *id = msg;
    char *name = colon + 1;

    // lo?i b? newline
    name[strcspn(name, "\r\n")] = 0;

    if (strlen(id) == 0 || strlen(name) == 0) return 0;
    if (strchr(name, ' ') != NULL) return 0;

    strcpy(client_id, id);
    return 1;
}

void broadcast(int sender_sock, char *msg) {
    char buffer[BUFFER_SIZE];
    char *time_str = get_time_str();

    char sender_id[50] = "unknown";

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd == sender_sock) {
            strcpy(sender_id, clients[i].client_id);
            break;
        }
    }

    snprintf(buffer, sizeof(buffer), "%s %s: %s",
             time_str, sender_id, msg);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 &&
            clients[i].sockfd != sender_sock &&
            clients[i].registered) {
            send(clients[i].sockfd, buffer, strlen(buffer), 0);
        }
    }
}

int main() {
    int server_fd, new_socket, max_sd;
    struct sockaddr_in address;
    fd_set readfds;

    init_clients();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sockfd;
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        // New connection
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, NULL, NULL);
            printf("New connection: %d\n", new_socket);
            add_client(new_socket);
            char *msg = "Enter ID format: client_id: client_name\n";
            send(new_socket, msg, strlen(msg), 0);
        }

        // Handle clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = clients[i].sockfd;

            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE] = {0};
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);

                if (valread <= 0) {
                    remove_client(sd);
                    continue;
                }

                if (!clients[i].registered) {
                    if (parse_registration(buffer, clients[i].client_id)) {
                        clients[i].registered = 1;
                        send(sd, "Registered successfully!\n", 27, 0);
                    } else {
                        send(sd, "Invalid format. Use: id: name(no space)\n", 44, 0);
                    }
                } else {
                    broadcast(sd, buffer);
                }
            }
        }
    }

    return 0;
}
