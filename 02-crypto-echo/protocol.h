#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define MSG_KEY_EXCHANGE        1
#define MSG_DATA                2
#define MSG_ENCRYPTED_DATA      3
#define MSG_DIG_SIGNATURE       4 
#define MSG_HELP_CMD            5
#define MSG_CMD_CLIENT_STOP     6
#define MSG_CMD_SERVER_STOP     7
#define MSG_ERROR               8
#define MSG_EXIT                9
#define MSG_SHUTDOWN           10


#define DIR_REQUEST  1
#define DIR_RESPONSE 2


#define RC_OK                0
#define RC_ENCRYPTION_ERR   -1
#define RC_DECRYPTION_ERR   -2
#define RC_MEMORY_ERR       -3

#define BUFFER_SIZE         1024
#define MAX_MSG_SIZE        BUFFER_SIZE
#define MAX_MSG_DATA_SIZE   (BUFFER_SIZE - sizeof(crypto_pdu_t))


#include <stdint.h>

typedef struct crypto_pdu {
    uint8_t  msg_type;      // Message type (MSG_*)
    uint8_t  direction;     // Direction (DIR_REQUEST or DIR_RESPONSE)
    uint16_t payload_len;   // Length of payload data
} crypto_pdu_t;

typedef struct crypto_msg {
    crypto_pdu_t header;    // PDU header
    uint8_t      payload[]; // Flexible array member for payload data
} crypto_msg_t;



#endif