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
    int dest_port = 12345; // Ŀ��˿ںţ����ճ���Ķ˿ںţ�

    // ���� UDP �׽���
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket");
        return 1;
    }

    // Ŀ���ַ
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(dest_port);
    dest_addr.sin_addr.s_addr = inet_addr("192.168.43.69"); // �滻Ϊ���ճ���������ʵ�� IP ��ַ

    // ѭ�����տ���̨�������Ϣ������
    while (1) {
        printf("Enter message to send (type 'exit' to quit): ");
        fflush(stdout); // ȷ����ʾ��Ϣ��������ӡ

        // ʹ�� fgets ��ȡ����̨�������Ϣ
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            perror("fgets");
            return 1;
        }

        // ����Ƿ��������˳�����
        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        // �Ƴ����з�
        buffer[strcspn(buffer, "\n")] = 0;

        // �������ݱ�
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("sendto");
            return 1;
        }
        printf("Datagram sent.\n");
    }

    // �ر��׽���
    close(sockfd);
    return 0;
}