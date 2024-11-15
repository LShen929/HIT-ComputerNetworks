#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEST_IP "192.168.72.128"  // Ŀ�ĵ�ַ
#define DEST_PORT 12345  // ������Ϣ��Ŀ�Ķ˿�
#define SERVER_PORT 54321  // ���ؼ����˿�
#define MESSAGE "Hello, this is a test message."
#define BUFFER_SIZE 1024

int main() {
    int send_sockfd, recv_sockfd;
    struct sockaddr_in dest_addr, server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // ���������õ� UDP �׽���
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Socket creation for sending failed");
        return 1;
    }

    // ����Ŀ�ĵ�ַ
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(DEST_PORT);
    if (inet_pton(AF_INET, DEST_IP, &dest_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(send_sockfd);
        return 1;
    }

    // �������ݰ�
    if (sendto(send_sockfd, MESSAGE, strlen(MESSAGE), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Sendto failed");
        close(send_sockfd);
        return 1;
    }
    printf("Message sent to %s:%d\n", DEST_IP, DEST_PORT);

    // �رշ����õ��׽���
    close(send_sockfd);

    // ���������õ� UDP �׽���
    recv_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (recv_sockfd < 0) {
        perror("Socket creation for receiving failed");
        return 1;
    }

    // ���÷�������ַ
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // ���׽��ֵ����ض˿�
    if (bind(recv_sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(recv_sockfd);
        return 1;
    }

    // �������ݰ�
    while (1) {  // ��������
        // ��ջ�����
        memset(buffer, 0, BUFFER_SIZE);

        int recv_len = recvfrom(recv_sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("Recvfrom failed");
            continue;  // ����������󣬼�����һ��ѭ��
        }

        // ��ӡ���յ�����Ϣ
        buffer[recv_len] = '\0';
        printf("Received message: %s\n", buffer);
    }

    close(recv_sockfd);
    return 0;
}