#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h> 

#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in dest_addr;
    char buffer[BUFFER_SIZE];
    int dest_port = 12345; // 目标端口号（接收程序的端口号）

    // 创建 UDP 套接字
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket");
        return 1;
    }

    // 目标地址
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    dest_addr.sin_addr.s_addr = inet_addr("192.168.43.69"); // 替换为接收程序主机的实际 IP 地址

    // 循环接收控制台输入的消息并发送
    while (1) {
        printf("Enter message to send (type 'exit' to quit): ");
        fflush(stdout); // 确保提示信息被立即打印

        // 使用 fgets 读取控制台输入的消息
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("fgets");
            return 1;
        }

        // 检查是否输入了退出命令
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        // 移除换行符
        buffer[strcspn(buffer, "\n")] = 0;

        // 发送数据报
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto");
            return 1;
        }
        printf("Datagram sent.\n");
    }

    // 关闭套接字
    close(sockfd);
    return 0;
}