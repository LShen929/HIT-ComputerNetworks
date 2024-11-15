#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEST_IP "192.168.208.128"  // 目的地址
#define DEST_PORT 54321  // 发送消息的目的端口
#define SERVER_PORT 12345  // 本地监听端口
#define RESPONSE "Hello! Got your message loud and clear."  // 回复消息
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr, dest_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // 创建 UDP 套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 设置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // 绑定套接字到本地端口
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 1;
    }

    // 接收数据包
    while (1) {  // 持续监听
        // 清空缓冲区
        memset(buffer, 0, BUFFER_SIZE);

        int recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("Recvfrom failed");
            continue;  // 如果发生错误，继续下一次循环
        }

        // 打印接收到的消息
        buffer[recv_len] = '\0';
        printf("Received message: %s\n", buffer);

        // 设置目的地址
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(DEST_PORT);
        if (inet_pton(AF_INET, DEST_IP, &dest_addr.sin_addr) <= 0) {
            perror("Invalid address/ Address not supported");
            close(sockfd);
            return 1;
        }

        // 发送响应消息
        if (sendto(sockfd, RESPONSE, strlen(RESPONSE), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Sendto failed");
        }
        else {
            printf("Response sent to %s:%d\n", DEST_IP, DEST_PORT);
        }
    }

    close(sockfd);
    return 0;
}