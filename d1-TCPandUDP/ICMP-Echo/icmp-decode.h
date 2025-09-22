#pragma once

#include "packet.h"

#include<stdbool.h>

//solution
bool decode_raw_packet(uint8_t *packet);
void print_icmp_echo(icmp_echo_pdu_t *icmp_pdu, ip_pdu_t *ip_pdu);
void print_icmp_payload(uint8_t *payload, uint16_t payload_size);
void print_common_eth_frame_types();