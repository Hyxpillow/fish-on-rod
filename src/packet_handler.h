#pragma once
#include <pcap.h>
#include <winsock2.h>
#include <vector>
#include <unordered_map>
#include "rc4.h"
#include "rf4_fish_name.h"
#include "ui.h"
#include "pro_micro.h"

typedef struct {
    u_char dest_mac[6];
    u_char src_mac[6];
    u_short eth_type;
} ethernet_header;

typedef struct {
    u_short lo_magic_number;
    u_short padding;
} ethernet_header_lo;

// IP header
typedef struct {
    u_char  ver_ihl;        // Version (4 bits) + IP header length (4 bits)
    u_char  tos;            // Type of service 
    u_short tlen;           // Total length 
    u_short identification; // Identification
    u_short flags_fo;       // Flags (3 bits) + Fragment offset (13 bits)
    u_char  ttl;            // Time to live
    u_char  proto;          // Protocol
    u_short crc;            // Header checksum
    u_int   saddr;          // Source address
    u_int   daddr;          // Destination address
} ip_header;

// TCP header
typedef struct {
    u_short sport;          // Source port
    u_short dport;          // Destination port
    u_int   seq;            // Sequence number
    u_int   ack_seq;        // Acknowledgement number
    u_char  data_off;       // Data offset
    u_char  flags;          // Flags
    u_short window;         // Window size
    u_short checksum;       // Checksum
    u_short urg_ptr;        // Urgent pointer
} tcp_header;
void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet);



static bool rf4_in_process;
static int total_data_count;
extern int dev_choice;
extern char dev_description[256];

class packet_parser {
private:
    u_int expect_seq;
    u_int64 packet_count;
    std::unordered_map<u_int, std::vector<u_char>> future_packet_table;
    u_char dec_buffer[33554432];
    int dec_buffer_offset;
    int to_be_decrypt_count;
    int to_be_print_count;
    RC4 rc4;
public:
    u_int64 valid_data_count;
    u_int port;
    void filter(tcp_header *tcp_hdr, int data_length, u_char* raw_data) {
        int fin_flag = tcp_hdr->flags & 0x01;
        if (fin_flag) {init(); return;}
        if (data_length == 0) {return;}
        u_int seq = ntohl(tcp_hdr->seq);
        if (seq < expect_seq && expect_seq - seq < (u_int)0x7F7F7F7F) return;  // 重传包不要
        if (seq > expect_seq && seq - expect_seq < (u_int)0x7F7F7F7F) { // 未来包
            future_packet_table[seq] = std::vector<u_char>(raw_data, raw_data + data_length);
            return;
        }
        expect_seq = seq + data_length;
        parse_single_packet(raw_data, data_length);
        while (future_packet_table.find(expect_seq) != future_packet_table.end()) { // 检查未来包列表
            std::vector<u_char> fp = future_packet_table[expect_seq];
            parse_single_packet(fp.data(), fp.size());
            future_packet_table.erase(expect_seq);
            expect_seq += fp.size();
        }
    }
    void parse_single_packet(u_char* buffer, u_int size) {
        packet_count += 1;
        if (packet_count == 1) return;
        int offset = 0;
        while(offset < size) {
            if (to_be_decrypt_count > 0) {
                u_char decrypted_byte = buffer[offset] ^ rc4.PRGA();

                dec_buffer[dec_buffer_offset] = decrypted_byte;
                dec_buffer_offset++;

                to_be_decrypt_count--;
                offset++;
                
                if (to_be_decrypt_count == 0) {
                    rf4_parser(dec_buffer, dec_buffer_offset);
                }
            } else {
                to_be_decrypt_count = *(u_int*)(buffer + offset) - 9;
                offset += 13;
                if (to_be_decrypt_count > 0) {
                    dec_buffer_offset = 0;
                }
            }
        }
    }
    void load(u_int _port, u_int _expect_seq, unsigned char *key, unsigned int key_length) {
        port = _port;
        expect_seq = _expect_seq;
        rc4.KSA(key, key_length);// S盒初始置换
    }

    void init() {
        WCHAR cellText[512];
        swprintf(cellText, 512, L"等待连接至服务器... (网口:%d.%s)", dev_choice, dev_description);
        UpdateStatus(cellText, 0);
        swprintf(cellText, 512, L"--");
        UpdateStatus(cellText, 1);
        UpdateStatus(cellText, 2);
        rf4_in_process = false;
        port = 0;
        expect_seq = 0;
        packet_count = 0;
        future_packet_table.clear();
        dec_buffer_offset = 0;
        to_be_decrypt_count = 0;
        to_be_print_count = 0;
        valid_data_count = 0;
        rc4.reset();

        // if (wavList[0] == NULL) {
        //     wavList[0] = load_wav_file("./resource/1.wav");
        //     wavList[1] = load_wav_file("./resource/2.wav");
        //     wavList[2] = load_wav_file("./resource/3.wav");
        // }
        // rod_table.clear();
        UI_reset();
    }

    virtual void rf4_parser(u_char* buffer, u_int size) = 0;
};


class packet_parser_local : public  packet_parser {
private:
    void save_packet(u_char* buffer, int size, u_int opcode) {
        char filename[256];
        // snprintf(filename, sizeof(filename), "./log");
        // CreateDirectoryA(filename, NULL);
        snprintf(filename, sizeof(filename), "./log/%lld_%08X_%d_local.log", total_data_count, opcode, size);
        FILE* file_ptr = fopen(filename, "wb+");
        fwrite(buffer, sizeof(u_char), size, file_ptr);
        fclose(file_ptr);
        file_ptr = NULL;
    }
    char hw_state = 0;
public:   
    void rf4_parser(u_char* buffer, u_int size) override {
        WCHAR cellText[256];
        valid_data_count++;
        total_data_count++;
        if ((*(u_short*)(buffer + 2)) == (u_short)0xFFFF) {
            swprintf(cellText, 256, L"发送正常:%d", valid_data_count & 0xFFFF);
        } else {
            swprintf(cellText, 256, L"发送异常");
        }
        UpdateStatus(cellText, 2);
        u_int opcode = *(u_int*)(buffer + 8);
        // if (opcode != 0x031b1800) // 消息
        //     save_packet(buffer, size, opcode);
        if (opcode == 0x03010E00) {  // 抛竿
            swprintf(cellText, 256, L"抛竿");
            UpdateText(cellText);
        } else if (opcode == 0x03020E00) { // 入水
            swprintf(cellText, 256, L"入水");
            UpdateText(cellText);
        } else if (opcode == 0x03070E00) { // 在水中
            // float loc_x = *(float*)(buffer + 0x28);
            // float loc_y = *(float*)(buffer + 0x2c);
            // float loc_z = *(float*)(buffer + 0x30);
            u_char loc_len = *(buffer + 0x34);
            u_char rod_state = *(buffer + 0x34 + loc_len + 7);
            float rod_out_line_length = *(float*)(buffer + 0x34 + loc_len + 21);
            if ((rod_state & 0x20) != 0 && hw_state != 'B') {
                try_send('B'); // 开始抽动
                hw_state = 'B';
            }
            if ((rod_state & 0x21) == 0x21) { // 沉底轻抽
                swprintf(cellText, 256, L"底部的运动+轻微的抽停");
                UpdateText(cellText);
            } else if ((rod_state & 0x21) == 0x20) { // 沉底
                swprintf(cellText, 256, L"底部的运动");
                UpdateText(cellText);
            } else if ((rod_state & 0x21) == 0x01) { // 轻微的抽停
                swprintf(cellText, 256, L"轻微的抽停");
                UpdateText(cellText);
            }
        } else if (opcode == 0x03040E00) { // 收杆
            swprintf(cellText, 256, L"收杆");
            UpdateText(cellText);
            try_send('E');
            hw_state = 'E';
        } else if (opcode == 0x03050E00) { // 入户
            swprintf(cellText, 256, L"入户");
            UpdateText(cellText);
        } else if (opcode == 0x030B0E00) { // 鱼
            // swprintf(cellText, 256, L"本地咬钩");
            // UpdateText(cellText);
        } else if (opcode == 0x03080E00) { // 拉鱼
            float rod_out_line_length = *(float*)(buffer + 0x74);
            if (rod_out_line_length < 5 && hw_state != 'D') {
                try_send('D');
                hw_state = 'D';
            }
        } else if (opcode == 0x030C0E00) { // 滑口
            swprintf(cellText, 256, L"本地滑口");
            UpdateText(cellText);
            try_send('C');
            hw_state = 'C';
        }
            // 0x03090300 发送定时信息
            // 03131800 发送 确认消息
            // 03010E00 抛竿
            // 03020E00 入水？
            // 03030E00 
            // 03040E00 收杆
            // 03050E00 入户
            // 03060E00 
            // 03070E00 在水中 位置改变
            // 03080E00 拉鱼
            // 03090E00 
            // 030B0E00 上鱼
            // 030C0E00 滑口

            // 03010C00 打窝
    }

};

class packet_parser_remote : public  packet_parser {
private:
    void save_packet(u_char* buffer, int size, u_int opcode) {
        char filename[256];
        // snprintf(filename, sizeof(filename), "./log");
        // CreateDirectoryA(filename, NULL);
        snprintf(filename, sizeof(filename), "./log/%lld_%08X_%d_remote.log", total_data_count, opcode, size);
        FILE* file_ptr = fopen(filename, "wb+");
        fwrite(buffer, sizeof(u_char), size, file_ptr);
        fclose(file_ptr);
        file_ptr = NULL;
    }
public: 
    void rf4_parser(u_char* buffer, u_int size) override {
        WCHAR cellText[256];
        valid_data_count++;
        total_data_count++;
        if ((*(u_short*)(buffer + 2)) == (u_short)0xFFFF) {
            swprintf(cellText, 256, L"接收正常:%d", valid_data_count & 0xFFFF);
        } else {
            swprintf(cellText, 256, L"接收异常");
        }
        UpdateStatus(cellText, 1);
        u_int opcode = *(u_int*)(buffer + 8);
        // if (opcode != 0x03131800 && opcode != 0x03090300 && opcode != 0x03070E00) // 消息
        //     save_packet(buffer, size, opcode);
        if (opcode == 0x030E0E00) {
            u_int rod_hash1 = *(u_int*)(buffer + 0x13);

            u_char fish_name_len = buffer[0x38];
            char fishname[64];
            memcpy(fishname, buffer + 0x39, fish_name_len);
            fishname[fish_name_len] = 0;
        
            float fish_weight = *(float*)(buffer + 62 + fish_name_len);
            
            const char* fishname_cn = "未知";
            if (fish_table.find(fishname) != fish_table.end()) {
                const Fish_Data& fish = fish_table[fishname];
                fishname_cn = fish.name; // CN
            }
            swprintf(cellText, 256, L"上鱼了 (%s, %.3fkg)", fishname_cn, fish_weight);
            UpdateText(cellText);
        } else if (opcode == 0x03160400) {  
            u_int oprand1 = *(u_int*)(buffer + 12);
            u_int oprand2 = *(u_int*)(buffer + 16);
            if (oprand1 == 0x01323602 && oprand2 == 0x00830100) { //鱼咬钩
                // swprintf(cellText, 256, L"远程咬钩");
                // UpdateText(cellText);
                try_send('A');
            }
        }
       
        // 01920100 切换场景
        // 03200100 吃东西
        // 031b1800 收消息
        // 37_3A9B0100_3528 开船
        // 009E0100 抛竿
        // 03210100 手杆
        // 03200100 鱼入户 切换棒子 装配件 打窝杆入水 竿子入水
        // 03010200 快照

        // if (opcode == 0x030E0E00) { //刷新鱼    
        //     parse_fish_packet(buffer, size); 
        // } else if (opcode == 0x00880100) { //快捷键
        //     parse_rod_packet(buffer, size); 
        // } else if (opcode == 0x03160400) {  
        //     u_int oprand1 = *(u_int*)(buffer + 12);
        //     u_int oprand2 = *(u_int*)(buffer + 16);
        //     if (oprand1 == 0x01323602 && oprand2 == 0x00830100) { //鱼咬钩
        //         parse_fish_on_rod_packet(buffer, size);
        //     } else { // 手抛窝子
        
        //     }
        
        // } else if (opcode == 0x03200100) { // 
        //     u_int oprand = *(u_int*)(buffer + 0x14);
        //     if (oprand == 0x00950100 || oprand == 0x009D0100) { //杆子入水
        //         parse_rod_into_water_packet(buffer, size); 
        //     }
        // } else if (opcode == 0x03210100) { // 收杆
        //     parse_rod_back_packet(buffer, size);
        // }
    }

};

static packet_parser_local local_parser;
static packet_parser_remote remote_parser;

// static char* wavList[3];
// char* load_wav_file(const char* filename);

// char* load_wav_file(const char* filename) {
//     size_t size = 0;
//     FILE* file = fopen(filename, "rb");
//     if (!file) {
//         printf("%s 不存在\n", filename);
//         system("pause");
//         exit(1);
//     };

//     fseek(file, 0, SEEK_END);
//     size = ftell(file);
//     rewind(file);

//     char* buffer = (char*)malloc(size + 1);
//     if (!buffer) {
//         fclose(file);
//         return NULL;
//     }

//     fread(buffer, 1, size, file);
//     buffer[size] = '\0'; // Null-terminate for safety if it's a text file

//     fclose(file);
//     return buffer;
// }

int dev_choice;
char dev_description[256];

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
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
    if (ip_hdr->proto != 6) return;

    tcp_header *tcp_hdr = (tcp_header *)(packet + eth_header_len + ip_header_len);
    int tcp_header_len = ((tcp_hdr->data_off & 0xF0) >> 4) * 4;
    int data_length = ntohs(ip_hdr->tlen) - ip_header_len - tcp_header_len;
    u_char *raw_data = (u_char *)(packet + eth_header_len + ip_header_len + tcp_header_len);

    if (!rf4_in_process) {  //未找到俄钓起始数据包
        if (data_length > 6 && *(u_int*)(raw_data + 2) == data_length - 6) {
            rf4_in_process = 1; 
            remote_parser.load(tcp_hdr->sport, ntohl(tcp_hdr->ack_seq), raw_data + 6, *(u_int*)(raw_data + 2));
            local_parser.load(tcp_hdr->dport, ntohl(tcp_hdr->seq) + data_length, raw_data + 6, *(u_int*)(raw_data + 2));
            UpdateStatus(L"已连接到服务器", 0);
        }
    } else { //已找到俄钓起始数据包
        if (tcp_hdr->dport == remote_parser.port) {
            remote_parser.filter(tcp_hdr, data_length, raw_data); 
        } else if (tcp_hdr->dport == local_parser.port) {
            local_parser.filter(tcp_hdr, data_length, raw_data);
        }
    }
}