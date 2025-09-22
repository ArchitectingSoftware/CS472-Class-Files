#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>

#include "tcp-echo.h"


// Global buffers
char send_buffer[BUFFER_SIZE];
char recv_buffer[BUFFER_SIZE];

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

void start_client(const char* addr, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    char input_buffer[BUFFER_SIZE];
    char extracted_msg[BUFFER_SIZE];
    ssize_t pdu_len;
    
    // Set up client-specific signal handler for Ctrl+C
    signal(SIGINT, client_signal_handler);
    
    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    client_sockfd = sockfd; // For signal handler
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Error: Invalid address %s\n", addr);
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server %s:%d\n", addr, port);
    printf("Type messages to send to server.\n");
    printf("Type 'exit' to quit, or 'exit server' to shutdown the server.\n");
    printf("Press Ctrl+C to exit at any time.\n\n");
    
    // Client interaction loop
    while (!client_exit_requested) {
        printf("> ");
        fflush(stdout);
        
        // Get input from user
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            if (client_exit_requested) {
                printf("\nExiting due to signal...\n");
            } else {
                printf("\nEOF detected, closing connection.\n");
            }
            break;
        }
        
        // Remove trailing newline
        size_t len = strlen(input_buffer);
        if (len > 0 && input_buffer[len-1] == '\n') {
            input_buffer[len-1] = '\0';
            len--;
        }
        
        // Check for exit command
        if (strcmp(input_buffer, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
        
        // Send message PDU to server
        if (send_pdu(sockfd, input_buffer) < 0) {
            printf("Error sending message. Server may have disconnected.\n");
            break;
        }
        
        // Receive response PDU from server
        pdu_len = recv_pdu(sockfd, extracted_msg, sizeof(extracted_msg));
        if (pdu_len < 0) {
            printf("Error receiving response. Server may have disconnected.\n");
            break;
        } else if (pdu_len == 0) {
            printf("Server closed connection.\n");
            break;
        }
        
        printf("Server: %s\n", extracted_msg);
        
        // Check if server is exiting (response to "exit server" command)
        if (strstr(extracted_msg, "server is exiting") != NULL) {
            printf("Server is shutting down.\n");
            break;
        }
    }
    
    close(sockfd);
    client_sockfd = -1;
    printf("Client disconnected.\n");
}

void start_server(const char* addr, int port) {
    int sockfd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN];
    char extracted_msg[BUFFER_SIZE];
    char response_msg[BUFFER_SIZE];
    ssize_t pdu_len;
    int reuse = 1;
    int server_should_exit = 0;
    
    // Create TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    server_sockfd = sockfd; // For signal handler
    
    // Set socket options to reuse address
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Error setting socket options");
        close(sockfd);
        exit(EXIT_FAILURE);
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
    
    // Listen for connections
    if (listen(sockfd, BACKLOG) < 0) {
        perror("Error listening on socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on %s:%d\n", addr, port);
    printf("Server will handle multiple clients sequentially.\n");
    printf("Send 'exit server' from any client to shutdown the server.\n");
    printf("Press Ctrl+C to stop server immediately.\n\n");
    
    // Main server loop - handle multiple clients
    while (!server_should_exit) {
        printf("Waiting for client connection...\n");
        
        // Accept client connection
        client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            perror("Error accepting connection");
            continue; // Try to accept next connection
        }
        
        client_sockfd = client_sock; // For signal handler
        
        // Get client IP address for logging
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        printf("Server ready to process messages from this client...\n");
        
        // Client communication loop
        while (1) {
            // Receive PDU from client
            pdu_len = recv_pdu(client_sock, extracted_msg, sizeof(extracted_msg));
            
            if (pdu_len < 0) {
                printf("Error receiving message from client.\n");
                break; // Close this client, wait for next one
            } else if (pdu_len == 0) {
                printf("Client disconnected gracefully.\n");
                break; // Close this client, wait for next one
            }
            
            printf("Received from client: \"%s\"\n", extracted_msg);
            
            // Check for exit server command
            if (strcmp(extracted_msg, "exit server") == 0) {
                printf("Client requested server shutdown.\n");
                
                // Send shutdown response
                strcpy(response_msg, "echo: exit server - The server is exiting");
                if (send_pdu(client_sock, response_msg) < 0) {
                    perror("Error sending shutdown response");
                } else {
                    printf("Sent shutdown message to client: \"%s\"\n", response_msg);
                }
                
                server_should_exit = 1; // Signal to exit main server loop
                break; // Break out of client loop
            }
            
            // Create echo response: "echo: original_message"
            snprintf(response_msg, sizeof(response_msg), "echo: %.500s", extracted_msg);
            
            // Send response PDU back to client
            if (send_pdu(client_sock, response_msg) < 0) {
                printf("Error sending response to client. Client may have disconnected.\n");
                break; // Close this client, wait for next one
            }
            
            printf("Sent to client: \"%s\"\n", response_msg);
            printf("---\n");
        }
        
        // Close current client connection
        close(client_sock);
        client_sockfd = -1;
        printf("Client connection closed.\n");
        
        if (!server_should_exit) {
            printf("Ready for next client connection.\n\n");
        }
    }
    
    // Clean up server socket
    close(sockfd);
    server_sockfd = -1;
    printf("Server shutdown complete.\n");
}

// Send all bytes in buffer (handles partial sends)
ssize_t send_all(int sockfd, const char* buffer, size_t length) {
    size_t bytes_sent = 0;
    ssize_t result;
    
    while (bytes_sent < length) {
        result = send(sockfd, buffer + bytes_sent, length - bytes_sent, 0);
        if (result < 0) {
            return -1;
        }
        bytes_sent += result;
    }
    
    return bytes_sent;
}

// Send a message as a PDU
ssize_t send_pdu(int sockfd, const char *message) {
    int pdu_len = netmsg_from_cstr(message, (uint8_t*)send_buffer, BUFFER_SIZE);
    if (pdu_len < 0) {
        fprintf(stderr, "Error: Message too long for buffer\n");
        return -1;
    }
    
    return send_all(sockfd, send_buffer, pdu_len);
}

// Receive a PDU and extract the message
ssize_t recv_pdu(int sockfd, char *message, size_t max_length) {
    // First, receive the length field (2 bytes)
    uint16_t net_msg_len;
    size_t bytes_received = 0;
    ssize_t result;
    
    // Receive length field
    while (bytes_received < sizeof(net_msg_len)) {
        result = recv(sockfd, ((char*)&net_msg_len) + bytes_received, 
                     sizeof(net_msg_len) - bytes_received, 0);
        if (result <= 0) {
            return result; // Error or connection closed
        }
        bytes_received += result;
    }
    
    // Convert length from network byte order
    uint16_t msg_len = ntohs(net_msg_len);
    
    // Validate message length
    if (msg_len > MAX_MSG_DATA_SIZE) {
        fprintf(stderr, "Error: Message length %u exceeds maximum %zu\n", 
                msg_len, (size_t)MAX_MSG_DATA_SIZE);
        return -1;
    }
    
    // Receive the message data
    bytes_received = 0;
    while (bytes_received < msg_len) {
        result = recv(sockfd, recv_buffer + bytes_received, 
                     msg_len - bytes_received, 0);
        if (result <= 0) {
            return result; // Error or connection closed
        }
        bytes_received += result;
    }
    
    // Extract message and null-terminate
    size_t copy_len = (msg_len < max_length - 1) ? msg_len : max_length - 1;
    memcpy(message, recv_buffer, copy_len);
    message[copy_len] = '\0';
    
    return copy_len;
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