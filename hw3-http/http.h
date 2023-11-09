#pragma once

#include <stdint.h>
#include <stddef.h>

#define     DEFAULT_HOST    "httpbin.org"
#define     DEFAULT_PORT    80
#define     DEFAULT_PATH    "/"

#define     HTTP_HEADER_EOL "\r\n"          //AS PER THE PDU Specifiction
#define     HTTP_HEADER_DELIM ':'

#define     CL_HEADER        "Content-Length"
#define     MAX_HEADER_LINE 512


//THE HTTP HEADER ENDS WITH \r\n\r\n.  This is because the last key:value pair ends with \r\n
//and the spec states the end of the headers is \r\n - so there will be 2 in a row
#define     HTTP_HEADER_END "\r\n\r\n"      

//Exported funcitons
int socket_connect(const char *host, uint16_t port);
int get_http_header_len(char *http_buff, int http_buff_len);
int get_http_content_len(char *http_buff, int http_buff_len);
int process_http_header(char *http_buff, int http_buff_len, int *header_len, int *content_len);
void print_header(char *http_buff, int http_header_len);

//Utilities
char *strnstr(const char *s, const char *find, size_t slen);
char *strcasestr(const char *s, const char *find);
