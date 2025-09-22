#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdint.h>

#include "udp-echo.h"



// Global buffers
char send_buffer[BUFFER_SIZE];
char recv_buffer[BUFFER_SIZE];


int main(int argc, char* argv[]) {
    int is_client = 0;
    int is_server = 0;
    int port = DEFAULT_PORT;
    char addr[INET_ADDRSTRLEN] = {0};
    char message[BUFFER_SIZE] = DEFAULT_CLIENT_MESSAGE;
    char prefix[BUFFER_SIZE] = DEFAULT_SERVER_PREFIX;
    
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
        } else if (argv[i][0] != '-') {
            // This is a quoted message or prefix
            if (is_client || (!is_client && !is_server)) {
                strncpy(message, argv[i], sizeof(message) - 1);
                message[sizeof(message) - 1] = '\0';
            } else if (is_server) {
                strncpy(prefix, argv[i], sizeof(prefix) - 1);
                prefix[sizeof(prefix) - 1] = '\0';
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
        printf("Starting UDP client: connecting to %s:%d, message: \"%s\"\n", 
               addr, port, message);
        start_client(addr, port, message);
    } else {
        printf("Starting UDP server: binding to %s:%d, prefix: \"%s\"\n", 
               addr, port, prefix);
        start_server(addr, port, prefix);
    }
    
    return 0;
}

void start_client(const char* addr, int port, const char* message) {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);
    ssize_t bytes_sent, bytes_received;
    char extracted_msg[BUFFER_SIZE];
    
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: Invalid address %s\n", addr);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Create PDU from message string
    int pdu_len = netmsg_from_cstr(message, (uint8_t*)send_buffer, BUFFER_SIZE);
    if (pdu_len < 0) {
        fprintf(stderr, "Error: Message too long for buffer\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Send PDU to server
    bytes_sent = sendto(sockfd, send_buffer, pdu_len, 0,
                       (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (bytes_sent < 0) {
        perror("Error sending message");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Sent %zd bytes to server (PDU with message: \"%s\")\n", bytes_sent, message);
    
    // Receive response PDU from server
    bytes_received = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0,
                             (struct sockaddr*)&server_addr, &addr_len);
    if (bytes_received < 0) {
        perror("Error receiving response");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Extract message from received PDU
    if (extract_msg_data((uint8_t*)recv_buffer, bytes_received, extracted_msg, sizeof(extracted_msg)) < 0) {
        fprintf(stderr, "Error: Invalid PDU received\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Received %zd bytes from server (message: \"%s\")\n", bytes_received, extracted_msg);
    
    close(sockfd);
}

void start_server(const char* addr, int port, const char* prefix) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    ssize_t bytes_received, bytes_sent;
    char client_ip[INET_ADDRSTRLEN];
    char extracted_msg[BUFFER_SIZE];
    char response_msg[BUFFER_SIZE];
    int reuse = 1;
    
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    // Set socket option to reuse address (helpful for debugging)
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Warning: Could not set SO_REUSEADDR");
        // Continue anyway - this is not a fatal error
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (strcmp(addr, "0.0.0.0") == 0) {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
            fprintf(stderr, "Error: Invalid address %s\n", addr);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }
    
    // Bind socket to address
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on %s:%d\n", addr, port);
    printf("Waiting for client messages... (Press Ctrl+C to stop)\n");
    
    // Server loop
    while (1) {
        // Receive PDU from client
        bytes_received = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0,
                                 (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (bytes_received < 0) {
            perror("Error receiving message");
            continue;
        }
        
        // Extract message from received PDU
        if (extract_msg_data((uint8_t*)recv_buffer, bytes_received, extracted_msg, sizeof(extracted_msg)) < 0) {
            fprintf(stderr, "Error: Invalid PDU received, ignoring\n");
            continue;
        }
        
        // Get client IP address for logging
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Received %zd bytes from %s:%d (message: \"%s\")\n", 
               bytes_received, client_ip, ntohs(client_addr.sin_port), extracted_msg);
        
        // Check for exit command
        if (strcmp(extracted_msg, "exit") == 0) {
            printf("Client requested server shutdown.\n");
            
            // Create shutdown response PDU
            strcpy(response_msg, "The server is exiting");
            int pdu_len = netmsg_from_cstr(response_msg, (uint8_t*)send_buffer, BUFFER_SIZE);
            if (pdu_len < 0) {
                fprintf(stderr, "Error creating shutdown response PDU\n");
                break;
            }
            
            // Send shutdown message to client
            bytes_sent = sendto(sockfd, send_buffer, pdu_len, 0,
                               (struct sockaddr*)&client_addr, client_addr_len);
            
            if (bytes_sent < 0) {
                perror("Error sending shutdown response");
            } else {
                printf("Sent shutdown message to client: \"%s\"\n", response_msg);
            }
            
            printf("Server shutting down.\n");
            break;
        }
        
        // Create response message: "prefix: original_message"
        snprintf(response_msg, sizeof(response_msg), "%.500s: %.500s", prefix, extracted_msg);
        
        // Create response PDU
        int pdu_len = netmsg_from_cstr(response_msg, (uint8_t*)send_buffer, BUFFER_SIZE);
        if (pdu_len < 0) {
            fprintf(stderr, "Error: Response message too long for buffer\n");
            continue;
        }
        
        // Send response back to client
        bytes_sent = sendto(sockfd, send_buffer, pdu_len, 0,
                           (struct sockaddr*)&client_addr, client_addr_len);
        
        if (bytes_sent < 0) {
            perror("Error sending response");
            continue;
        }
        
        printf("Sent %zd bytes back to client (message: \"%s\")\n", bytes_sent, response_msg);
        printf("---\n");
    }
    
    close(sockfd);
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [MESSAGE/PREFIX]\n", program_name);
    printf("OPTIONS:\n");
    printf("  --client              Run in client mode\n");
    printf("  --server              Run in server mode\n");
    printf("  --port <port>         Port number (default: %d)\n", DEFAULT_PORT);
    printf("  --addr <address>      IP address\n");
    printf("                        Client: server address (default: %s)\n", DEFAULT_CLIENT_ADDR);
    printf("                        Server: bind address (default: %s)\n", DEFAULT_SERVER_ADDR);
    printf("  MESSAGE/PREFIX        For client: message to send (default: \"%s\")\n", DEFAULT_CLIENT_MESSAGE);
    printf("                        For server: response prefix (default: \"%s\")\n", DEFAULT_SERVER_PREFIX);
    printf("\nSpecial Commands:\n");
    printf("  Client can send \"exit\" to stop the server\n");
    printf("\nNetwork Protocol:\n");
    printf("  Uses PDU format: [2-byte length][message data]\n");
    printf("  Length is in network byte order (big-endian)\n");
    printf("\nExamples:\n");
    printf("  %s --server\n", program_name);
    printf("  %s --server --port 8080 --addr 192.168.1.100 \"SERVER\"\n", program_name);
    printf("  %s --client\n", program_name);
    printf("  %s --client --port 8080 --addr 192.168.1.100 \"Hello World\"\n", program_name);
    printf("  %s --client --port 8080 --addr 192.168.1.100 \"exit\"\n", program_name);
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