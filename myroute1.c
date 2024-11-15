#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <time.h>
#define BUFFER_SIZE 65536

//����ԴIP��Ŀ��IP��Ŀ��MAC��ַ�Ľṹ
struct HostPair {
    char* src_ip;
    char* dst_ip;
    unsigned char target_mac[ETH_ALEN];
};

//����·�ɱ��е�������
struct HostPair host_pairs[] = {
    {"192.168.43.97", "192.168.43.215", {0x00, 0x0c, 0x29, 0x81, 0xe4, 0xb3}},  // Դ������Ŀ��������route1MAC
    {"192.168.43.69", "192.168.43.215", {0x00, 0x0c, 0x29, 0x8f, 0x9e, 0x76}}, // route1��Ŀ��������route2MAC
    {"192.168.43.66", "192.168.43.215", {0x00, 0x0c, 0x29, 0x20, 0x80, 0xbc}}, // route2��Ŀ��������route3MAC
    {"192.168.43.155", "192.168.43.215", {0x00, 0x0c, 0x29, 0x31, 0xad, 0xbd}}, // route3��Ŀ��������Ŀ������MAC
};
#define NUM_HOST_PAIRS (sizeof(host_pairs) / sizeof(host_pairs[0]))

unsigned short checksum(void* b, int len)
{
    unsigned short* buf = b;
    unsigned int sum = 0;
    unsigned short result;
    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
int main()
{
    int sockfd;
    struct sockaddr saddr;
    unsigned char* buffer = (unsigned char*)malloc(BUFFER_SIZE);
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return 1;
    }
    while (1)
    {
        int saddr_len = sizeof(saddr);
        int data_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, &saddr, (socklen_t*)&saddr_len);
        if (data_size < 0)
        {
            perror("Recvfrom error");
            return 1;
        }
        struct ethhdr* eth_header = (struct ethhdr*)buffer;
        struct iphdr* ip_header = (struct iphdr*)(buffer + sizeof(struct ethhdr));
        char src_ip[INET_ADDRSTRLEN];
        char dest_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(ip_header->saddr), src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(ip_header->daddr), dest_ip, INET_ADDRSTRLEN);
        
        //����·�ɱ��е�������
        for (int i = 0; i < NUM_HOST_PAIRS; i++) {
            if (strcmp(src_ip, host_pairs[i].src_ip) == 0 && strcmp(dest_ip, host_pairs[i].dst_ip) == 0)
            {
                // ��ȡ��ǰϵͳʱ��
                time_t rawtime;
                struct tm* timeinfo;
                char time_str[100];
                time(&rawtime);
                timeinfo = localtime(&rawtime);
                // ��ʽ��ʱ���ַ���
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

                // ��ӡ��Ϣ
                printf("[%s] Captured packet from %s to %s\n", time_str, src_ip, dest_ip);

                ip_header->saddr = inet_addr(host_pairs[i + 1].src_ip);//����·�ɱ��޸�IP���ݱ����ͷ�IP��ַ

                // �޸� TTL
                ip_header->ttl -= 1;
                ip_header->check = 0;
                //ip_header->tot_len = htons(20 + 8 + 30); // �ܳ���=IP�ײ�����+IP���ݳ���

                ip_header->check = checksum((unsigned short*)ip_header, ip_header->ihl * 4);
                // �������ݰ���Ŀ������
                struct ifreq ifr, ifr_mac;
                struct sockaddr_ll dest;
                // ��ȡ�����ӿ�����
                memset(&ifr, 0, sizeof(ifr));
                snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "ens33");
                if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0)
                {
                    perror("ioctl");
                    return 1;
                }
                // ��ȡ�����ӿ� MAC ��ַ
                memset(&ifr_mac, 0, sizeof(ifr_mac));
                snprintf(ifr_mac.ifr_name, sizeof(ifr_mac.ifr_name), "ens33");
                if (ioctl(sockfd, SIOCGIFHWADDR, &ifr_mac) < 0)
                {
                    perror("ioctl");
                    return 1;
                }

                // ����·�ɱ�����Ŀ�� MAC ��ַ
                memset(&dest, 0, sizeof(dest));
                dest.sll_ifindex = ifr.ifr_ifindex;
                dest.sll_halen = ETH_ALEN;
                memcpy(dest.sll_addr, host_pairs[i+1].target_mac, ETH_ALEN);
                // �����µ���̫��֡ͷ
                memcpy(eth_header->h_dest, host_pairs[i+1].target_mac, ETH_ALEN);//��host_pairs[i+1].target_mac��ΪĿ��MAC��ַ
                memcpy(eth_header->h_source, host_pairs[i].target_mac, ETH_ALEN);//��host_pairs[i].target_mac��ΪԴMAC��ַ
                eth_header->h_proto = htons(ETH_P_IP);                              // ��̫������Ϊ IP
                printf("Interface name: %s, index: %d\n", ifr.ifr_name, ifr.ifr_ifindex);
                if (sendto(sockfd, buffer, data_size, 0, (struct sockaddr*)&dest,
                    sizeof(dest)) < 0)
                {
                    perror("Sendto error");
                    return 1;
                }
                printf("Datagram forwarded.\n");
            }
            else
            {
                // printf("Ignored packet from %s to %s\n", src_ip, dest_ip);
            }
        }
    }
    close(sockfd);
    free(buffer);
    return 0;
}