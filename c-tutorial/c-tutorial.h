//Questions why this?
//#pragma once

#ifndef C_TUTORIAL_H_INCLUDED
#define C_TUTORIAL_H_INCLUDED

#include <stdint.h>

//type aliases
typedef uint16_t ube16_t;
typedef uint32_t ube32_t;
typedef uint8_t  raw_packet_t[];

/* 
 * Sizes and helper types for various network addresses. For example IP addresses
 * in (IPv4) are 32 bits or 4 bytes, MAC addresses are 6 bytes (48 bits)
 */
#define ETH_ALEN        6       /* Ethernet MAC addresses are 6 octects - 48 bytes */
#define IP4_ALEN        4       /* Ethernet MAC addresses are 4 octets - 32 bytes */
typedef uint8_t  ipaddress_t[IP4_ALEN];
typedef uint8_t  macaddress_t[ETH_ALEN];


//flags

typedef struct test_struct{
    uint8_t filed1;
    uint8_t filed2;
}test_struct_t;

//structs
typedef struct ip_pdu{
  uint8_t version_ihl;
  uint8_t type_of_service;
  ube16_t total_length;
  ube16_t identification;
  uint8_t flags;
  //whats this?
  uint32_t fragment_offset:24;
  uint8_t time_to_live;
  uint8_t protocol;
  ube16_t header_checksum;
  uint8_t source_address[IP4_ALEN];
  uint8_t destination_address[IP4_ALEN];
} ip_pdu_t;

#define FIN_FLAG 0x01   // 00000001
#define SYN_FLAG 0x02   // 00000010
#define PSH_FLAG 0x04   // 00000100
#define RST_FLAG 0x08   // 00001000
#define ACK_FLAG 0x10   // 00010000
#define URG_FLAG 0x20   // 00100000
#define ECE_FLAG 0x40   // 01000000
#define CWR_FLAG 0x80   // 10000000 

#define FIN_ACK  FIN_FLAG | ACK_FLAG    // 00010001

//which ones do we need to worry about with network byte order?
typedef struct tcp_pdu {
  uint16_t source_port;
  uint16_t destination_port;
  uint32_t sequence_number;
  uint32_t acknowledgement_number;
  uint8_t data_offset;
  uint8_t reserved;
  uint8_t flags;
  uint16_t window_size;
  uint16_t checksum;
  uint16_t urgent_pointer;
}tcp_pdu_t;

//prototypes
void output(char *);
void tcpPDUPlayground();
void testTheTestStruct();

#endif // C_TUTORIAL_H_INCLUDED