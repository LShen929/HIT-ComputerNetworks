#include <stdio.h>
#include <stdlib.h>
#include <string.h>  
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>  

#define BUFFER_SIZE 1024
#define PORT 54321

int main() {
    int sockfd;
    struct sockaddr_in my_addr, src_addr;
    char buffer[BUFFER_SIZE];

    // 创建 UDP 套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    // 本地地址
    memset(&my_addr, 0, sizeof(my_addr));  // 初始化结构体
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定套接字到本地地址
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        close(sockfd);  // 在退出前关闭套接字
        return 1;
    }

    // 循环接收数据报
    while (1) {
        // 接收数据报
        socklen_t addr_len = sizeof(src_addr);
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&src_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            close(sockfd);  // 在退出前关闭套接字
            return 1;
        }

        // 确保字符串以空字符结尾
        buffer[bytes_received] = '\0';

        // 打印接收到的数据
        printf("Datagram received ：%s\n",buffer);
    }

    // 关闭套接字
    close(sockfd);
    return 0;
}