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

    // ���� UDP �׽���
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket creation failed");
        return 1;
    }

    // ���ص�ַ
    memset(&my_addr, 0, sizeof(my_addr));  // ��ʼ���ṹ��
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // ���׽��ֵ����ص�ַ
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        close(sockfd);  // ���˳�ǰ�ر��׽���
        return 1;
    }

    // ѭ���������ݱ�
    while (1) {
        // �������ݱ�
        socklen_t addr_len = sizeof(src_addr);
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&src_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom failed");
            close(sockfd);  // ���˳�ǰ�ر��׽���
            return 1;
        }

        // ȷ���ַ����Կ��ַ���β
        buffer[bytes_received] = '\0';

        // ��ӡ���յ�������
        printf("Datagram received ��%s\n",buffer);
    }

    // �ر��׽���
    close(sockfd);
    return 0;
}