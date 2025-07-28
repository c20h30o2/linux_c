#include<stdio.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<string.h>
#include<errno.h>   

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>



//epoll 反应堆 服务端
int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    if(port <= 0 || port > 65535)
    {
        printf("Invalid port number.\n");
        return -1;
    }

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(listenfd);
        return -1;
    }

    if(listen(listenfd, 5) < 0)
    {
        perror("listen");
        close(listenfd);
        return -1;
    }

    printf("Server listening on port %d\n", port);

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if(epoll_fd < 0)
    {
        perror("epoll_create1");
        close(listenfd);
        return -1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN; // Monitor for input events
    ev.data.fd = listenfd;

    // Add the listening socket to the epoll instance
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &ev) < 0)
    {
        perror("epoll_ctl");
        close(listenfd);
        close(epoll_fd);
        return -1;
    }

    struct epoll_event events[10]; // Array to hold events

    while(1)
    {
        int nfds = epoll_wait(epoll_fd, events, 10, -1);
        if(nfds < 0)
        {
            perror("epoll_wait");
            break;
        }

        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd == listenfd) // New connection
            {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_len);
                if(connfd < 0)
                {
                    perror("accept");
                    continue;
                }

                printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                // Set the new socket to non-blocking mode
                int flags = fcntl(connfd, F_GETFL, 0);
                fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

                // Add the new socket to the epoll instance
                ev.events = EPOLLIN | EPOLLET; // Edge-triggered
                ev.data.fd = connfd;
                if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &ev) < 0)
                {
                    perror("epoll_ctl");
                    close(connfd);
                    continue;
                }
            }
            else // Existing connection
            {
                char buffer[1024];
                ssize_t bytes_read = read(events[i].data.fd, buffer, sizeof(buffer));
                if(bytes_read < 0)
                {
                    perror("read");
                    close(events[i].data.fd);
                    continue;
                }
                else if(bytes_read == 0) // Connection closed by client
                {
                    printf("Client disconnected\n");
                    close(events[i].data.fd);
                    continue;
                }

                printf("Received: %.*s\n", (int)bytes_read, buffer);
                // Echo back the data
                ssize_t bytes_written = write(events[i].data.fd, buffer, bytes_read);
                if(bytes_written < 0)
                {
                    perror("write");
                    close(events[i].data.fd);
                    continue;
                }
                printf("Sent: %.*s\n", (int)bytes_written, buffer);
            }
        }
    }   

    close(epoll_fd);
    close(listenfd);
    return 0;
}