#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>
#include <ctype.h>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }

    if(listen(listenfd, 5) < 0) {
        perror("listen");
        close(listenfd);
        return -1;
    }

    if(set_nonblocking(listenfd) == -1) {
        perror("set_nonblocking");
        close(listenfd);
        return -1;
    }

    printf("Server listening on port 8080\n");

    fd_set all_fds, read_fds;
    FD_ZERO(&all_fds);
    FD_SET(listenfd, &all_fds);
    int max_fd = listenfd;

    while(1)
    {
        read_fds = all_fds;

        int ret = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if(ret < 0) {
            perror("select");
            break;
        }

        for(int fd = 0; fd <= max_fd; fd++) {
            if(FD_ISSET(fd, &read_fds)) {
                if(fd == listenfd) {
                    // 新客户端连接
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);
                    if(connfd < 0) {
                        perror("accept");
                        continue;
                    }

                    printf("Accepted connection from %s:%d\n",
                        inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                    if(set_nonblocking(connfd) == -1) {
                        perror("set_nonblocking");
                        close(connfd);
                        continue;
                    }

                    FD_SET(connfd, &all_fds);
                    if(connfd > max_fd) max_fd = connfd;
                } else {
                    // 客户端发数据
                    char buffer[1024];
                    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
                    if(bytes_read < 0) {
                        perror("read");
                        close(fd);
                        FD_CLR(fd, &all_fds);
                    }
                    else if(bytes_read == 0) {
                        printf("Client disconnected: fd=%d\n", fd);
                        close(fd);
                        FD_CLR(fd, &all_fds);
                    }
                    else {
                        // 转换为大写并回写
                        for(int i = 0; i < bytes_read; i++) {
                            buffer[i] = toupper((unsigned char)buffer[i]);
                        }

                        write(fd, buffer, bytes_read);
                        printf("Echoed back: %.*s\n", (int)bytes_read, buffer);
                    }
                }
            }
        }
    }

    close(listenfd);
    return 0;
}
