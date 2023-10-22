/*

Protocol Format
---------------
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | PROTO |  VER  |  CMD  |DIR|ATM|     AY: Academic Year         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             CC1: Course Code (First 4 Chars)                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       CC2: Course Code (Last 3 Chars)         |LEN: Msg Length|
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     MSG: Message Data*                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Field Values
------------
PROTO  Protocol Name  CS472-FUN Protocol is the only allowed value with 0x1
VER    Protocol Version Current Version is 0x1
CMD    Protocol Command - Allowed Values are:
           - 0x0: Get Class Info
           - 0x1: Ping my Pong
DIR    Direction - 0x0 for Send and 0x1 for Response
ATM    Academic Term 0 for Fall, 1 for Winter, 2 for Spring, 3 for summer
AY     Academic Year, Unsigned 16 bit number of the start of the year, we are
       in AY 2022
CC1    First 4 characters of the course code, each character is an ASCII 
       Character value - so for CS472 - this would include CS47
CC2    Last 3 characters of the course code.  Pad with ZEROs, so for CS472
       this would be 2\0\0
LEN    The length of the message that follows the header in bytes, up to
       250 on a request, becuase a response will add PONG:, which is 6
       characters resulting in a max of 255 characters. We will leave space
       for the null to help with C strings thus a total of 256 max

*/

#ifndef CS472PROTO_H_INCLUDED
#define CS472PROTO_H_INCLUDED

#include <stdint.h>

#define SOCKET_NAME "/tmp/cs472-fun.sock"        //For Unix Sockets

#define PORT_NUM        1080                     //Lets use port 1080 for TCP
#define NULL 0

#define PROTO_CS_FUN    0x1
#define PROTO_VER_1     0x1
#define CMD_CLASS_INFO  0x0
#define CMD_PING_PONG   0x1
#define DIR_SEND        0x0
#define DIR_RECV        0x1
#define TERM_FALL       0x0
#define TERM_WINTER     0x1
#define TERM_SPRING     0x2
#define TERM_SUMMER     0x3

#define MAX_MSG_SIZE    250
#define MAX_MSG_BUFFER  256

//This is the header data structure, if you have never seen
//something like this before in C, read up on bit-fields
//makes doing these things a lot easier.
typedef struct cs472_proto_header_t{
    uint32_t proto : 4, 
             ver   : 4,
             cmd   : 4,
             dir   : 2,
             atm   : 2,
             ay    :16;
    char     course[7];
    uint8_t len;
} cs472_proto_header_t;

//Prototypes for the helper functions
//
void print_proto_header(cs472_proto_header_t *h);
uint16_t  prepare_req_packet(cs472_proto_header_t *header, 
            uint8_t *payload, uint8_t pay_length, uint8_t *packet,
            uint16_t packet_len );
uint8_t  process_recv_packet(cs472_proto_header_t *header, 
            uint8_t *buffer, uint8_t **msg, uint8_t *msgLen );
#endif