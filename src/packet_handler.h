#pragma once
#include <pcap.h>
#include <winsock2.h>
#include <vector>
#include <unordered_map>
#include "rc4.h"
#include "rf4_parser.h"
#include "ui.h"

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

static bool rf4_in_process;
static u_int local_port;
static u_int remote_port;
static u_int expect_seq;
static u_int64 packet_count;
static std::unordered_map<u_int, std::vector<u_char>> future_packet_table;
static u_char dec_buffer[33554432];
static int dec_buffer_offset;
static int to_be_decrypt_count;
static int to_be_print_count;
extern int dev_choice;
extern char dev_description[256];

void packet_handler(u_char *user_data, const struct pcap_pkthdr *pkthdr, const u_char *packet);
void parse_single_packet(u_char* buffer, u_int size);

void init_sniffer();