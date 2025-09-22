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
typedef struct __attribute__((packed)) ether_pdu{
   uint8_t dest_addr[ETH_ALEN]; /* Destination hardware address */
   uint8_t src_addr[ETH_ALEN];  /* Source hardware address */
   ube16_t frame_type;          /* Ethernet frame type */
} ether_pdu_t;


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

typedef struct __attribute__((packed)) ip_pdu{
  uint8_t version_ihl;
  uint8_t type_of_service;
  ube16_t total_length;
  ube16_t identification;
  uint8_t flags;
  uint8_t fragment_offset;
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

typedef struct __attribute__((packed)) icmp_pdu{
  uint8_t type;
  uint8_t code;
  ube16_t checksum;
} icmp_pdu_t;

//An icmp_echo_pdu is starts with the basic icmp_pdu and then is followed
//by a number of other fields shown below
typedef struct __attribute__((packed)) icmp_echo_pdu{
  icmp_pdu_t icmp_hdr;
  ube16_t id;
  ube16_t sequence;
  ube32_t timestamp;
  ube32_t timestamp_ms;
  uint8_t icmp_payload[];
} icmp_echo_pdu_t;
