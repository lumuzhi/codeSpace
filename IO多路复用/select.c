#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main() {
    int server_fd, max_fd, ready_fd, i;
    fd_set read_fds;
    int client_fds[MAX_CLIENTS];
    char buffer[BUFFER_SIZE];

    // 创建服务器套接字
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 绑定服务器地址
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 监听连接
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 初始化文件描述符集合和客户端文件描述符数组
    FD_ZERO(&read_fds);
    FD_SET(server_fd, &read_fds);
    max_fd = server_fd;

    for (i = 0; i < MAX_CLIENTS; ++i) {
        client_fds[i] = -1;
    }

    while (1) {
        // 复制文件描述符集合，因为select会修改它
        fd_set tmp_fds = read_fds;

        // 调用select等待事件发生
        ready_fd = select(max_fd + 1, &tmp_fds, NULL, NULL, NULL);
        if (ready_fd == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // 处理所有就绪的事件
        for (i = 0; i <= max_fd; ++i) {
            if (FD_ISSET(i, &tmp_fds)) {
                // 如果是服务器套接字就绪，表示有新的客户端连接
                if (i == server_fd) {
                    // 接受客户端连接
                    struct sockaddr_in client_addr;
                    socklen_t client_addr_len = sizeof(client_addr);
                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
                    if (client_fd == -1) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }

                    // 将新的客户端文件描述符加入到文件描述符集合和客户端文件描述符数组中
                    for (i = 0; i < MAX_CLIENTS; ++i) {
                        if (client_fds[i] == -1) {
                            client_fds[i] = client_fd;
                            break;
                        }
                    }

                    if (i == MAX_CLIENTS) {
                        fprintf(stderr, "Too many clients\n");
                        exit(EXIT_FAILURE);
                    }

                    FD_SET(client_fd, &read_fds);
                    if (client_fd > max_fd) {
                        max_fd = client_fd;
                    }

                    printf("New client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                } 
                // 如果是客户端套接字就绪，表示有数据到达
                else {
                    memset(buffer, 0, sizeof(buffer));
                    int bytes_read = read(i, buffer, sizeof(buffer));
                    if (bytes_read == -1) {
                        perror("read");
                        exit(EXIT_FAILURE);
                    }

                    if (bytes_read == 0) {
                        // 客户端关闭了连接
                        printf("Client disconnected\n");
                        close(i);
                        FD_CLR(i, &read_fds);

                        // 从客户端文件描述符数组中移除该客户端
                        for (i = 0; i < MAX_CLIENTS; ++i) {
                            if (client_fds[i] == i) {
                                client_fds[i] = -1;
                                break;
                            }
                        }
                    } else {
                        // 打印收到的数据
                        printf("Received data from client: %s\n", buffer);
                    }
                }
            }
        }
    }

    // 关闭服务器套接字
    close(server_fd);

    return 0;
}
