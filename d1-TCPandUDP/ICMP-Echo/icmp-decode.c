#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include "packet.h"
#include "nethelper.h"
#include "icmp-decode.h"

//This is where you will be putting your captured network frames for testing.
//Before you do your own, please test with the ones that I provided as samples:
#include "testframe.h"


int main(int argc, char **argv) {
    uint8_t *test_packet_icmp = raw_packet_icmp_frame362;
    uint8_t *test_packet_arp = raw_packet_arp_frame78;
    
    printf("STARTING...");

        printf("\n--------------------------------------------------\n");
        printf("TESTING A NEW PACKET (SHOULD BE ICMP-ECHO)\n");
        printf("--------------------------------------------------\n");

        decode_raw_packet(test_packet_icmp);

        printf("\n--------------------------------------------------\n");
        printf("TESTING A NEW PACKET (IS ARP AND NOT ICMP-ECHO)\n");
        printf("--------------------------------------------------\n");

        decode_raw_packet(test_packet_arp);
    

    printf("\n\nDONE\n");
}

bool decode_raw_packet(uint8_t *packet){

    //Everything we are doing starts with the ethernet PDU at the
    //front.  The below code projects an ethernet_pdu structure 
    //POINTER onto the front of the buffer so we can decode it.
    struct ether_pdu *p = (struct ether_pdu *)packet;
    uint16_t ft = ntohs(p->frame_type);

    printf("Detected raw frame type from ethernet header: 0x%04x\n", ft);

    if (ft != IP4_PTYPE){
        printf("Looking for ICMP packet, IP expected but not found\n\n");
        print_common_eth_frame_types();
        return false;
    }

    printf("\nFrame type = IPv4, what addresses?\n");

    //We know its IP, so lets type the raw packet as an IP packet
    ip_pdu_t *ip_pdu = (ip_pdu_t *)(packet + sizeof(ether_pdu_t));
    char ip_addr_buffer[16]; //ip address string aaa.bbb.ccc.ddd\0 = 16 bytes

    ip_toStr(ip_pdu->source_address,ip_addr_buffer,sizeof(ip_addr_buffer));
    printf("Packet Src IP Address: %s\n", ip_addr_buffer);
    ip_toStr(ip_pdu->destination_address,ip_addr_buffer,sizeof(ip_addr_buffer));
    printf("Packet Dest IP Address: %s\n", ip_addr_buffer);

    printf("\nWith frame being IPv4, lets check if its ICMP?\n");

    //Now check the IP packet to see if its payload is an ICMP packet
    uint8_t next_proto = ip_pdu->protocol;
    if (next_proto != ICMP_PTYPE){
        printf("Expected next protocol to be ICMP, but it was instead 0x%04x\n", 
            next_proto);
        return false;
    }

    //ICMP Has many protocol subtypes, so we need to next check if its an
    //Echo ICMP type.  Dont fall for this frequent nasty C bug, notice
    //both items below mean the same thing.  
    //icmp_pdu_t *icmp_pdu = (icmp_pdu_t *)(ip_pdu + 1);
    icmp_pdu_t *icmp_pdu = (icmp_pdu_t *)((uint8_t *)ip_pdu + sizeof(ip_pdu_t));

    uint8_t icmp_type = icmp_pdu->type;
    printf("ICMP Type %d\n", icmp_type);

    //Its an echo if the type is either an ECHO_REQ or and ECHO_RESP
    if ((icmp_type != ICMP_ECHO_REQUEST ) && (icmp_type != ICMP_ECHO_RESPONSE)){
        printf("Error: Expected an ECHO REQUEST or an ECHO response, but got 0x%x\n",
            icmp_type);
        return false;
    }

    //ICMP Has many protocol subtypes, so we need to next check if its an
    //Echo ICMP type, note icmp_echo_pdu is just an icmp_pdu with extra stuff
    //at end
    icmp_echo_pdu_t *icmp_echo_pdu = (icmp_echo_pdu_t *)icmp_pdu;

    print_icmp_echo(icmp_echo_pdu, ip_pdu);

    printf("\nOOPS - forgot about endianess...\n\n");

    icmp_echo_pdu->sequence = ntohs(icmp_echo_pdu->sequence);
    icmp_echo_pdu->id = ntohs(icmp_echo_pdu->id);
    icmp_echo_pdu->timestamp = ntohl(icmp_echo_pdu->timestamp);
    icmp_echo_pdu->timestamp_ms = ntohl(icmp_echo_pdu->timestamp_ms);
    icmp_echo_pdu->icmp_hdr.checksum = ntohs(icmp_echo_pdu->icmp_hdr.checksum);
    

    print_icmp_echo(icmp_echo_pdu, ip_pdu);

    return true;
}


void print_icmp_echo(icmp_echo_pdu_t *icmp_pdu, ip_pdu_t *ip_pdu){
    //Step 1: Figure out ICMP size.  Notice the PDU has an unknown lenght
    //byte array as the last value. AKA uint8_t icmp_payload[];
    //dont forget endianess of total_len
    uint16_t icmp_len = ntohs(ip_pdu->total_length) - sizeof(ip_pdu_t);
    uint16_t payload_size = icmp_len - sizeof(icmp_echo_pdu_t);

    printf("ICMP PACKET DETAILS \n \
    type:\t0x%02x \n \
    checksum:\t0x%04x \n \
    id:\t0x%04x \n \
    sequence:\t0x%04x \n \
    timestamp:\t0x%08x-%08x \n \
    payload:\t%d bytes \n \
    ",
    icmp_pdu->icmp_hdr.type,
    icmp_pdu->icmp_hdr.checksum,
    icmp_pdu->id,
    icmp_pdu->sequence,
    icmp_pdu->timestamp,
    icmp_pdu->timestamp_ms,
    payload_size);

    char *echo_ts = get_ts_formatted(icmp_pdu->timestamp, 
        icmp_pdu->timestamp_ms);

    printf("ECHO Timestamp: %s\n", echo_ts);

    print_icmp_payload(icmp_pdu->icmp_payload, payload_size);

}


/*
 *  This function pretty prints the icmp_echo_packet payload.  You can be
 *  creative here, but try to make it look nice.  Below is an example of 
 *  how I printed it.  You basically do this by looping trough each
 *  byte in the payload.  Below, I set the line length to 16.  So, as we
 *  loop through the array with an index (lets call this "i"), with a 
 *  line_len = 16 we do the following:
 *  
 *  if (i % line_length) == 0 then we have a new line, write offset which is
 *      the loop index i
 * 
 *  we next write the element at buffer[i]
 * 
 *  if (i % line_lenght) == (line_lenght - 1) then we write a newline "\n"
 * 
 *  You dont have to make it look exactly like I made my solution shown below
 *  but it should look nice :-)
 * 
 * PAYLOAD
 *
 * OFFSET | CONTENTS
 * -------------------------------------------------------
 * 0x0000 | 0x08  0x09  0x0a  0x0b  0x0c  0x0d  0x0e  0x0f  
 * 0x0008 | 0x10  0x11  0x12  0x13  0x14  0x15  0x16  0x17  
 * 0x0010 | 0x18  0x19  0x1a  0x1b  0x1c  0x1d  0x1e  0x1f  
 * 0x0018 | 0x20  0x21  0x22  0x23  0x24  0x25  0x26  0x27  
 * 0x0020 | 0x28  0x29  0x2a  0x2b  0x2c  0x2d  0x2e  0x2f  
 * 0x0028 | 0x30  0x31  0x32  0x33  0x34  0x35  0x36  0x37  
 */
void print_icmp_payload(uint8_t *payload, uint16_t payload_size) {

    int numElementsPerLine = 8;

    printf("\nPAYLOAD\n");
    printf("\nOFFSET | CONTENTS\n");
    printf("-------------------------------------------------------\n");

    for (int i = 0; i < payload_size; i++){
        if ((i % numElementsPerLine) == 0)
            printf("0x%04x | ", i);

        printf("0x%02x  ", payload[i]);

        if ((i % numElementsPerLine) == numElementsPerLine-1)
            printf("\n");
    }
}

void print_common_eth_frame_types() {
    printf("\n=== COMMON ETHERNET FRAME TYPES ===\n\n");
    
    printf("Network Layer Protocols:\n");
    printf("  0x%04X - IPv4 (Internet Protocol version 4) - most common\n", 0x0800);
    printf("  0x%04X - IPv6 (Internet Protocol version 6) - increasingly common\n", 0x86DD);
    printf("  0x%04X - ARP (Address Resolution Protocol) - maps IP to MAC addresses\n", 0x0806);
    printf("  0x%04X - RARP (Reverse ARP) - maps MAC to IP addresses (rarely used)\n", 0x8035);
    
    printf("\nVLAN and QoS:\n");
    printf("  0x%04X - VLAN-tagged frame (802.1Q) - very common in enterprise\n", 0x8100);
    printf("  0x%04X - QinQ/Service VLAN (802.1ad) - provider bridging\n", 0x88A8);
    printf("  0x%04X - Legacy QinQ\n", 0x9100);
    
    printf("\nSecurity and VPN:\n");
    printf("  0x%04X - MAC Security (MACsec/802.1AE) - layer 2 encryption\n", 0x88E5);
    printf("  0x%04X - EAPoL (EAP over LAN/802.1X) - network access control\n", 0x888E);
    
    printf("\nDiscovery and Management:\n");
    printf("  0x%04X - LLDP (Link Layer Discovery Protocol) - topology discovery\n", 0x88CC);
    printf("  0x%04X - PTP (Precision Time Protocol/802.1AS) - time sync\n", 0x88F7);
    
    printf("\nLegacy/Specialized:\n");
    printf("  0x%04X - IPX (Internetwork Packet Exchange) - old Novell networks\n", 0x8137);
    printf("  0x%04X - AppleTalk - legacy Apple networking\n", 0x809B);
    printf("  0x%04X - MPLS unicast - service provider networks\n", 0x8847);
    printf("  0x%04X - MPLS multicast\n", 0x8848);
    
    printf("\nNote: Values > 1500 (0x05DC) indicate length field for 802.3 frames\n");
    printf("      IPv4 (0x0800) comprises ~95%% of modern network traffic\n\n");
}