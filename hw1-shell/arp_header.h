//#pragma once
#ifndef ARPHEADERS_H_INCLUDED
#define ARPHEADERS_H_INCLUDED

#include <stdint.h>

#define ETH_P_ARP 0x0806 /* Address Resolution packet */
#define ARP_HTYPE_ETHER 1  /* Ethernet ARP type */
#define ARP_PTYPE_IPv4 0x0800 /* Internet Protocol packet */
#define ETH_ALEN    6   /* Ethernet MAC addresses are 6 octects - 48 bytes */
#define IP4_ALEN    4   /* Ethernet MAC addresses are 4 octets - 32 bytes */
#define ARP_REQ_OP   1   /* From RFC - Request Op - 1 */
#define ARP_RSP_OP   2   /* From RFC - Response Op - 2 */

typedef uint32_t ipaddress_t;
typedef uint8_t  macaddress_t[ETH_ALEN];

/* Ethernet frame header */
typedef struct {
   uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
   uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
   uint16_t frame_type;   /* Ethernet frame type */
} ether_hdr;

/* Ethernet ARP packet from RFC 826 */
typedef struct arp_ether_ipv4{
   uint16_t htype;   /* Format of hardware address */
   uint16_t ptype;   /* Format of protocol address */
   uint8_t hlen;    /* Length of hardware address */
   uint8_t plen;    /* Length of protocol address */
   uint16_t op;    /* ARP opcode (command) */
   uint8_t sha[ETH_ALEN];  /* Sender hardware address */
   uint8_t spa[IP4_ALEN];   /* Sender IP address */
   uint8_t tha[ETH_ALEN];  /* Target hardware address */
   uint8_t tpa[IP4_ALEN];   /* Target IP address */
} arp_ether_ipv4;

#endif