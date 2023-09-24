#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "nethelper.h"
#include "packet.h"

/*
 * This helper function takes an ip address in a 4 byte uint8_t and
 * returns a formatted string.  For example:
 *   ip = [10, 220. 44. 18]  will return
 *   dst = "10.220.44.18"
 * 
 * Note you must provide/allocate the space for dst, and provide the
 * lenght of the buffer.  The max IP string length is 16. Each component
 * of an IP address will be a string representation of a number between
 * "0" and "255".  Each of the components is seperated by a ".".  Also,
 * This returns a C string so we need space for a "\0" at the end. 
 * 
 * This returns 1 for success and -1 for an error
 */
uint16_t ip_toStr(uint8_t *ip, char *dst, int len) {
    //note max len is 15 plus add null byte 255.255.255.255\0
    if( len < 16) return -1;

    sprintf(dst,"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]); 
    return 1;
}

/*
 * This function takes an IP formatted string and returns via dst
 * a 4 byte array containing the IP address
 */
uint16_t str_toIP(const char *src, uint8_t *dst, int len) {
    //note max len must be at least 4 bytes because an IP address requires 4 bytes
    if( len <  4) return -1;

    uint8_t ipBuff[IP4_ALEN];
    str_toByteBuff(src, ipBuff, ".", IP4_ALEN, 10);
    memcpy (dst, ipBuff, sizeof(ipBuff));
    return 1;
}

/*
 *  Takes a mac address formatted string as follows:
 *      01:02:03:04:05:06   or
 *      01-02-03-04-05-06
 * 
 *  and returns the mac address into a 6 byte array via dst
 */
uint16_t str_toMAC(const char *src, uint8_t *dst, int len) {
    //Note that a mac address takes 6 bytes, so dst must have at least 6 bytes allocated
    if (len < 6) return -1;
    return str_toByteBuff(src, dst, ":-", ETH_ALEN, 16);
}

/*
 * Parses in a binary mac address, and convert it to a string, copy the string into the dst
 * buffer.  Notice the check for 18 - thats because MAC strings are 17 characters and in
 * C you need an extra byte for the null
 */
int16_t mac_toStr(uint8_t *mac, char *dst, int len){
    //note max len is 17 plus add null byte 00-00-00-00-00-00\0
    if( len < 18) return -1;

    sprintf(dst,"%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 1;
}

/*
 * This function takes a 64 bit timestamp in two parts:
 *    ts is the standard linux epoch 32 bit number (seconds since 1/1/1970)
 *    ts_ms is the fractional part of a second in larger timestamps
 * 
 *  Most of the time if ts_ms is provided its in milliseconds but on some
 *  machines it can be milli- or nano-seconds. 
 */
char *get_ts_formatted(uint32_t ts, uint32_t ts_ms){
    static char buff[50];

    time_t datetime = ts;
    struct tm *lt  = localtime(&datetime);

    char fmt_buffer[26];
    strftime(fmt_buffer, 26, "%Y-%m-%d %H:%M:%S", lt);
    sprintf(buff, "TS = %s.%lu\n", fmt_buffer, ts_ms);

    return buff;
}

/*
 * This is a generic function that helps with some of the above.  Specificlly converting
 * IP and MAC address strings into numberic byte arrays.  This function accepts:
 *      src:    Input C string
 *      dst:    A "byte" array for the result
 *      delims: A set of delimiters to parse src, for example "." for IP addresses 
 *              and ":" for MAC addresses
 *      buff_word_count: is the size of the *dst array
 *      base:  The output base.  Note IP addresses use base 10, and MAC addresses use
 *              base 16 (hex) typically
 */
static uint16_t 
str_toByteBuff (const char *src, uint8_t *dst,  const char *delims, uint8_t buff_word_count, int base)
{
    char *token;
    //uint16_t buff_bytes = buff_word_count * 2;
    uint8_t tmp[buff_word_count];
    uint8_t resultBuff[buff_word_count];
    int octet = 0;
    uint8_t octet_val = 0;

    //dup so we dont destroy what was passed in
    int src_len = strlen(src);
    char srcIP[src_len+1];
    strcpy((char *)&srcIP,src);

    unsigned long ulTemp;
    
    token = strtok(srcIP, delims);
    /* walk through other tokens */
    while( token != NULL ) {
      ulTemp = strtoul(token,NULL,base);
      if (ulTemp > 255){
        perror("number too big, larger than 255");
        return 0;
      }
      tmp[octet++] = (uint8_t)ulTemp;
      token = strtok(NULL, delims);
      if (octet > buff_word_count) {
        perror("misformed source address");
        return 0;
      }
    }

    for (int i=0; i < buff_word_count; i++){
        resultBuff[i] = tmp[i];
    }
  
    memcpy (dst, &tmp, sizeof(tmp));
    return 1;
}

