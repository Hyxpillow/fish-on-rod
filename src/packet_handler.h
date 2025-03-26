#pragma once
#include <pcap.h>
#include <winsock2.h>
#include <vector>
#include <unordered_map>
#include "rc4.h"
#include "rf4_fish_name.h"
#include "ui.h"
#include "pro_micro.h"
#include <iostream>
#include <random>
#include <ctime>
#include <fstream>

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


std::vector<char> wav;
bool drop_that_fish = 0;
u_int hotKey[3];
static int total_data_count;
extern int dev_choice;
extern char dev_description[256];

class packet_parser {
protected:
    u_int expect_seq;
    u_int64 packet_count;
    std::unordered_map<u_int, std::vector<u_char>> future_packet_table;
    int to_be_decrypt_count;
    RC4 rc4;
    bool rf4_in_process;
    u_int64 valid_data_count;
    u_int port;

    int parser_state;
    u_char buffer[33554432];
    int buffer_offset;

    std::random_device rd;
    std::mt19937 gen;
    std::normal_distribution<double> distribution;
    WCHAR cellText[256];

    const char* fishname_single;
    float fish_weight_single;

    const char* fishname_multi[3];
    float fish_weight_multi[3];

public:
    virtual bool sub_filter(tcp_header *tcp_hdr, int data_length, u_char* raw_data) = 0;
    void filter(tcp_header *tcp_hdr, int data_length, u_char* raw_data) {
        if (!sub_filter(tcp_hdr, data_length, raw_data)) return;
        if (tcp_hdr->dport != port) return; 
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
        parse_single_tcp(raw_data, data_length);
        while (future_packet_table.find(expect_seq) != future_packet_table.end()) { // 检查未来包列表
            std::vector<u_char> fp = future_packet_table[expect_seq];
            parse_single_tcp(fp.data(), fp.size());
            future_packet_table.erase(expect_seq);
            expect_seq += fp.size();
        }
    }
    void parse_single_tcp(u_char* raw_data, u_int size) {
        packet_count += 1;
        if (packet_count == 1) return;
        for (int offset = 0; offset < size; offset++) {
            switch (parser_state) {
            case 1:
                buffer[buffer_offset] = raw_data[offset];
                buffer_offset++;
                if (buffer_offset == 4) {
                    if (*(u_int*)(buffer) == 0) {
                        buffer_offset = 0;
                        break;
                    }
                    to_be_decrypt_count = *(u_int*)(buffer) - 9;
                    buffer_offset = 0;
                    parser_state = 2;
                }
                break;
            case 2:
                buffer_offset++;
                if (buffer_offset == 9) {
                    buffer_offset = 0;
                    if (to_be_decrypt_count > 0) {
                        parser_state = 3;
                    } else {
                        parser_state = 1;
                    }
                }
                break;
            case 3:
                buffer[buffer_offset] = raw_data[offset] ^ rc4.PRGA();
                buffer_offset++;
                if (buffer_offset == to_be_decrypt_count) {
                    rf4_parser(buffer, buffer_offset);
                    buffer_offset = 0;
                    parser_state = 1;
                }
                break;
            default:
                break;
            }
        }
    }
    
    void init() {
        swprintf(cellText, 256, L"等待连接至服务器... (网口:%d.%s)", dev_choice, dev_description);
        UpdateStatus(cellText, 0);
        swprintf(cellText, 256, L"--");
        UpdateStatus(cellText, 2);
        UpdateStatus(cellText, 1);
        rf4_in_process = false;
        port = 0;
        expect_seq = 0;
        packet_count = 0;
        future_packet_table.clear();
        to_be_decrypt_count = 0;
        valid_data_count = 0;
        total_data_count = 0;
        parser_state = 1;
        buffer_offset = 0;
        rc4.reset();
        gen = std::mt19937(std::time(0));
        UI_reset();
        for (int i = 0; i < 3; i++) {
            hotKey[i] = 0;
        }
    }
    
    virtual void rf4_parser(u_char* buffer, u_int size) = 0;
};


class packet_parser_local : public  packet_parser {
private:
    void save_packet(u_char* buffer, int size, u_int opcode) {
        char filename[256];
        snprintf(filename, sizeof(filename), "./log/%lld_%08X_%d_local.log", total_data_count, opcode, size);
        FILE* file_ptr = fopen(filename, "wb+");
        fwrite(buffer, sizeof(u_char), size, file_ptr);
        fclose(file_ptr);
        file_ptr = NULL;
    }
    bool sent = false;
public:   
    bool sub_filter(tcp_header *tcp_hdr, int data_length, u_char* raw_data) override {
        if (rf4_in_process) {  //正在处理俄钓数据包
            return true;
        }
        if (data_length > 6 && *(u_int*)(raw_data + 2) == data_length - 6) {
            rf4_in_process = true; 
            port = tcp_hdr->dport;
            expect_seq = ntohl(tcp_hdr->seq) + data_length;
            rc4.KSA(raw_data + 6, *(u_int*)(raw_data + 2));// S盒初始置换
            UpdateStatus(L"已连接到服务器", 0);
        }
        return false;
    }
    void rf4_parser(u_char* buffer, u_int size) override {
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
            u_int rod_hash = *(u_int*)(buffer + 0x13);
            swprintf(cellText, 256, L"抛竿");
            UpdateTextSingle(cellText);
            for (int i = 0; i < 3; i++) if (hotKey[i] == rod_hash) UpdateTextMulti(i, cellText);
            sent = false;
        } else if (opcode == 0x03020E00) { // 入水
            u_int rod_hash = *(u_int*)(buffer + 0x13);
            swprintf(cellText, 256, L"入水");
            UpdateTextSingle(cellText);
            for (int i = 0; i < 3; i++) if (hotKey[i] == rod_hash) UpdateTextMulti(i, cellText);
        } else if (opcode == 0x03070E00) { // 在水中
            // float loc_x = *(float*)(buffer + 0x28);
            // float loc_y = *(float*)(buffer + 0x2c);
            // float loc_z = *(float*)(buffer + 0x30);
            if (sent) return;
            u_char loc_len = *(buffer + 0x34);
            u_char rod_state = *(buffer + 0x34 + loc_len + 7);
            float rod_out_line_length = *(float*)(buffer + 0x34 + loc_len + 21);

            if ((rod_state & 0x20) != 0 && isMenuChecked(ID_MARINE_BOTTOM_MOTION)) { // 沉底
                if (isMenuChecked(ID_SOUND_MARINE)) Beep(700, 500);
                try_send('2');
                sent = true;
            } else if (rod_out_line_length > meter && isMenuChecked(ID_MARINE_DISTANCE_MARKER)) { // 卡米轻抽
                if (isMenuChecked(ID_SOUND_MARINE)) Beep(700, 500);
                try_send('2');
                sent = true;
            }
        } else if (opcode == 0x03040E00) { // 收杆
            u_int rod_hash = *(u_int*)(buffer + 0x13);
            swprintf(cellText, 256, L"收杆");
            UpdateTextSingle(cellText);
            UpdateColorSingle(0);
            for (int i = 0; i < 3; i++) 
                if (hotKey[i] == rod_hash) {
                    UpdateTextMulti(i, cellText);
                    UpdateColorMulti(i, 0);
                }

            if (!isMenuChecked(ID_PROMICRO_AUTO)) {
                try_send('1');
            } else if (drop_that_fish) {
                try_send('7'); // 丢弃
            } else {
                try_send('6'); // 入户
            }
            sent = false;
        } else if (opcode == 0x03050E00) { // 入户
        } else if (opcode == 0x030B0E00) { // 咬钩
            try_send('3');
            if (isMenuChecked(ID_SOUND_FISH_HOOKED)) {
                PlaySound(wav.data(), NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
            }
        } else if (opcode == 0x03080E00) { // 拉鱼
            float rod_out_line_length = *(float*)(buffer + 0x74);
            if (rod_out_line_length < 8) {
                try_send('5');
            }
        } else if (opcode == 0x030C0E00) { // 滑口
            u_int rod_hash = *(u_int*)(buffer + 0x13);
            swprintf(cellText, 256, L"滑口了 (%s, %.3fkg)", fishname_single, fish_weight_single);
            UpdateTextSingle(cellText);
            UpdateColorSingle(0);
            for (int i = 0; i < 3; i++) 
                if (hotKey[i] == rod_hash) {
                    swprintf(cellText, 256, L"滑口了 (%s, %.3fkg)", fishname_multi[i], fish_weight_multi[i]);
                    UpdateTextMulti(i, cellText);
                    UpdateColorMulti(i, 0);
                }
            try_send('4');
        }
    }

};

class packet_parser_remote : public packet_parser {
private:
    void save_packet(u_char* buffer, int size, u_int opcode) {
        char filename[256];
        snprintf(filename, sizeof(filename), "./log/%lld_%08X_%d_remote.log", total_data_count, opcode, size);
        FILE* file_ptr = fopen(filename, "wb+");
        fwrite(buffer, sizeof(u_char), size, file_ptr);
        fclose(file_ptr);
        file_ptr = NULL;
    }
public: 
    bool sub_filter(tcp_header *tcp_hdr, int data_length, u_char* raw_data) override {
        if (rf4_in_process) {  //正在处理俄钓数据包
            return true;
        }
        if (data_length > 6 && *(u_int*)(raw_data + 2) == data_length - 6) {
            rf4_in_process = true; 
            port = tcp_hdr->sport;
            expect_seq = ntohl(tcp_hdr->ack_seq);
            rc4.KSA(raw_data + 6, *(u_int*)(raw_data + 2));// S盒初始置换
            UpdateStatus(L"已连接到服务器", 0);
        }
        return false;
    }
    void rf4_parser(u_char* buffer, u_int size) override {
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
            u_int rod_hash = *(u_int*)(buffer + 0x13);

            u_char fish_name_len = buffer[0x38];
            char fishname[64];
            memcpy(fishname, buffer + 0x39, fish_name_len);
            fishname[fish_name_len] = 0;
            
            u_char fish_quality = *(buffer + 57 + fish_name_len);
            float fish_weight = *(float*)(buffer + 62 + fish_name_len);
            
            const char* fishname_cn = "未知";
            if (fish_table.find(fishname) != fish_table.end()) {
                const Fish_Data& fish = fish_table[fishname];
                fishname_cn = fish.name; // CN
            }
            if (fish_quality == 1) {
                drop_that_fish = true;
                swprintf(cellText, 256, L"小卡拉米 (%s, %.3fkg)", fishname_cn, fish_weight);
            } else {
                drop_that_fish = false;
                swprintf(cellText, 256, L"电视鱼 (%s, %.3fkg)", fishname_cn, fish_weight);
            }
            fish_weight_single = fish_weight;
            fishname_single = fishname_cn;
            UpdateTextSingle(cellText);
            UpdateColorSingle(1);
            for (int i = 0; i < 3; i++) 
                if (hotKey[i] == rod_hash) {
                    fish_weight_multi[i] = fish_weight;
                    fishname_multi[i] = fishname_cn;
                    UpdateTextMulti(i, cellText);
                    UpdateColorMulti(i, 1);
                }

            if (isMenuChecked(ID_SOUND_FISH_SPAWN)) {
                PlaySound(wav.data(), NULL, SND_MEMORY | SND_ASYNC | SND_NOSTOP);
            }
        } else if (opcode == 0x00880100) {
            u_char shortcut = *(buffer + 0x0E);
            u_int rod_hash1 = *(u_int*)(buffer + 0x10);
            int rod_short_cut = shortcut;
            if (rod_short_cut != 1 && rod_short_cut != 2 && rod_short_cut != 3) return;

            if (rod_hash1 == 0) { //移除竿子
                hotKey[rod_short_cut - 1] = 0;
                swprintf(cellText, 256, L"[空]");
                UpdateTextMulti(rod_short_cut - 1, cellText);
                UpdateColorMulti(rod_short_cut - 1, 0);
            } else { //设置竿子快捷键
                hotKey[rod_short_cut - 1] = rod_hash1;
                swprintf(cellText, 256, L"就绪");
                UpdateTextMulti(rod_short_cut - 1, cellText);
                UpdateColorMulti(rod_short_cut - 1, 0);
            }
        }
    }
};

static packet_parser_local local_parser;
static packet_parser_remote remote_parser;

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

    remote_parser.filter(tcp_hdr, data_length, raw_data);
    local_parser.filter(tcp_hdr, data_length, raw_data);
}


