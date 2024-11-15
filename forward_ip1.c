#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in src_addr, dest_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len;
    int src_port = 12345;  // ԭʼ�˿ں�
    int dest_port = 54321; // Ŀ��˿ںţ����ճ���Ķ˿ںţ�

    // ���� UDP �׽���
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket");
        return 1;
    }

    // ���ص�ַ
    struct sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(src_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // ���׽��ֵ����ص�ַ
    if (bind(sockfd, (struct sockaddr*)&my_addr, sizeof(my_addr)) < 0) {
        perror("bind");
        return 1;
    }

    // ѭ���������ݱ���ת��
    while (1) {
        addr_len = sizeof(src_addr);
        // �������ݱ�
        ssize_t bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&src_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom");
            return 1;
        }

        // ȷ���ַ����Կ��ַ���β
        buffer[bytes_received] = '\0';

        // ��ӡ���յ�������
        printf("Datagram received: %s\n", buffer);

        // �޸�Ŀ���ַΪ���ճ��������� IP ��ַ
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(dest_port);
        dest_addr.sin_addr.s_addr = inet_addr("192.168.43.215"); // �滻Ϊ���ճ���������ʵ�� IP ��ַ

        // �������ݱ�
        if (sendto(sockfd, buffer, bytes_received, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto");
            return 1;
        }
        printf("Datagram forwarded.\n");
    }

    // �ر��׽���
    close(sockfd);
    return 0;
}