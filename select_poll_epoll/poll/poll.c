#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 1024
#define PORT 8080

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 5) < 0) {
        perror("listen");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    if (set_nonblocking(listenfd) == -1) {
        perror("set_nonblocking");
        close(listenfd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;

    fds[0].fd = listenfd;
    fds[0].events = POLLIN;

    for (int i = 1; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;
    }

    while (1) {
        int ready = poll(fds, nfds, -1);
        if (ready < 0) {
            perror("poll");
            break;
        }

        // 监听 socket 可读：新连接
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int connfd = accept(listenfd, (struct sockaddr*)&client_addr, &client_len);
            if (connfd < 0) {
                perror("accept");
                continue;
            }

            if (set_nonblocking(connfd) == -1) {
                perror("set_nonblocking");
                close(connfd);
                continue;
            }

            printf("Accepted connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            int i;
            for (i = 1; i < MAX_CLIENTS; i++) {
                if (fds[i].fd == -1) {
                    fds[i].fd = connfd;
                    fds[i].events = POLLIN;
                    if (i >= nfds) nfds = i + 1;
                    break;
                }
            }

            if (i == MAX_CLIENTS) {
                fprintf(stderr, "Too many clients\n");
                close(connfd);
            }
        }

        // 遍历其他客户端
        for (int i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) continue;

            if (fds[i].revents & POLLIN) {
                char buffer[1024];
                ssize_t n = read(fds[i].fd, buffer, sizeof(buffer));

                if (n <= 0) {
                    if (n < 0) perror("read");
                    else printf("Client disconnected: fd=%d\n", fds[i].fd);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                } else {
                    for (int j = 0; j < n; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }
                    write(fds[i].fd, buffer, n);
                    printf("Echoed back: %.*s\n", (int)n, buffer);
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
