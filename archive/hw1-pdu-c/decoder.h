#pragma once

#include "packet.h"

#include<stdbool.h>

//solution
void decode_raw_packet(uint8_t *packet, uint64_t packet_len);

bool check_ip_for_icmp(ip_packet_t *ip);
icmp_packet_t *process_icmp(ip_packet_t *ip);
icmp_echo_packet_t *process_icmp_echo(icmp_packet_t *icmp);
bool is_icmp_echo(icmp_packet_t *icmp);
void print_icmp_echo(icmp_echo_packet_t *icmp_packet);
void print_icmp_payload(uint8_t *payload, uint16_t payload_size);

//void process_arp(arp_packet_t *arp);
arp_packet_t *process_arp(raw_packet_t raw_packet);
void print_arp(arp_packet_t *arp);




