#pragma once

#include <stdint.h>
#include <arpa/inet.h>

//----------------------------TYPES AND CONSTANTS--------------------------//
/* 
 * Special type used to document that the data is sent in network
 * byte order - AKA Big Endian - and might need to be converted
 * to little endian based on your processor architecture.  Become
 * familiar with ntohs() and htons() from #include <arpa/inet.h>
 * to properly convert fields typed with ube16_t to work with you
 * machine. 
 * 
 * Note to convert from "N"etwork byte order to "H" byte order 
 * the #include <arpa/inet.h> header has a number of helpers:
 * 
 * Convert from network to host byte order:
 *     ntohs - Network to Host Short (16 bit)
 *     ntohl - Network to Host Long (32 bit)
 * 
 *  Note there are also functions hton[s,l,be64] that go the other
 *  way. 
 */

/*
 * ANY ATTRIBUTE TAGGED BELOW WITH ube16_6 or ube32_t IS PROVIDED AS DOCUMENTATION
 * SO THAT YOU KNOW THAT THIS FIELD IS CAPTURED IN NETWORK, or BIG ENDIAN BYTE ORDER
 * THUS, TO HANDLE IT IN YOUR PROGRAM YOU WILL BE RESPONSIBLE TO CONVERT THIS VALUE
 * INTO YOUR HOST MACHINES BYTE ORDER, WHICH IN JUST ABOUT EVERY CASE WILL BE 
 * LITTLE ENDIAN - From above ntohs() and ntohl() WILL BE YOUR FRIENDS
 */
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

//----------------------------------PROTOCOLS--------------------------------------//

//                                  ETHERNET
/*
 *  With TCP/IP networking the most common data link transport is ethernet, it is
 *  what we will be using in wired and wireless networks.  The Ethernet PDU contains
 *  the source and destination MAC address, and a 16 bit indicator to highlight the
 *  Type of the encapsulated payload that comes next.
 * 
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                      Source MAC Address                       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Source MAC Address (Continued)|    Destination MAC Address    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Frame Type            |   START OF DATA ------------->
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *  Thus the Ethenet PDU contains 14 bytes of data, with the Frame Type specifying
 *  the format of how to interpret the data that comes after it.  See "Start of Data"
 *  at Byte 15 above
 */

/* 
 * These values are defined in various RFCs to help decode if the protocol is
 * is IP4, ARP, or ICMP.  They specify if the "Start of Data" is an IP Frame or
 * an ARP request/response
 */
#define IP4_PTYPE       0x0800  /* Internet Protocol packet */
#define ARP_PTYPE       0x0806  /* Address Resolution packet */


/* Ethernet frame header - aka PDU */
typedef struct ether_pdu{
   uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
   uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
   ube16_t frame_type;          /* Ethernet frame type */
} ether_pdu_t;


//                     ARP - Address Resolution Protocol
/*
 *  The address resolution protocol is designed to to query MAC and IP
 *  addresses on the local link network - we will talk about this in class.
 * 
 *  The format of the protocol is defined in the arp_pdu struct below.  For
 *  this assignment the htype will always be ARP_HTYPE_ETHER since we are
 *  assuming we are running over ethernet and ptype will always be ARP_PTYPE_IPV4
 *  since we are using ARP with IPv4.  More to come later, but ARP was discontinued
 *  in IPv6.  The "op" field in the structure indicates if this is an ARP request
 *  aka query, or an ARP reply which is a response to a query. 
 */
#define ARP_HTYPE_ETHER 1       /* Ethernet ARP type */
#define ARP_PTYPE_IPV4  0x0800  /* We are using ARP with IPv4 */
#define ARP_REQ_OP      1       /* From RFC - Request Op - 1 */
#define ARP_RSP_OP      2       /* From RFC - Response Op - 2 */

/* Ethernet ARP PDU from RFC 826 */
typedef struct arp_pdu{
   ube16_t htype;           /* Format of hardware address */
   ube16_t ptype;           /* Format of protocol address */
   uint8_t hlen;            /* Length of hardware address */
   uint8_t plen;            /* Length of protocol address */
   ube16_t op;              /* ARP opcode (command) */
   uint8_t sha[ETH_ALEN];   /* Sender hardware address */
   uint8_t spa[IP4_ALEN];   /* Sender IP address */
   uint8_t tha[ETH_ALEN];   /* Target hardware address */
   uint8_t tpa[IP4_ALEN];   /* Target IP address */
} arp_pdu_t;


//                               Internet Protocol (IP)

/*
 *  The IP PDU is shown below and encode in the struct ip_pdu.  There is a lot
 *  going on in this structure.  We will be covering it in class in a lot of 
 *  detail.  For this assignment IP will be encapsulating an ICMP ECHO or "Ping"
 *  request.   The only fields you will need to deal with are the "total length" and
 *  the "Protocol field".
 * 
 *  The "Total Length" 16 bit value is the total lenght of the IP PDU itself plus
 *  All of the trailing data.  Given the IP header itself (below) is 20 bytes, the
 *  size of the data that comes after the PDU can be determined by taking the total
 *  length field and subtracting sizeof(ip_pdu_t).  If you havent thought about it
 *  the maximum size can be 64K (because total length is 16 bits), and the maximum
 *  size of the data is 64K-20 bytes because of the IP header itself
 * 
 *  We will see lots of usage of IP in this class, however, the only other field you
 *  will need to deal with in this assignment is "Protocol".  This 8 byte field 
 *  specifies the protocol of the data that comes after the PDU.  IP can manage many
 *  different protocols, with TCP being the most popular.  However, for this assignment
 *  we will be looking for Protocol = 1, which indicates that the trailing data is 
 *  ICMP (Internet Control Message Protocol)
 * 
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |Version|  IHL  |Type of Service|          Total Length         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |         Identification        |Flags|      Fragment Offset    |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Time to Live |    Protocol   |         Header Checksum       |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Source Address                          |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                    Destination Address                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | START OF IP DATA .............................................>   
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#define ICMP_PTYPE      0x01    /* ICMP packet */

typedef struct ip_pdu{
  uint8_t version_ihl;
  uint8_t type_of_service;
  ube16_t total_length;
  ube16_t identification;
  ube16_t flags_and_fragment_offset;
  uint8_t time_to_live;
  uint8_t protocol;
  ube16_t header_checksum;
  uint8_t source_address[IP4_ALEN];
  uint8_t destination_address[IP4_ALEN];
} ip_pdu_t;

//                                      ICMP

/*
 *
 * 
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     TYPE      |      CODE     |            CHECKSUM           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  ICMP Content (This varies based on TYPE) ....................>
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 *  FOR A PING or Echo Request Type  = 0x08 and Code = 0x00 
 *  FOR A PING or Echo Response Type = 0x00 and Code = 0x00
 * 
 *  The ICMP Content for an Echo Request takes the following form:
 * 
 *   0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                ID             |           SEQUENCE            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                   ICMP TIMESTAMP (64 bits)                    |
 *  |  Timestamp is number of milliseconds since the Epoch 1/1/1970 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | ICMP Payload (Generated Data shipped back and forth) .........>
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 *  For this assignment we will only be considering ECHO requests thats
 *  why the PDU is named icmp_echo_pdu.  For different types of ICMP messages
 *  the fields after the "checksum" will vary.  These are the correct
 *  ones for the ECHO request and reply. 
 */
#define ICMP_ECHO_REQUEST   0x08
#define ICMP_ECHO_RESPONSE  0x00

typedef struct icmp_pdu{
  uint8_t type;
  uint8_t code;
  ube16_t checksum;
} icmp_pdu_t;

//An icmp_echo_pdu is starts with the basic icmp_pdu and then is followed
//by a number of other fields shown below
typedef struct icmp_echo_pdu{
  icmp_pdu_t icmp_hdr;
  ube16_t id;
  ube16_t sequence;
  ube32_t timestamp;
  ube32_t timestamp_ms;
} icmp_echo_pdu_t;


//----------------------------PACKETS--------------------------//

/*
 *   A "Packet" is a generalized term to indicate a complete sequence of bytes
 *   that are sent over the network that perform some sort of meaningful operation
 * 
 *   For this assignment we will be considering 3 different types of packets:
 * 
 *   1) An ARP packet, which has an ethernet PDU + ARP PDU
 *   2) An IP Packet, which has an ethernet PDU + IP PDU
 *   3) AN ICMP Echo Packet, which has an ethernet PDU + IP PDU + ICMP Echo PDU
 */

/* ARP Packet = ethernet PDU + arp PDU*/
typedef struct arp_packet{
    ether_pdu_t eth_hdr;
    arp_pdu_t   arp_hdr;
} arp_packet_t;

/* IP Packet = ethernet PDU + IP PDU*/
typedef struct ip_packet{
    ether_pdu_t eth_hdr;
    ip_pdu_t    ip_hdr;
} ip_packet_t;

/* ICMP Packet = ethernet PDU + IP PDU + ICMP PDU (ECHO) + ICMP PAYLOAD           */
/* You can ignore the __attribute__((packed)) for now  - we will discuss in class */
/* ------------------------------------------------------------------------------ */
/* ALSO, note that almost all modern implementations of PING that generate the 
 * ICMP Echo packet, create a 64 bit timestamp as the first part of the payload.
 * In order to avoid having to deal with 64 bit values in network byte order the 
 * following structure breaks apart the timestamp into its 2 foundational parts. 
 * The first part is the timestamp itself, that has Month, Day, Year, Hour, Min, Second
 * And the second part is the fraction of a second where the packet was captured
 * I name this field timestamp_ms, becuase most of the time its captured in
 * milliconds, but some machines might capture in micro or nanoseconds.  Its should
 * just be considered as "the fraction of a second".
*/
typedef struct __attribute__((packed)) icmp_packet {
    ip_packet_t ip;
    icmp_pdu_t  icmp_hdr;
} icmp_packet_t;

typedef struct __attribute__((packed)) icmp_echo_packet {
    ip_packet_t ip;
    icmp_echo_pdu_t icmp_echo_hdr;
    uint8_t icmp_payload[];
} icmp_echo_packet_t;

/* 
 * Helper to get the icmp_payload[] length.  This is calculated from the
 * total length of the IP packet. Its in ip_hdr.total_length.  You take this
 * and then subtract the size of the IP PDU.  This gives the total size of the
 * ICMP portion.  You then subtract the fixed size of the ICMP PDU.  What is
 * left is the reamining varible data that comes with the echo request packet.
 * This data will be addressable by the "icmp_payload" buffer shown above.  Note
 * This is not a zero/null terminated C string, so you must be careful when 
 * managing it by respecting its length!
 * 
 */
#define ICMP_Payload_Size(icmp) ntohs(icmp->ip.ip_hdr.total_length) - \
    sizeof(ip_pdu_t) - sizeof(icmp_echo_pdu_t)


