#define MAX_PACKET_FILTER_COUNT 256
#define MAX_DESCRIPTION_LENGTH 512
#define PROTO_TCP 6
#define FISHES_TXT "./data/fishes.txt"
#define INVALID_PACKET_TXT "./data/invalid_packet.txt"

// Ethernet header
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

// Structure to store fish information
typedef struct {
    char description[MAX_DESCRIPTION_LENGTH];
    int number;
} Packet_filter;

