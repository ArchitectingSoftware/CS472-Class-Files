#include "c-tutorial.h"

#include <sys/socket.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>

void testTheTestStruct(){
    ip_pdu_t ip_req;
    ip_pdu_t ip_resp;

    ip_req.flags = FIN_FLAG;
    ip_resp.flags = FIN_ACK;

    //I want to see if the actual flag is FIN_ACK?
    if( (ip_resp.flags & FIN_FLAG) == (FIN_FLAG)) {
        printf("FIN BIT is set\n");
    }
    printf("Response from other side is: 0x%02x\n", ip_resp.flags);
}

int main(int argc, char *argv[]) {
    output("Hello World");
    tcpPDUPlayground();
    testTheTestStruct();
}

void output(char *message) {
    //Know the various formatting messages, we often print out
    //hex

    uint8_t val = 0xAB;
    printf("%s\n", message);
    printf("Hex demo: %04x, %08x\n", val, val);
}



void tcpPDUPlayground(){
    tcp_pdu_t pdu;

    //Initialize the PDU
    memset(&pdu, 0, sizeof(pdu));

    //since this is directly on the stack, we can just directly
    //access the fields with a "."
    pdu.source_port = 0x1234;
    pdu.destination_port = 0x5678;
    pdu.sequence_number = 0x12345678;
    pdu.acknowledgement_number = 0x87654321;
    pdu.flags = 0x12;
    pdu.window_size = 0x1234;
    pdu.checksum = 0x1234;
    pdu.urgent_pointer = 0x1234;

    printf("TCP Flags 0x%02x\n", pdu.flags);
}

