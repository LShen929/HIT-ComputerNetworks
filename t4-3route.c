#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <time.h>
#include <net/ethernet.h>
#include <errno.h>

#define BUFFER_SIZE 65536

struct route_entry
{
    uint32_t dest;
    uint32_t gateway;
    uint32_t netmask;
    char interface[IFNAMSIZ];
};

struct route_entry route_table[2];  // 存储路由表的数组

int route_table_size = sizeof(route_table) / sizeof(route_table[0]);

// 将IP地址转换为字符串格式
void convert_to_ip_string(uint32_t ip_addr, char* ip_str)
{
    struct in_addr addr;
    addr.s_addr = ip_addr;
    inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);
}

// 计算IP头部校验和
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

// 查找路由表，根据目标IP地址获取下一跳路由
struct route_entry* lookup_route(uint32_t dest_ip)
{
    for (int i = 0; i < route_table_size; i++)
    {
        if ((dest_ip & route_table[i].netmask) == (route_table[i].dest & route_table[i].netmask))
        {
            return &route_table[i];
        }
    }
    return NULL;  // 没有找到匹配的路由
}

// 初始化路由表，设定两个路由条目
void initialize_route_table()
{
    // 发送主机到路由器的路由条目
    route_table[0].dest = inet_addr("192.168.208.0");  // 目标网络
    route_table[0].gateway = inet_addr("192.168.208.128"); // 网关地址
    route_table[0].netmask = inet_addr("255.255.255.0"); // 子网掩码
    strcpy(route_table[0].interface, "ens33");  // 发送主机到路由器的接口

    // 路由器到接收主机的路由条目
    route_table[1].dest = inet_addr("192.168.72.0");    // 目标网络
    route_table[1].gateway = inet_addr("192.168.72.128"); // 网关地址
    route_table[1].netmask = inet_addr("255.255.255.0"); // 子网掩码
    strcpy(route_table[1].interface, "ens37");  // 路由器到接收主机的接口
}

// 从 /proc/net/arp 文件中获取给定 IP 地址的 MAC 地址
int get_mac_from_arp_cache(uint32_t ip_addr, unsigned char* mac)
{
    FILE* arp_file = fopen("/proc/net/arp", "r");  // 打开 /proc/net/arp 文件
    if (arp_file == NULL)
    {
        perror("Failed to open /proc/net/arp");
        return -1;  // 打开文件失败
    }
    char line[256];
    if (fgets(line, sizeof(line), arp_file) == NULL)// 读取并忽略第一行标题
    {
        fclose(arp_file);
        return -1;  // 没有数据行
    }
    while (fgets(line, sizeof(line), arp_file))    // 遍历 arp 缓存文件中的每一行
    {
        line[strcspn(line, "\n")] = 0; //去掉行尾的换行符
        char ip[16], hw_type[16], flags[16], hw_address[20], mask[16], device[16];//分割字符串
        if (sscanf(line, "%15[^ ] %15[^ ] %15[^ ] %19[^ ] %15[^ ] %15[^\n]",
            ip, hw_type, flags, hw_address, mask, device) != 6)
        {
            fprintf(stderr, "Failed to parse line: %s\n", line);
            continue;  // 跳过无法解析的行
        }
        struct in_addr in; // 将 IP 地址字符串转换为无符号长整型
        if (inet_aton(ip, &in) == 0)
        {
            continue;  // 跳过无效的 IP 地址
        }
        unsigned long parsed_ip = in.s_addr;
        // 比较 ARP 缓存中的 IP 地址与目标 IP 地址
        if (parsed_ip == ip_addr)  // 如果目标 IP 地址匹配
        {
            // 将 MAC 地址字符串转换为字节数组
            if (sscanf(hw_address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) != 6)
            {
                fprintf(stderr, "Failed to parse MAC address: %s\n", hw_address);
                fclose(arp_file);
                return -1;  // 无法解析 MAC 地址
            }
            fclose(arp_file);
            printf("找到目标 IP 地址 %s 的 MAC 地址: %02x:%02x:%02x:%02x:%02x:%02x\n",
                ip, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            return 0;  // 成功获取 MAC 地址
        }
    }
    fclose(arp_file);
    printf("未能找到 IP 地址 %s 对应的 MAC 地址\n", inet_ntoa((struct in_addr) { ip_addr }));
    return -1;  // 没有在 ARP 缓存中找到对应的 MAC 地址
}

int main()
{
    int sockfd;
    struct sockaddr saddr;
    unsigned char* buffer = (unsigned char*)malloc(BUFFER_SIZE);  // 为数据包分配内存

    // 初始化路由表
    initialize_route_table();

    // 创建一个原始套接字用于捕获数据包
    sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if (sockfd < 0)
    {
        perror("Socket creation failed");
        return 1;  // 创建套接字失败
    }

    while (1)
    {
        int saddr_len = sizeof(saddr);
        int data_size = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, &saddr, (socklen_t*)&saddr_len);
        if (data_size < 0)
        {
            perror("Recvfrom error");
            return 1;  // 接收数据包失败
        }
        if (data_size == 0)
            continue;  // 没有接收到数据包时跳过

        // 解析以太网帧头
        struct ethhdr* eth_header = (struct ethhdr*)buffer;
        struct iphdr* ip_header = (struct iphdr*)(buffer + sizeof(struct ethhdr));

        // 查找目标 IP 地址的路由信息
        struct route_entry* route = lookup_route(ip_header->daddr);
        if (route == NULL)
        {
            // 如果没有找到匹配的路由，跳过该数据包
            continue;
        }

        // 将源 IP 和目的 IP 地址转换为字符串形式
        char ip_s[32], ip_d[32];
        convert_to_ip_string(ip_header->saddr, ip_s);
        convert_to_ip_string(ip_header->daddr, ip_d);
        printf("捕获到数据包，从 %s 到 %s\n", ip_s, ip_d);

        // 修改 IP 包的 TTL（生存时间）
        ip_header->ttl -= 1;
        ip_header->check = 0;  // 在重新计算校验和前，清空校验和
        ip_header->check = checksum((unsigned short*)ip_header, ip_header->ihl * 4);  // 重新计算 IP 校验和

        // 获取路由接口的索引和目标 MAC 地址
        struct ifreq ifr, ifr_mac;
        struct sockaddr_ll dest;

        // 获取网络接口的索引
        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, route->interface, sizeof(ifr.ifr_name) - 1);
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';  // 确保字符串以'\0'结尾
        if (ioctl(sockfd, SIOCGIFINDEX, &ifr) < 0)
        {
            perror("Get interface index error");
            return 1;
        }

        // 获取目标 MAC 地址
        unsigned char mac[6];
        if (get_mac_from_arp_cache(ip_header->daddr, mac) == 0)
        {
            printf("目标MAC地址: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
        else
        {
            printf("未能获取目标 MAC 地址\n");
            continue;  // 如果没有获取到目标 MAC 地址，跳过该数据包
        }

        // 获取本机接口的 MAC 地址
        memset(&ifr_mac, 0, sizeof(ifr_mac));
        strncpy(ifr_mac.ifr_name, route->interface, sizeof(ifr_mac.ifr_name) - 1);
        ifr_mac.ifr_name[sizeof(ifr_mac.ifr_name) - 1] = '\0';
        if (ioctl(sockfd, SIOCGIFHWADDR, &ifr_mac) < 0)
        {
            perror("Get interface MAC address error");
            return 1;
        }

        // 构造新的以太网帧头
        memcpy(eth_header->h_dest, mac, 6);  // 目标 MAC 地址
        memcpy(eth_header->h_source, ifr_mac.ifr_hwaddr.sa_data, 6);  // 源 MAC 地址
        eth_header->h_proto = htons(ETH_P_IP);  // 以太网类型为 IP

        // 发送修改后的数据包
        memset(&dest, 0, sizeof(dest));
        dest.sll_family = AF_PACKET;
        dest.sll_protocol = htons(ETH_P_IP);
        dest.sll_ifindex = ifr.ifr_ifindex;
        dest.sll_halen = 6;
        memcpy(dest.sll_addr, mac, 6);

        if (sendto(sockfd, buffer, data_size, 0, (struct sockaddr*)&dest, sizeof(dest)) < 0)
        {
            perror("Send failed");
            return 1;  // 发送数据包失败
        }
        printf("数据包转发成功\n");
    }

    close(sockfd);
    free(buffer);
    return 0;
}