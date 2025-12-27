#ifndef __UDP_ECHO_H__
#define __UDP_ECHO_H__

#include<stdint.h>          //for uint types
#include<unistd.h>          //for ssize_t

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 1234
#define DEFAULT_CLIENT_ADDR "127.0.0.1"
#define DEFAULT_SERVER_ADDR "0.0.0.0"
#define BACKLOG 5
#include "protocol.h"

#define MAX_MSG_DATA_SIZE (BUFFER_SIZE - sizeof(crypto_pdu_t))

// PDU structure for network messages (same as UDP version)
typedef struct {
    uint16_t msg_len;     // Length of msg_data in network byte order
    uint8_t  msg_data[1]; // Variable length message data (use as flexible array)
} echo_pdu_t;


// Function prototypes

void print_usage(const char* program_name);
void signal_handler(int sig);
void client_signal_handler(int sig);
ssize_t send_all(int sockfd, const char* buffer, size_t length);
int netmsg_from_cstr(const char *msg_str, uint8_t *msg_buff, uint16_t msg_buff_sz);
int extract_msg_data(const uint8_t *pdu_buff, uint16_t pdu_len, char *msg_str, uint16_t max_str_len);




#endif