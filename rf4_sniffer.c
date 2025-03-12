// Packet sniffer using Npcap
// Compile with: gcc -o npcap_sniffer npcap_sniffer.c -lwpcap -lws2_32

#include <stdio.h>
#include <stdlib.h>
#include <pcap.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <time.h>
#include <locale.h>
#include "rf4_sniffer.h"
#include "rf4_ports.h"


Packet_filter fish_list[MAX_PACKET_FILTER_COUNT];
int fish_list_len;
Packet_filter invalid_packet_list[MAX_PACKET_FILTER_COUNT];
int invalid_list_len;

int font_color_list[4] = {15, 14, 11, 10};

char* wavList[3];
int* localPorts;
int localPortCount;
char localPortString[100];
int packet_received;
int interface_num;


char* load_wav_file(const char* filename) {
    size_t size = 0;
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("%s 不存在\n", filename);
        system("pause");
        exit(1);
    };

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0'; // Null-terminate for safety if it's a text file

    fclose(file);
    return buffer;
}

void read_data(const char* filename, Packet_filter *filter_list, int *count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("无法打开数据文件\n");
        system("pause");
        exit(1);
    }
    while (*count < MAX_PACKET_FILTER_COUNT && fscanf(file, "%89[^,],%d\n", 
        filter_list[*count].description, &filter_list[*count].number) == 2) {
        (*count)++;
    }
    fclose(file);
}

// Packet processing callback function
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
    
    // Extract Ethernet header
    ethernet_header *eth_header = (ethernet_header *)packet;
    ethernet_header_lo *eth_header_lo = (ethernet_header_lo *)packet;
    int eth_header_len = 0;
    if (eth_header_lo->lo_magic_number == 0x02) {
        eth_header_len = sizeof(ethernet_header_lo);
    } else {
        eth_header_len = sizeof(ethernet_header);
        if (ntohs(eth_header->eth_type) != 0x0800) return;
    }
    
    ip_header *ip_hdr = (ip_header *)(packet + eth_header_len);
    int ip_header_len = (ip_hdr->ver_ihl & 0x0F) * 4;
    
    if (ip_hdr->proto != PROTO_TCP) return;
    // Extract TCP header
    tcp_header *tcp_hdr = (tcp_header *)(packet + eth_header_len + ip_header_len);
    int tcp_header_len = ((tcp_hdr->data_off & 0xF0) >> 4) * 4;

    int push_flag = (tcp_hdr->flags & 0x08) >> 3;
    int data_length = ntohs(ip_hdr->tlen) - ip_header_len - tcp_header_len;

    int dport = ntohs(tcp_hdr->dport);
    int port_in_list = 0;
    for (int i = 0; i < localPortCount; i++) {
        if (dport == localPorts[i]) {
            port_in_list = 1;
            break;
        }
    }
    if (!port_in_list) return;
    if (!push_flag) return;

    if (!packet_received) {
        printf("\r网口:%d  俄钓端口:[%s\b]    \033[32m网口/端口有效\033[0m                 \n", interface_num, localPortString);
        packet_received = 1;
    }

    if (data_length < 525) return;
    if (data_length > 1800) return;
    for (int i = 0; i < invalid_list_len; i++) {
        if (invalid_packet_list[i].number == data_length) {
            return;
        }
    }
    
    u_char *raw_data = (u_char *)(packet + eth_header_len + ip_header_len + tcp_header_len);

    if  (raw_data[2] != 0 || raw_data[3] != 0) return;
    /* filter end */

    static int log_line_count = 0;

    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), font_color_list[log_line_count % 4]);
    log_line_count++;
    printf("No.%d  %02d:%02d:%02d  Packet:%4d  ", log_line_count, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,data_length);
    int have_prediction = 0;
    char prediction[100] = {0};
    for (int i = 0; i < fish_list_len; i++) {
        if (fish_list[i].number == data_length) {
            have_prediction = 1;
            sprintf(prediction + strlen(prediction), "%s ", fish_list[i].description);
        }
    }
    if (have_prediction) {
        printf("Predict:[%s\b]", prediction);
        PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    pcap_if_t *alldevs, *d;
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    srand(time(NULL));  // 设置随机种子
    wavList[0] = load_wav_file("./resource/1.wav");
    wavList[1] = load_wav_file("./resource/2.wav");
    wavList[2] = load_wav_file("./resource/3.wav");

    setlocale(LC_ALL, "en_US.UTF-8");
    SetConsoleTitleW(L"妙妙小工具");

    const char* processName = "rf4_x64.exe";
    DWORD pid = GetProcessIdByName(processName);
    if (pid) {
        localPortCount = GetTcpConnectionsForPid(pid, &localPorts);
        if (localPorts == NULL) {
            printf("游戏已打开，但尚未建立网络连接\n");
            system("pause");
            exit(1);
        }
    } else {
        printf("游戏未找到\n");
        system("pause");
        exit(1);
    }

    for (int i = 0; i < localPortCount; i++) {
        sprintf(localPortString + strlen(localPortString), "%d ", localPorts[i]);
    }

    // Initialize winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    // Get the list of available devices
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        fprintf(stderr, "Error finding devices: %s\n", errbuf);
        return -1;
    }
    int i = 0;
    // List all available interfaces if no specific interface is provided
    printf("\033[32m** data/fishes.txt记录上鱼数据包 (白名单，出现时会响)\n");
    printf("** data/invalid_packet.txt记录无效数据包 (黑名单，不会显示)\n");
    printf("** 以上两个文件可以随时修改,保证格式不变就好,修改后重启v2.exe生效\n");
    printf("** PS: 长度小于525的数据包默认丢弃\033[0m\n");

    printf("网口列表:\n");
    for (d = alldevs; d; d = d->next) {
        i++;
        // printf("%d. %s", i, d->name);
        printf("%d.", i);
        if (d->description) {
            printf(" %s\n", d->description);
        } else {
            printf(" (No description available)\n");
        }
    }
    if (i == 0) {
        printf("No interfaces found! Make sure Npcap is installed.\n");
        exit(1);
    }
    
    // Find the selected interface
    char interface_name[256] = {0};
    printf("选择网口号: ");
    scanf("%s", &interface_name);
    system("cls");
	i = 0;
    // Check if the user provided an interface number or name
    if (interface_name[0] >= '0' && interface_name[0] <= '9') {
        interface_num = atoi(interface_name);
        if (interface_num < 1) {
            fprintf(stderr, "Invalid interface number\n");
            pcap_freealldevs(alldevs);
            return -1;
        }
        
        // Find the selected interface by number
        for (d = alldevs, i = 0; d && i < interface_num - 1; d = d->next, i++);
        
        if (!d) {
            fprintf(stderr, "Interface number %d out of range\n", interface_num);
            pcap_freealldevs(alldevs);
            return -1;
        }
    } else {
        // Find the selected interface by name
        for (d = alldevs; d; d = d->next) {
            if (strcmp(d->name, interface_name) == 0)
                break;
        }
        
        if (!d) {
            fprintf(stderr, "Interface %s not found\n", interface_name);
            pcap_freealldevs(alldevs);
            return -1;
        }
    }
    
    
    // Open the device
    handle = pcap_open_live(d->name, 65536, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Could not open device %s: %s\n", d->name, errbuf);
        pcap_freealldevs(alldevs);
        return -1;
    }
    // We don't need the device list anymore
    pcap_freealldevs(alldevs);
    
    printf("网口:%d  俄钓端口:[%s\b]  \033[31m此网口/端口无数据包 (稍等2s...)\033[0m", interface_num, localPortString);
    
    read_data(FISHES_TXT, fish_list, &fish_list_len);
    read_data(INVALID_PACKET_TXT, invalid_packet_list, &invalid_list_len);

    // Start capturing packets
    pcap_loop(handle, 0, packet_handler, NULL);
    
    // Cleanup
    pcap_close(handle);
    
    return 0;
}