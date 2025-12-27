#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdint.h>

#include "crypto-echo.h"
#include "crypto-lib.h"
#include "crypto-client.h"
#include "crypto-server.h"


// Global socket for signal handler
int server_sockfd = -1;
int client_sockfd = -1;
volatile int client_exit_requested = 0;



int main(int argc, char* argv[]) {
    int is_client = 0;
    int is_server = 0;
    int port = DEFAULT_PORT;
    char addr[INET_ADDRSTRLEN] = {0};
    
    // Set up signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--client") == 0) {
            is_client = 1;
            strcpy(addr, DEFAULT_CLIENT_ADDR);
        } else if (strcmp(argv[i], "--server") == 0) {
            is_server = 1;
            strcpy(addr, DEFAULT_SERVER_ADDR);
        } else if (strcmp(argv[i], "--port") == 0) {
            if (i + 1 < argc) {
                port = atoi(argv[++i]);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Error: Invalid port number %d\n", port);
                    exit(EXIT_FAILURE);
                }
            } else {
                fprintf(stderr, "Error: --port requires a value\n");
                exit(EXIT_FAILURE);
            }
        } else if (strcmp(argv[i], "--addr") == 0) {
            if (i + 1 < argc) {
                strncpy(addr, argv[++i], sizeof(addr) - 1);
                addr[sizeof(addr) - 1] = '\0';
            } else {
                fprintf(stderr, "Error: --addr requires a value\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    
    // Validate arguments
    if (!is_client && !is_server) {
        fprintf(stderr, "Error: Must specify either --client or --server\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (is_client && is_server) {
        fprintf(stderr, "Error: Cannot specify both --client and --server\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Set default address if not specified
    if (strlen(addr) == 0) {
        if (is_client) {
            strcpy(addr, DEFAULT_CLIENT_ADDR);
        } else {
            strcpy(addr, DEFAULT_SERVER_ADDR);
        }
    }
    
    // Start client or server
    if (is_client) {
        printf("Starting TCP client: connecting to %s:%d\n", addr, port);
        start_client(addr, port);
    } else {
        printf("Starting TCP server: binding to %s:%d\n", addr, port);
        start_server(addr, port);
    }
    
    return 0;
}




void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    
    if (client_sockfd != -1) {
        close(client_sockfd);
        client_sockfd = -1;
    }
    
    if (server_sockfd != -1) {
        close(server_sockfd);
        server_sockfd = -1;
    }
    
    exit(0);
}

void client_signal_handler(int sig) {
    printf("\nReceived signal %d, exiting client...\n", sig);
    client_exit_requested = 1;
    
    if (client_sockfd != -1) {
        close(client_sockfd);
        client_sockfd = -1;
    }
    
    exit(0);
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("OPTIONS:\n");
    printf("  --client              Run in client mode\n");
    printf("  --server              Run in server mode\n");
    printf("  --port <port>         Port number (default: %d)\n", DEFAULT_PORT);
    printf("  --addr <address>      IP address\n");
    printf("                        Client: server address (default: %s)\n", DEFAULT_CLIENT_ADDR);
    printf("                        Server: bind address (default: %s)\n", DEFAULT_SERVER_ADDR);
    printf("\nClient Usage:\n");
    printf("  Connect to server and type messages at the '>' prompt.\n");
    printf("  Commands:\n");
    printf("    'exit'        - Close client connection\n");
    printf("    'exit server' - Shutdown the server\n");
    printf("    Ctrl+C        - Exit client immediately\n");
    printf("\nNetwork Protocol:\n");
    printf("  Uses PDU format: [2-byte length][message data]\n");
    printf("  Length is in network byte order (big-endian)\n");
    printf("  Same protocol as UDP version for consistency\n");
    printf("\nServer Features:\n");
    printf("  - Detects client disconnection automatically\n");
    printf("  - Handles 'exit server' command gracefully\n");
    printf("  - Uses SO_REUSEADDR for development convenience\n");
    printf("\nExamples:\n");
    printf("  %s --server\n", program_name);
    printf("  %s --server --port 8080 --addr 192.168.1.100\n", program_name);
    printf("  %s --client\n", program_name);
    printf("  %s --client --port 8080 --addr 192.168.1.100\n", program_name);
}

// Helper function to create a network message PDU from a C string
// Returns total PDU length (header + data) on success, -1 on error
int netmsg_from_cstr(const char *msg_str, uint8_t *msg_buff, uint16_t msg_buff_sz) {
    if (!msg_str || !msg_buff || msg_buff_sz < sizeof(uint16_t)) {
        return -1;
    }
    
    uint16_t msg_len = strlen(msg_str);
    uint16_t total_len = sizeof(uint16_t) + msg_len;
    
    // Check if message fits in buffer
    if (total_len > msg_buff_sz) {
        return -1;
    }
    
    // Create PDU structure overlay
    echo_pdu_t *pdu = (echo_pdu_t *)msg_buff;
    
    // Set length in network byte order
    pdu->msg_len = htons(msg_len);
    
    // Copy message data
    memcpy(pdu->msg_data, msg_str, msg_len);
    
    return total_len;
}

// Helper function to extract message data from a received PDU
// Returns 0 on success, -1 on error
int extract_msg_data(const uint8_t *pdu_buff, uint16_t pdu_len, char *msg_str, uint16_t max_str_len) {
    if (!pdu_buff || !msg_str || pdu_len < sizeof(uint16_t) || max_str_len == 0) {
        return -1;
    }
    
    // Overlay PDU structure
    const echo_pdu_t *pdu = (const echo_pdu_t *)pdu_buff;
    
    // Extract message length (convert from network byte order)
    uint16_t msg_len = ntohs(pdu->msg_len);
    
    // Validate PDU length matches header + data
    if (pdu_len != sizeof(uint16_t) + msg_len) {
        return -1;
    }
    
    // Ensure we don't overflow destination string buffer
    uint16_t copy_len = (msg_len < max_str_len - 1) ? msg_len : max_str_len - 1;
    
    // Copy message data and null-terminate
    memcpy(msg_str, pdu->msg_data, copy_len);
    msg_str[copy_len] = '\0';
    
    return 0;
}


