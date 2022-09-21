#ifndef DECODER_H_INCLUDED
#define DECODER_H_INCLUDED

#include <stdint.h>
#include "arp_header.h"

//prototypes
static void bytesToArp(arp_ether_ipv4 *arp, u_int8_t *buff);
static void wordsToArp(arp_ether_ipv4 *arp, uint16_t *buff);
void  arp_toString(arp_ether_ipv4 *ap, char *dstStr, int len);

//helpers
int16_t mac_toStr(uint8_t *mac, char *dst, int len);
uint16_t ip_toStr(u_int8_t *ip, char *dst, int len);

#endif