#include "cs472-proto.h"
#include <stdio.h>
#include <string.h>

/*
 *  This helper prepares the request packet, it takes a number of parameters:
 *
 *      *header: This is the pointer to the packet header data sructure, this
 *               function fills in this structure
 *      *payload: A pointer to the payload data, note this will be null for
 *                a course lookup, or it will be a c string to the payload
 *                data if its a ping message
 *      pay_lenght:  The length of the payload data, basically 0 for
 *                  course lookup, or strlen(payload) for a ping
 *      *packet:  A pointer to a buffer to create the packet.  This will be
 *                allocated outside of this function and passed in
 *      packet_len:  this is the size of the packet buffer
 * 
 *  Try to follow good coding and make sure the buffer is big enough to
 *  hold the entire packet, return -1 if not, or return the ultimate size
 *  of the packet data.
 * 
 *      uint16_t packet_sz = sizeof(cs472_proto_header_t) + pay_length;
 *      if ( packet_sz > packet_len)
 *          return -1;
 *  
 */
uint16_t  prepare_req_packet(cs472_proto_header_t *header, 
            uint8_t *payload, uint8_t pay_length, uint8_t *packet,
            uint16_t packet_len ){

    uint16_t packet_sz = sizeof(cs472_proto_header_t) + pay_length;
    if ( packet_sz > packet_len)
        return -1;
    
    bzero(packet, packet_len);
    memcpy(packet, header, sizeof(cs472_proto_header_t));
    memcpy(packet + sizeof(cs472_proto_header_t), payload, pay_length);
    header->len = packet_sz;


    return packet_sz;
}

/*
 *  This helper processes a packet received and breaks it apart to make
 *  processing a bit easier
 *  
 */
uint8_t  process_recv_packet(cs472_proto_header_t *header, 
            uint8_t *buffer, uint8_t **msg, uint8_t *msgLen ){

    //memcpy(header, buffer, sizeof(cs472_proto_header_t));
    *msg = buffer + sizeof(cs472_proto_header_t);
    *msgLen = header->len - sizeof(cs472_proto_header_t);
    return *msgLen;
}


/*
 * Utility to print the header
 */
void print_proto_header(cs472_proto_header_t *h) {
    static char proto_def[16];
    static char proto_ver[16];
    static char proto_cmd[16];
    static char proto_dir[16];
    static char proto_atm[16];
    
    if (h->proto == PROTO_CS_FUN)
        sprintf(proto_def, "PROTO_CS_FUN");
    else    
        sprintf(proto_def, "BAD_PROTO: %d", h->ver);

    if (h->ver == PROTO_VER_1)
        sprintf(proto_ver, "VERSION_1");
    else    
        sprintf(proto_ver,"BAD_VER: %d", h->ver);

    switch(h->cmd) {
        case CMD_CLASS_INFO:
            sprintf(proto_cmd,"CMD_CLASS_INFO");
            break;
        case CMD_PING_PONG:
            sprintf(proto_cmd,"CMD_PING_PONG");
            break;
        default:
            sprintf(proto_cmd,"BAD_CMD %d",h->cmd);
            break;
    }
    switch(h->dir) {
        case CMD_CLASS_INFO:
            sprintf(proto_dir,"DIR_SEND");
            break;
        case CMD_PING_PONG:
            sprintf(proto_dir,"DIR_RECV");
            break;
        default:
            sprintf(proto_dir,"BAD_DIR %d",h->dir);
            break;
    }
    switch(h->atm) {
        case TERM_FALL:
            sprintf(proto_atm,"TERM_FALL");
            break;
        case TERM_WINTER:
            sprintf(proto_atm,"TERM_WINTER");
            break;
        case TERM_SPRING:
            sprintf(proto_atm,"TERM_SPRING");
            break;
        case TERM_SUMMER:
            sprintf(proto_atm,"TERM SUMMER");
            break;
        default:
            sprintf(proto_atm,"BAD TERM %d",h->atm);
            break;
    }

    printf("HEADER VALUES \n" 
    "  Proto Type:\t %s\n" 
    "  Proto Ver:\t %s\n"
    "  Command:\t %s\n"
    "  Direction:\t %s\n"
    "  Term:\t\t %s \n"
    "  Course:\t %s\n"
    "  Pkt Len:\t %d\n"
    "\n"
    , proto_def, proto_ver, proto_cmd, proto_dir, proto_atm, h->course, h->len);
}