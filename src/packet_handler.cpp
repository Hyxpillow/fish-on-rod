#include "packet_handler.h"

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
    int fin_flag = tcp_hdr->flags & 0x01;
    int data_length = ntohs(ip_hdr->tlen) - ip_header_len - tcp_header_len;
    u_char *raw_data = (u_char *)(packet + eth_header_len + ip_header_len + tcp_header_len);

    if (!rf4_in_process) {  //未找到俄钓起始数据包
        if (data_length > 6 && *(u_int*)(raw_data + 2) == data_length - 6) {
            rf4_in_process = 1; // 找到了
            KSA(raw_data + 6, *(u_int*)(raw_data + 2));// S盒初始置换
            local_port = tcp_hdr->sport;
            expect_seq = ntohl(tcp_hdr->ack_seq);
        }
    } else { //已找到俄钓起始数据包
        // 匹配接受的俄钓数据包
        if (tcp_hdr->dport != local_port) return; // 端口不对
        if (fin_flag) {init_sniffer(); return;}
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

}

void parse_single_packet(u_char* buffer, u_int size) {
    packet_count += 1;
    if (packet_count == 1) return;
    int offset = 0;
    while(offset < size) {
        if (to_be_decrypt_count > 0) {
            u_char decrypted_byte = buffer[offset] ^ PRGA();

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

void init_sniffer() {
    rf4_in_process = 0;
    local_port = 0;
    expect_seq = 0;
    packet_count = 0;
    future_packet_table.clear();
    dec_buffer_offset = 0;
    to_be_decrypt_count = 0;
    to_be_print_count = 0;
    RC4_reset();
    rf4_parser_init();
    
    WCHAR cellText[512];
    swprintf(cellText, 512, L"等待连接至服务器... (网口:%d.%s)", dev_choice, dev_description);
    UpdateStatus(cellText);

    SetCellColor(0, 0);
    SetCellColor(1, 0);
    SetCellColor(2, 0);
}