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
#include "rf4_fish_name.h"
#include "rf4_packet_list.h"
#include "rf4_rod_type.h"


char* wavList[3] = {0};
int interface_num;
pcap_if_t *alldevs, *d;
char logname[64] = "./log/error.log";
wchar_t* status_normal = L"正常";
wchar_t* status_obnormal = L"失败";


int rf4_token_captured = 0;
u_int local_port;
u_int packet_count;
u_char dec_buffer[33554432];
int dec_buffer_offset;
int to_be_decrypt_count;
int to_be_print_count;
u_int expect_seq;
List* future_packets;
int fish_count;

FILE *file_ptr = NULL;
FILE *log_ptr = NULL;

wchar_t* status_current;
wchar_t title[256];

struct _rc4 {
    u_char s_box[256];
    int i;
    int j;
} rc4;

void init_sniffer() {
    rf4_token_captured = 0;
    local_port = 0;
    packet_count = 0;
    dec_buffer_offset = 0;
    to_be_decrypt_count = 0;
    to_be_print_count = 0;
    expect_seq = 0;
    fish_count = 0;

    status_current = status_normal;
    for (int i = 0; i < 256; i++) {
        rc4.s_box[i] = i;
        rc4.i = 0;
        rc4.j = 0;
    }

    list_destroy(future_packets);
    future_packets = list_create();

    if (log_ptr) fclose(log_ptr);
    log_ptr = fopen(logname, "w");

    system("cls");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
    printf("网口:%d.%s   \033[31m 等待连接至服务器(小退重连)... \033[0m", interface_num, d->description);
}


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

void save_packet(int size, u_int opcode) {
    char filename[64];
    snprintf(filename, sizeof(filename), "./log/%08x", opcode);
    CreateDirectoryA(filename, NULL);
    snprintf(filename, sizeof(filename), "./log/%08x/%u_%d.log", opcode, expect_seq  - size, size);
    file_ptr = fopen(filename, "wb+");
    fwrite(dec_buffer, sizeof(u_char), dec_buffer_offset, file_ptr);
    fclose(file_ptr);
    file_ptr = NULL;
}

void parse_rod_packet() {
    u_int shortcut = *(u_int*)(dec_buffer + 0x0C);
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);

    int rod_shortcut = 0;
    if (shortcut == 0x010000) {
        rod_shortcut = 1;
    } else if (shortcut == 0x020000) {
        rod_shortcut = 2;
    } else if (shortcut == 0x030000) {
        rod_shortcut = 3;
    } else {
        return;
    }

    u_int rod_hash1 = *(u_int*)(dec_buffer + 0x10);
    if (rod_hash1 != 0) {
        printf("%02d:%02d:%02d  设置%d号竿 [%08X]\n",tm_info->tm_hour,tm_info->tm_min, tm_info->tm_sec, rod_shortcut, rod_hash1);
        set_rod_shortcut(rod_hash1, rod_shortcut);
    } else {
        printf("%02d:%02d:%02d  移除%d号竿\n",tm_info->tm_hour,tm_info->tm_min, tm_info->tm_sec, rod_shortcut);
    }
}

void parse_fish_packet() {
    u_int rod_hash1 = *(u_int*)(dec_buffer + 0x13);

    int rod = get_rod_shortcut(rod_hash1);
    int rod_color = 0x0F;
    if (rod == 1) {
        rod_color = 0x0C;
    } else if (rod == 2) {
        rod_color = 0x0B;
    } else if (rod == 3) {
        rod_color = 0x0E;
    }

    u_char fish_name_len = dec_buffer[0x38];
    char fishname[64];
    memcpy(fishname, dec_buffer + 0x39, fish_name_len);
    fishname[fish_name_len] = 0;

    float fish_weight = *(float*)(dec_buffer + 62 + fish_name_len);

    struct Fish_Data *fish_data = get_fish_data(fishname);

    const char* to_be_print_fish_name = "未知";
    int back_color = 0x00;
    int font_color = 0x0F;
    if (fish_data) {
        to_be_print_fish_name = fish_data->name;
        if ((double)fish_weight * 1000 >= (double)fish_data->trophy) {back_color = 0xE0; font_color = 0x00;}
        if ((double)fish_weight * 1000 >= (double)fish_data->super_trophy) back_color = 0x90;
    }
    
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), back_color | font_color);
    printf("%02d:%02d:%02d    ",
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec);

    if (get_rod_type(rod_hash1) != 3) { // 如果不是底部钓组 就响
        PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    } else {
        rod_color = font_color; //如果是底部钓组，不显示钓竿颜色
        printf("等待咬钩    ");
    }
    

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), back_color | rod_color);
    printf("[%d号竿]", rod);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), back_color | font_color);
    printf("    %.3f kg    %s\n", 
        fish_weight,
        to_be_print_fish_name);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

void parse_fish_on_rod_packet() {
    u_int rod_hash1 = *(u_int*)(dec_buffer + 0x2B);

    if (get_rod_type(rod_hash1) == 3) { // 如果是底部钓组 响并打印
        PlaySound(wavList[rand() % 3], NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
    } else {
        return; // 如果是其他钓组，
    }

    int rod = get_rod_shortcut(rod_hash1);
    int rod_color = 0x0F;
    if (rod == 1) {
        rod_color = 0x0C;
    } else if (rod == 2) {
        rod_color = 0x0B;
    } else if (rod == 3) {
        rod_color = 0x0E;
    }

    

    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    printf("%02d:%02d:%02d    ",
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), rod_color);
    printf("[%d号竿] 鱼咬钩了\n", rod);
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
}

void parse_rod_into_water_packet() {
    u_int rod_hash1;
    u_int oprand = *(u_int*)(dec_buffer + 0x14);
    if (oprand == 0x00950100) {
        rod_hash1 = *(u_int*)(dec_buffer + 0x218);
    } else if (oprand == 0x009D0100) {
        rod_hash1 = *(u_int*)(dec_buffer + 0x30);
    } else {
       return; 
    }  
    // 如果竿子hash没见过
    
    int rod_type = get_rod_type(rod_hash1);
    if (rod_type != 0) return;

    for (int i = 0; i < dec_buffer_offset - 1; i++) {
        if (dec_buffer[i] == 0) dec_buffer[i] = 1;
    }
    dec_buffer[dec_buffer_offset - 1] = 0;

    if (strstr(dec_buffer, "Float")) {
        rod_type = 1;
    } else if (strstr(dec_buffer, "Lure")) {
        rod_type = 2;
    } else if (strstr(dec_buffer, "Bottom")) {
        rod_type = 3;
    } else if (strstr(dec_buffer, "Marine")) {
        rod_type = 4;
    }
    set_rod_type(rod_hash1, rod_type);
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    printf("%02d:%02d:%02d  ",
        tm_info->tm_hour,
        tm_info->tm_min,
        tm_info->tm_sec);
    if (rod_type == 1) {
        printf("鱼竿[%08X]类型为[浮子]\n", rod_hash1);
    } else if (rod_type == 2) {
        printf("鱼竿[%08X]类型为[路亚]\n", rod_hash1);
    } else if (rod_type == 3) {
        printf("鱼竿[%08X]类型为[水底]\n", rod_hash1);
    } else if (rod_type == 4) {
        printf("鱼竿[%08X]类型为[海钓]\n", rod_hash1);
    } else {
        printf("鱼竿[%08X]类型为[未知钓法]\n", rod_hash1);
    }
}

void parse_single_packet(u_char* buffer, u_int size) {
    packet_count += 1;
    if (packet_count == 1) return;
    swprintf(title, 256, L"状态:%s 总计:%d", status_current, packet_count & 0xFFFF);
    SetConsoleTitleW(title);
    int offset = 0;
    // if (size > 13) {
    //     fprintf(log_ptr, "seq:%u len:%u\n", expect_seq - size, size);
    // }
    while(offset < size) {
        if (to_be_decrypt_count > 0) {
            u_char decrypted_byte = buffer[offset] ^ _PRGA();

            dec_buffer[dec_buffer_offset] = decrypted_byte;
            dec_buffer_offset++;

            to_be_decrypt_count--;
            offset++;
            
            if (to_be_decrypt_count == 0) {
                fprintf(log_ptr, "seq:%u len:%u\n", expect_seq - size, size);
                // fprintf(log_ptr, "    >> decrypt data:%x size:%d\n", *(u_int*)dec_buffer, dec_buffer_offset);
                if ((*(u_short*)(dec_buffer + 2)) == (u_short)0xFFFF) {
                    status_current = status_normal;
                } else {
                    status_current = status_obnormal;
                    // 打印log
                }
                u_int opcode = *(u_int*)(dec_buffer + 8);
                save_packet(size, opcode);
                if (opcode == 0x030E0E00) { //刷新鱼    
                    parse_fish_packet(); 
                } else if (opcode == 0x00880100) { //快捷键
                    parse_rod_packet(); 
                } else if (opcode == 0x03160400) {  
                    u_int oprand1 = *(u_int*)(dec_buffer + 12);
                    u_int oprand2 = *(u_int*)(dec_buffer + 16);
                    if (oprand1 == 0x01323602 && oprand2 == 0x00830100) { //鱼咬钩
                        parse_fish_on_rod_packet();
                    } else { // 手抛窝子

                    }
                } else if (opcode == 0x03200100) { // 切换棒子 装配件 打窝杆入水
                    u_int oprand = *(u_int*)(dec_buffer + 0x14);
                    if (oprand == 0x00950100 || oprand == 0x009D0100) { //杆子入水
                        parse_rod_into_water_packet(); 
                    }
                }
                fprintf(log_ptr, "seq:%u len:%u\n", expect_seq - size, size);
            }
        } else {
            to_be_decrypt_count = *(u_int*)(buffer + offset) - 9;
            offset += 13;
            if (to_be_decrypt_count > 0) {
                // fprintf(log_ptr, "  >> malloc size:%d\n", to_be_decrypt_count);
                dec_buffer_offset = 0;
            }
        }
    }
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
    // int push_flag = (tcp_hdr->flags & 0x08) >> 3;
    int fin_flag = tcp_hdr->flags & 0x01;
    int data_length = ntohs(ip_hdr->tlen) - ip_header_len - tcp_header_len;
    u_char *raw_data = (u_char *)(packet + eth_header_len + ip_header_len + tcp_header_len);

    if (!rf4_token_captured) {  //未找到俄钓起始数据包
        if (data_length > 6 && *(u_int*)(raw_data + 2) == data_length - 6) {
            rf4_token_captured = 1; // 找到了
            KSA(raw_data + 6, *(u_int*)(raw_data + 2));// S盒初始置换
            local_port = tcp_hdr->sport;
            expect_seq = ntohl(tcp_hdr->ack_seq);
            printf("\r网口:%d   本地端口:%d     \033[32m 已连接到服务器 \033[0m                                           \n", interface_num, ntohs(local_port));
        }
    } else { //已找到俄钓起始数据包
        // 匹配接受的俄钓数据包
        if (tcp_hdr->dport != local_port) return; // 端口不对
        if (fin_flag) {
            init_sniffer(); 
            return;
        }
        if (data_length == 0) {return;}

        u_int seq = ntohl(tcp_hdr->seq);
        if (seq < expect_seq && expect_seq - seq < (u_int)0x7F7F7F7F) return;  // 重传包不要
        if (seq > expect_seq && seq - expect_seq < (u_int)0x7F7F7F7F) { // 未来包
            // fprintf(log_ptr, "!!!!收到未来包 seq:%u expect:%u\n", seq, expect_seq);
            list_insert(future_packets, seq, data_length, raw_data);
            return;
        }
        expect_seq = seq + data_length;
        parse_single_packet(raw_data, data_length);

        Node* node = NULL;
        while (node = list_search(future_packets, expect_seq)) { // 检查未来包列表
            expect_seq = node->seq + node->size;
            parse_single_packet(node->buffer, node->size);
            list_delete(future_packets, node);
        }
    }

}

int main(int argc, char **argv) {
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    srand(time(NULL));  // 设置随机种子
    wavList[0] = load_wav_file("./resource/1.wav");
    wavList[1] = load_wav_file("./resource/2.wav");
    wavList[2] = load_wav_file("./resource/3.wav");


    setlocale(LC_ALL, "en_US.UTF-8");

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
    // pcap_freealldevs(alldevs);
    
    init_fish_table();

    init_sniffer();

    // Start capturing packets
    pcap_loop(handle, 0, packet_handler, NULL);
    
    // Cleanup
    pcap_close(handle);
    return 0;
}