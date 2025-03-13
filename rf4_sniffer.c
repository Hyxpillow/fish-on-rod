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

int rf4_token_captured = 0;
char* wavList[3]  = {0};
u_int local_port = 0;
u_int64 packet_count = 0;
u_int last_seq = 0;
u_int last_data_length = 0;
u_char dec_buffer[65535];
int dec_buffer_offset = 0;
int to_be_decrypt_count = 0;
int to_be_print_count = 0;

FILE *file_ptr = NULL;
char filename[64];

struct _rc4 {
    u_char s_box[256];
    int i;
    int j;
} rc4;

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


void KSA(u_char* key, u_int key_length) {
    int r8 = 0;
    int r9 = 0;
    int tmp = 0;

    while (r8 < 256) {
        r9 = (r9 + rc4.s_box[r8] + key[r8 % key_length]) % 256;
        tmp = rc4.s_box[r8];
        rc4.s_box[r8] = rc4.s_box[r9];
        rc4.s_box[r9] = tmp;
        r8++;
    }
}

u_char _PRGA() {
    int tmp_idx = 0;
    u_char tmp = 0;

    rc4.i = (rc4.i + 1) % 256;
    rc4.j = (rc4.j + rc4.s_box[rc4.i]) % 256;
    tmp = rc4.s_box[rc4.i];
    rc4.s_box[rc4.i] = rc4.s_box[rc4.j];
    rc4.s_box[rc4.j] = tmp;

    tmp_idx = (rc4.s_box[rc4.i] + rc4.s_box[rc4.j]) % 256;
    return rc4.s_box[tmp_idx];
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

    tcp_header *tcp_hdr = (tcp_header *)(packet + eth_header_len + ip_header_len);
    int tcp_header_len = ((tcp_hdr->data_off & 0xF0) >> 4) * 4;
    int push_flag = (tcp_hdr->flags & 0x08) >> 3;
    int data_length = ntohs(ip_hdr->tlen) - ip_header_len - tcp_header_len;
    u_char *raw_data = (u_char *)(packet + eth_header_len + ip_header_len + tcp_header_len);

    if (!rf4_token_captured) {  //未找到俄钓起始数据包
        if (data_length > 6 && *(u_int*)(raw_data + 2) == data_length - 6) {
            rf4_token_captured = 1; // 找到了
            KSA(raw_data + 6, *(u_int*)(raw_data + 2));// S盒初始置换
            local_port = tcp_hdr->sport;
            printf("俄钓启动..\n");
        }
    } else { //已找到俄钓起始数据包
        // 匹配接受的俄钓数据包
        if (tcp_hdr->dport != local_port) return; // 端口不对
        u_int seq = ntohl(tcp_hdr->seq);
        if (seq <= last_seq && last_seq - seq < 0x7FFFFF && last_data_length == data_length) return;  // 重传包不要
        last_seq = seq;
        last_data_length = data_length;
        if (data_length == 0) {return;}
        if (packet_count == 0) {packet_count = 1; return;}
        if (data_length <= 13) {return;}


        int raw_data_offset = 0;
        printf("seq:%u len:%u\n", seq, data_length);
        while(raw_data_offset < data_length) {
            if (to_be_decrypt_count > 0) {
                u_char decrypted_byte = raw_data[raw_data_offset] ^ _PRGA();

                dec_buffer[dec_buffer_offset] = decrypted_byte;
                dec_buffer_offset++;

                if (file_ptr != NULL) {
                    fwrite(&decrypted_byte, sizeof(u_char), 1, file_ptr);
                }

                to_be_decrypt_count--;
                raw_data_offset++;
                
                if (to_be_decrypt_count == 0) {
                    printf("    >> decrypt data:%x\n", *(u_int*)dec_buffer);
                    // 如果文件指针打开，关闭它
                    if (file_ptr != NULL) {
                        fclose(file_ptr);
                        file_ptr = NULL;
                    }
                }
            } else {
                to_be_decrypt_count = *(u_int*)(raw_data + raw_data_offset) - 9;
                raw_data_offset += 13;
                if (to_be_decrypt_count > 0) {
                    dec_buffer_offset = 0;

                    if (to_be_decrypt_count > 600) {
                        snprintf(filename, sizeof(filename), "./log/%u", seq);
                        file_ptr = fopen(filename, "wb+");
                    } else {
                        file_ptr = NULL;
                    }
                }
            }
        }
    }

    // static int log_line_count = 0;

    // time_t t;
    // struct tm *tm_info;
    // time(&t);
    // tm_info = localtime(&t);
    // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), font_color_list[log_line_count % 4]);
    // log_line_count++;
    // printf("No.%d  %02d:%02d:%02d  Packet:%4d  ", log_line_count, tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,data_length);
    // printf("\n");
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

    for (int i = 0; i < 256; i++) {
        rc4.s_box[i] = i;
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
    int interface_num;
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
    
    // Start capturing packets
    pcap_loop(handle, 0, packet_handler, NULL);
    
    // Cleanup
    pcap_close(handle);
    return 0;
}