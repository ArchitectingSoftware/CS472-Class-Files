#pragma once

#include <stdint.h>

uint16_t str_toMAC(const char *src, uint8_t *dst, int len);
uint16_t str_toIP(const char *src, uint8_t *dst, int len);

int16_t mac_toStr(uint8_t *mac, char *dst, int len);
uint16_t ip_toStr(uint8_t *ip, char *dst, int len);

char *get_ts_formatted(uint32_t ts, uint32_t ts_ms);

static uint16_t
str_toByteBuff (const char *src, uint8_t *dst,  const char *delims, 
    uint8_t buff_word_count, int base);