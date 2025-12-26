/*
 * NTP Client - Network Programming Assignment
 * 
 * This program implements a simple NTP (Network Time Protocol) client that:
 * 1. Connects to an NTP server (default: pool.ntp.org)
 * 2. Sends a time synchronization request
 * 3. Processes the server's response
 * 4. Calculates time offset and network delay
 * 
 * LEARNING OBJECTIVES:
 * - Understanding binary protocol data units (PDUs)
 * - Working with packed C structures for network protocols
 * - Handling network byte order (htonl/ntohl)
 * - Time representation and conversion
 * - Basic network time synchronization concepts
 * 
 * COMPILE: make
 * RUN:     ./ntp-client
 *          ./ntp-client -s time.nist.gov
 * 
 * STUDENT INSTRUCTIONS:
 * Complete all functions marked with "STUDENT TODO" below.
 * Follow the implementation order suggested in the header file.
 * Refer to the detailed comments for guidance on each function.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include "ntp-protocol.h"

// Default NTP servers - you can test with different ones!
#define DEFAULT_NTP_SERVER "pool.ntp.org"
#define TIMEOUT_SECONDS 5

/*
 * =============================================================================
 * PROVIDED FUNCTIONS - NETWORKING AND PROGRAM STRUCTURE
 * These functions handle the network communication and program flow.
 * Students should NOT modify these functions.
 * =============================================================================
 */

// Main function - handles command line arguments and starts the NTP query
int main(int argc, char* argv[]) {
    char* ntp_server = DEFAULT_NTP_SERVER;
    
    // Parse command line arguments
    int opt;
    while ((opt = getopt(argc, argv, "s:hd")) != -1) {
        switch (opt) {
            case 's':
                ntp_server = optarg;
                break;
            case 'd':
                // Debug mode - demonstrate epoch conversion
                printf("=== DEBUG MODE ===\n");
                demonstrate_epoch_conversion();
                printf("\n");
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return 1;
        }
    }
    
    printf("Querying NTP server: %s\n", ntp_server);
    
    // Resolve hostname to IP address
    char server_ip[INET_ADDRSTRLEN];
    if (resolve_hostname(ntp_server, server_ip) < 0) {
        fprintf(stderr, "Failed to resolve hostname: %s\n", ntp_server);
        return 1;
    }
    
    printf("Server IP: %s\n", server_ip);
    
    // Query the NTP server
    int result = query_ntp_server(ntp_server, server_ip);
    return result;
}

// Print usage information
void usage(const char* progname) {
    printf("Usage: %s [-s server] [-d] [-h]\n", progname);
    printf("\nOptions:\n");
    printf("  -s server    NTP server to query (default: %s)\n", DEFAULT_NTP_SERVER);
    printf("  -d           Debug mode - show epoch conversion example\n");
    printf("  -h           Show this help\n");
    printf("\nExamples:\n");
    printf("  %s\n", progname);
    printf("  %s -s time.nist.gov\n", progname);
    printf("  %s -s pool.ntp.org\n", progname);
    printf("  %s -d\n", progname);
}

// Resolve hostname to IP address using DNS
int resolve_hostname(const char* hostname, char* ip_str) {
    struct hostent* host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        return -1;
    }
    
    struct in_addr addr;
    memcpy(&addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
    strcpy(ip_str, inet_ntoa(addr));
    
    return 0;
}

// Create UDP socket with appropriate timeout settings
int create_udp_socket() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    // Set timeout for receive operations
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SECONDS;
    timeout.tv_usec = 0;
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Send NTP request packet over UDP
int send_ntp_request(int sockfd, const struct sockaddr_in* server_addr, 
                     const ntp_packet_t* packet) {
    ssize_t sent = sendto(sockfd, packet, sizeof(ntp_packet_t), 0,
                         (struct sockaddr*)server_addr, sizeof(struct sockaddr_in));
    
    if (sent != sizeof(ntp_packet_t)) {
        perror("sendto");
        return -1;
    }
    
    return 0;
}

// Receive NTP response packet over UDP
int recv_ntp_response(int sockfd, ntp_packet_t* packet) {
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t received = recvfrom(sockfd, packet, sizeof(ntp_packet_t), 0,
                               (struct sockaddr*)&from_addr, &from_len);
    
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            fprintf(stderr, "NTP request timed out\n");
        } else {
            perror("recvfrom");
        }
        return -1;
    }
    
    if (received != sizeof(ntp_packet_t)) {
        fprintf(stderr, "Received incomplete NTP packet: %zd bytes\n", received);
        return -1;
    }
    
    return 0;
}

// Main NTP query function - coordinates the entire NTP exchange
// This function orchestrates the complete NTP protocol exchange
int query_ntp_server(const char* server_name, const char* ip_str) {
    // Create UDP socket
    int sockfd = create_udp_socket();
    if (sockfd < 0) {
        return -1;
    }
    
    // Set up server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(NTP_PORT);
    if (inet_pton(AF_INET, ip_str, &server_addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP address: %s\n", ip_str);
        close(sockfd);
        return -1;
    }
    
    printf("Connecting to %s (%s) on port %d\n", server_name, ip_str, NTP_PORT);
    
    // Build NTP request packet
    ntp_packet_t request_packet;
    if (build_ntp_request(&request_packet) < 0) {
        fprintf(stderr, "Failed to build NTP request\n");
        close(sockfd);
        return -1;
    }
    
    printf("\nSending NTP request...\n");
    print_ntp_packet_info(&request_packet, "Request", IS_REQUEST);
    
    // Convert to network byte order first, then send
    ntp_to_net(&request_packet);    
    if (send_ntp_request(sockfd, &server_addr, &request_packet) < 0) {
        fprintf(stderr, "Failed to send NTP request\n");
        close(sockfd);
        return -1;
    }
    
    // Receive NTP response
    ntp_packet_t response_packet;
    if (recv_ntp_response(sockfd, &response_packet) < 0) {
        fprintf(stderr, "Failed to receive NTP response\n");
        close(sockfd);
        return -1;
    }

    // FIRST thing to do is get receive time (T4) for accurate timing
    ntp_timestamp_t recv_time;
    get_current_ntp_time(&recv_time);
    
    // Convert both packets back to host byte order for processing
    ntp_to_host(&request_packet);
    ntp_to_host(&response_packet);

    printf("\nReceived NTP response from %s!\n", server_name);
    print_ntp_packet_info(&response_packet, "Response", IS_RESPONSE);
    
    // Calculate time offset and delay using NTP algorithm
    ntp_result_t result;
    if (calculate_ntp_offset(&request_packet, &response_packet, &recv_time, &result) < 0) {
        fprintf(stderr, "Failed to calculate time offset\n");
        close(sockfd);
        return -1;
    }
    
    printf("\n=== NTP Time Synchronization Results ===\n");
    printf("Server: %s\n", server_name);
    print_ntp_results(&result);
    
    close(sockfd);
    return 0;
}

/*
 * =============================================================================
 * DEBUGGING HELPER FUNCTIONS - PROVIDED FOR STUDENT USE
 * =============================================================================
 */

// Debug function to show bit field breakdown
void debug_print_bit_fields(const ntp_packet_t* packet) {
    uint8_t li = GET_NTP_LI(packet);
    uint8_t vn = GET_NTP_VN(packet);  
    uint8_t mode = GET_NTP_MODE(packet);
    
    printf("DEBUG: li_vn_mode byte = 0x%02X\n", packet->li_vn_mode);
    printf("  Leap Indicator = %d\n", li);
    printf("  Version = %d\n", vn);
    printf("  Mode = %d\n", mode);
    printf("  Binary breakdown: LI=%d%d VN=%d%d%d Mode=%d%d%d\n",
           (li >> 1) & 1, li & 1,
           (vn >> 2) & 1, (vn >> 1) & 1, vn & 1,
           (mode >> 2) & 1, (mode >> 1) & 1, mode & 1);
}

// Demonstration function for epoch conversion
void demonstrate_epoch_conversion(void) {
    // Get current time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    uint32_t unix_seconds = tv.tv_sec;
    uint32_t ntp_seconds = unix_seconds + NTP_EPOCH_OFFSET;
    
    printf("=== EPOCH CONVERSION EXAMPLE ===\n");
    printf("Current Unix time: %u seconds since 1970\n", unix_seconds);
    printf("Same time in NTP:  %u seconds since 1900\n", ntp_seconds);
    printf("Difference:        %u seconds (70 years)\n", (uint32_t)NTP_EPOCH_OFFSET);
    printf("Human readable:    %s", ctime((time_t*)&tv.tv_sec));
    printf("Valid NTP range:   ~3.9 billion seconds (for 2025)\n");
    printf("Valid Unix range:  ~1.7 billion seconds (for 2025)\n");
}

/*
 * =============================================================================
 * STUDENT IMPLEMENTATION SECTION
 * Complete the functions below according to the specifications.
 * Follow the implementation order suggested in the header file.
 * =============================================================================
 */

/*
 * GROUP 1: TIME CONVERSION FUNCTIONS
 * These functions handle time format conversions between system time and NTP time.
 */

//STUDENT TODO
/*
 * Get current system time and convert to NTP timestamp format
 * 
 * WHAT TO DO:
 * 1. Use gettimeofday() to get current Unix time (seconds + microseconds)
 * 2. Convert Unix epoch (1970) to NTP epoch (1900) by adding NTP_EPOCH_OFFSET
 * 3. Convert microseconds to NTP fractional format (1/2^32 units)
 * 
 * KEY C FUNCTIONS TO USE:
 * - gettimeofday() - gets current time as seconds + microseconds
 * 
 * EXAMPLE BEHAVIOR:
 * If current time is Sept 15, 2025 13:36:14.541216 UTC:
 * - ntp_ts->seconds should be ~3933894574 (includes NTP_EPOCH_OFFSET)
 * - ntp_ts->fraction should be ~2324671300 (541216 microseconds converted)
 * 
 * MATH HINT:
 * To convert microseconds to NTP fraction: (microseconds * 2^32) / 1,000,000
 * 
 * DEBUGGING TIP:
 * Use demonstrate_epoch_conversion() to verify your conversion logic
 */
void get_current_ntp_time(ntp_timestamp_t *ntp_ts){
    printf("get_current_ntp_time() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Use gettimeofday(), convert epoch, scale microseconds
    memset(ntp_ts, 0, sizeof(ntp_timestamp_t));
}

//STUDENT TODO
/*
 * Convert NTP timestamp to human-readable string
 * 
 * WHAT TO DO:
 * 1. Convert NTP timestamp back to Unix time (subtract NTP_EPOCH_OFFSET)
 * 2. Convert NTP fraction back to microseconds
 * 3. Use localtime() or gmtime() based on 'local' parameter
 * 4. Format using snprintf() in "YYYY-MM-DD HH:MM:SS.uuuuuu" format
 * 
 * KEY C FUNCTIONS TO USE:
 * - localtime() - converts to local timezone
 * - gmtime() - converts to UTC
 * - snprintf() - formats the string
 * 
 * EXAMPLE BEHAVIOR:
 * Input: NTP timestamp for Sept 15, 2025 13:36:14.541216
 * Output: "2025-09-15 13:36:14.541216" (if UTC) or local timezone equivalent
 * 
 * MATH HINT:
 * To convert NTP fraction to microseconds: (fraction * 1,000,000) / 2^32
 * 
 * ERROR HANDLING:
 * If conversion fails, use snprintf to write "INVALID_TIME" to buffer
 */
void ntp_time_to_string(const ntp_timestamp_t *ntp_ts, char *buffer, size_t buffer_size, int local) {
    printf("ntp_time_to_string() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Convert NTP to Unix time, use localtime/gmtime, format with snprintf
    snprintf(buffer, buffer_size, "TO BE IMPLEMENTED");
}

//STUDENT TODO
/*
 * Convert NTP timestamp to double for mathematical operations
 * 
 * WHAT TO DO:
 * 1. Convert seconds part to double
 * 2. Convert fraction part to decimal fraction (divide by 2^32)
 * 3. Add them together
 * 
 * KEY C FUNCTIONS TO USE:
 * - Standard arithmetic operations
 * - Type casting to double
 * 
 * EXAMPLE BEHAVIOR:
 * Input: seconds=3933894574, fraction=2324671300
 * Output: 3933894574.541216 (approximately)
 * 
 * PURPOSE:
 * This allows precise mathematical operations needed for NTP calculations
 * 
 * PRECISION NOTE:
 * Use NTP_FRACTION_SCALE (2^32) constant for the division
 */
double ntp_time_to_double(const ntp_timestamp_t* timestamp) {
    printf("ntp_time_to_double() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Convert both parts to double and add
    return 0.0;
}

//STUDENT TODO
/*
 * Print NTP timestamp with descriptive label
 * 
 * WHAT TO DO:
 * 1. Use ntp_time_to_string() to convert timestamp to string
 * 2. Print with appropriate label and timezone indicator
 * 
 * KEY C FUNCTIONS TO USE:
 * - printf() - for formatted output
 * 
 * EXAMPLE BEHAVIOR:
 * Input: timestamp for current time, label="Transmit Time", local=1
 * Output: "Transmit Time: 2025-09-15 13:36:14.541216 (Local Time)"
 */
void print_ntp_time(const ntp_timestamp_t *ts, const char* label, int local){
    printf("print_ntp_time() - TO BE IMPLEMENTED - %s\n", label);
    // TODO: Implement this function
    // Hint: Use ntp_time_to_string and printf
}

/*
 * GROUP 2: NETWORK BYTE ORDER FUNCTIONS
 * These functions handle conversion between host and network byte order.
 * Network protocols require big-endian byte order regardless of host architecture.
 */

//STUDENT TODO
/*
 * Convert NTP timestamp from host to network byte order
 * 
 * WHAT TO DO:
 * 1. Convert both seconds and fraction fields using htonl()
 * 2. Modify the structure in place
 * 
 * KEY C FUNCTIONS TO USE:
 * - htonl() - host to network long (32-bit conversion)
 * 
 * EXAMPLE BEHAVIOR:
 * Input: timestamp with seconds=0x12345678 (little-endian host)
 * Output: timestamp with seconds=0x78563412 (big-endian network)
 * 
 * WHY NEEDED:
 * Network protocols require consistent byte order across different architectures
 */
void ntp_ts_to_net(ntp_timestamp_t* timestamp){
    printf("ntp_ts_to_net() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Use htonl() on both seconds and fraction fields
}

//STUDENT TODO
/*
 * Convert NTP timestamp from network to host byte order
 * 
 * WHAT TO DO:
 * 1. Convert both seconds and fraction fields using ntohl()
 * 2. Modify the structure in place
 * 
 * KEY C FUNCTIONS TO USE:
 * - ntohl() - network to host long (32-bit conversion)
 * 
 * EXAMPLE BEHAVIOR:
 * Input: timestamp with seconds=0x78563412 (big-endian network)
 * Output: timestamp with seconds=0x12345678 (little-endian host)
 * 
 * WHY NEEDED:
 * Host processing requires native byte order for correct arithmetic
 */
void ntp_ts_to_host(ntp_timestamp_t* timestamp){
    printf("ntp_ts_to_host() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Use ntohl() on both seconds and fraction fields
}

//STUDENT TODO
/*
 * Convert entire NTP packet from host to network byte order
 * 
 * WHAT TO DO:
 * 1. Convert all 32-bit fields using htonl(): root_delay, root_dispersion, reference_id
 * 2. Convert all timestamp fields using ntp_ts_to_net()
 * 3. Leave 8-bit fields unchanged (li_vn_mode, stratum, poll, precision)
 * 
 * KEY C FUNCTIONS TO USE:
 * - htonl() - for 32-bit fields
 * - ntp_ts_to_net() - for timestamp fields
 * 
 * EXAMPLE BEHAVIOR:
 * Converts all multi-byte fields in packet from host to network byte order
 * Single-byte fields (stratum, poll, etc.) remain unchanged
 * 
 * CALL THIS: Before sending packet over network
 */
void ntp_to_net(ntp_packet_t* packet){
    printf("ntp_to_net() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Convert 32-bit fields with htonl(), timestamps with ntp_ts_to_net()
}

//STUDENT TODO
/*
 * Convert entire NTP packet from network to host byte order
 * 
 * WHAT TO DO:
 * 1. Convert all 32-bit fields using ntohl(): root_delay, root_dispersion, reference_id
 * 2. Convert all timestamp fields using ntp_ts_to_host()
 * 3. Leave 8-bit fields unchanged
 * 
 * KEY C FUNCTIONS TO USE:
 * - ntohl() - for 32-bit fields
 * - ntp_ts_to_host() - for timestamp fields
 * 
 * EXAMPLE BEHAVIOR:
 * Converts all multi-byte fields in packet from network to host byte order
 * 
 * CALL THIS: After receiving packet from network
 */
void ntp_to_host(ntp_packet_t* packet){
    printf("ntp_to_host() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Convert 32-bit fields with ntohl(), timestamps with ntp_ts_to_host()
}

/*
 * GROUP 3: NTP PACKET CONSTRUCTION
 * These functions build NTP protocol packets according to RFC specifications.
 */

//STUDENT TODO
/*
 * Build NTP client request packet
 * 
 * WHAT TO DO:
 * 1. Clear entire packet with memset()
 * 2. Set leap indicator, version, mode using SET_NTP_LI_VN_MODE macro
 *    - LI: NTP_LI_UNSYNC (3) - client clock not synchronized
 *    - VN: NTP_VERSION (4) - NTP version 4
 *    - Mode: NTP_MODE_CLIENT (3) - client request
 * 3. Set stratum to 0 (unspecified for client)
 * 4. Set poll to 6 (64 second interval)
 * 5. Set precision to -20 (~1 microsecond)
 * 6. Set root_delay and root_dispersion to 0
 * 7. Set reference_id to 0
 * 8. Clear all timestamp fields except transmit time
 * 9. Set transmit time to current time using get_current_ntp_time()
 * 
 * KEY C FUNCTIONS TO USE:
 * - memset() - clear packet structure
 * - memcpy() or assignment - set fields
 * - get_current_ntp_time() - set transmit timestamp
 * 
 * EXAMPLE BEHAVIOR:
 * Creates a valid NTP client request with:
 * - All control fields properly set
 * - Only transmit timestamp filled (others are zero)
 * - Packet ready to send to server
 * 
 * DEBUGGING TIP:
 * Use debug_print_bit_fields() to verify your bit field settings
 * 
 * RETURN VALUE:
 * RC_OK (0) on success, RC_BAD_PACKET (-1) if packet is NULL
 */
int build_ntp_request(ntp_packet_t* packet) {
    printf("build_ntp_request() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: memset to zero, set bit fields with macro, set basic fields, set transmit time
    if (!packet) {
        return RC_BAD_PACKET;
    }
    memset(packet, 0, sizeof(ntp_packet_t));
    
    // After you implement this, uncomment the line below to debug:
    // debug_print_bit_fields(packet);
    
    return RC_OK;
}

/*
 * GROUP 4: PROTOCOL ANALYSIS FUNCTIONS
 * These functions parse NTP responses and perform time calculations.
 */

//STUDENT TODO
/*
 * Decode NTP reference_id field based on stratum level
 * 
 * WHAT TO DO:
 * 1. Check buffer size requirements:
 *    - If ref_id == 0: need 5 bytes for "NONE"
 *    - If stratum >= 2: need 16 bytes for IP address string
 *    - If stratum < 2: need 5 bytes for 4-char ASCII code
 * 2. Handle ref_id == 0 case: copy "NONE" to buffer
 * 3. Handle stratum >= 2 case: treat ref_id as IP address
 *    - Convert ref_id to network byte order with htonl()
 *    - Use inet_ntop() to convert to IP string
 * 4. Handle stratum < 2 case: treat ref_id as 4 ASCII characters
 *    - Convert ref_id to network byte order with htonl()
 *    - Copy 4 bytes directly to buffer as characters (like "NIST")
 * 
 * KEY C FUNCTIONS TO USE:
 * - strcpy() - for "NONE" case
 * - htonl() - convert to network byte order for inet_ntop()
 * - inet_ntop() - convert IP address to string
 * - memset() and memcpy() - for ASCII character handling
 * 
 * EXAMPLE BEHAVIOR:
 * - ref_id=0: output="NONE"
 * - stratum=2, ref_id=0xcf424f67: output="207.66.79.103" (IP address)
 * - stratum=1, ref_id=0x4e495354: output="NIST" (ASCII characters)
 * 
 * BUFFER SIZE REQUIREMENTS:
 * - Return RC_BUFF_TOO_SMALL if buffer is too small for any case
 * 
 * RETURN VALUE:
 * RC_OK (0) on success, RC_BUFF_TOO_SMALL (-2) if buffer too small
 */
int decode_reference_id(uint8_t stratum, uint32_t ref_id, char *buff, int buff_sz){
    printf("decode_reference_id() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Check buffer sizes, handle ref_id==0, stratum>=2 (IP), stratum<2 (ASCII)
    snprintf(buff, buff_sz, "TO BE IMPLEMENTED");
    return RC_OK;
}

//STUDENT TODO
/*
 * Calculate NTP time offset and delay using standard algorithm
 * 
 * WHAT TO DO:
 * 1. Extract the four timestamps and convert to double for precise math:
 *    - T1 = request->xmit_time (client request send time)
 *    - T2 = response->recv_time (server receive time)
 *    - T3 = response->xmit_time (server response send time)
 *    - T4 = recv_time (client response receive time)
 * 2. Calculate round-trip delay: delay = (T4 - T1) - (T3 - T2)
 * 3. Calculate time offset: offset = ((T2 - T1) + (T3 - T4)) / 2
 * 4. Calculate final dispersion using server values and computed delay
 * 5. Copy server and client timestamps to result structure
 * 
 * KEY C FUNCTIONS TO USE:
 * - ntp_time_to_double() - convert timestamps for math
 * - GET_NTP_Q1616_TS() - decode server dispersion/delay values
 * - memcpy() - copy timestamp structures
 * 
 * DETAILED MATH EXPLANATION:
 * 
 * NTP DELAY CALCULATION:
 * Formula: delay = (T4 - T1) - (T3 - T2)
 * Meaning: 
 * - (T4 - T1) = total time from client send to client receive
 * - (T3 - T2) = time the packet spent at the server
 * - Subtracting server time gives us pure network transit time
 * - This accounts for server processing delays
 * Example: If total round-trip is 100ms and server held packet for 20ms,
 *          then network delay = 100ms - 20ms = 80ms
 * 
 * NTP OFFSET CALCULATION:
 * Formula: offset = ((T2 - T1) + (T3 - T4)) / 2
 * Meaning:
 * - (T2 - T1) = how much client was behind when sending (includes network delay)
 * - (T3 - T4) = how much client was behind when receiving (includes network delay)
 * - Adding these cancels out network delays in opposite directions
 * - Dividing by 2 gives the average time difference
 * - Positive result: client clock is BEHIND server
 * - Negative result: client clock is AHEAD of server
 * Example: If T2-T1 = +50ms and T3-T4 = +30ms, offset = (50+30)/2 = +40ms behind
 * 
 * FINAL DISPERSION CALCULATION:
 * Formula: final_dispersion = server_dispersion + (server_delay/2) + (delay/2)
 * Meaning:
 * - server_dispersion = server's estimate of its own time accuracy
 * - server_delay/2 = half of server's root delay (accumulated network uncertainty)
 * - delay/2 = half of our measured delay (our network uncertainty)
 * - This gives total estimated error bounds for the time synchronization
 * - Smaller dispersion = more accurate time sync
 * Example: If server dispersion = 5ms, server delay = 10ms, our delay = 20ms
 *          then final_dispersion = 5 + 5 + 10 = 20ms error estimate
 * 
 * WHY THIS WORKS:
 * The NTP algorithm assumes network delays are symmetric (same in both directions).
 * While not always true, this assumption allows us to separate clock offset
 * from network delay using only four timestamps. The math elegantly cancels
 * out the network delay components when calculating offset.
 * 
 * EXAMPLE BEHAVIOR:
 * Input: Four timestamps from NTP exchange
 * Output: Populated ntp_result_t with offset, delay, dispersion, and times
 * 
 * RETURN VALUE:
 * 0 on success, -1 if any pointer is NULL
 */
int calculate_ntp_offset(const ntp_packet_t* request, 
                        const ntp_packet_t* response,
                        const ntp_timestamp_t* recv_time, 
                        ntp_result_t* result) {
    printf("calculate_ntp_offset() - TO BE IMPLEMENTED\n");
    // TODO: Implement this function
    // Hint: Extract T1-T4 timestamps, apply NTP formulas, calculate dispersion
    if (!request || !response || !result) {
        return -1;
    }
    
    // Initialize result with dummy values
    result->delay = 0.0;
    result->offset = 0.0;
    result->final_dispersion = 0.0;
    memset(&result->server_time, 0, sizeof(ntp_timestamp_t));
    memset(&result->client_time, 0, sizeof(ntp_timestamp_t));
    
    return 0;
}

/*
 * GROUP 5: DISPLAY FUNCTIONS
 * These functions format and display NTP information for the user.
 */

//STUDENT TODO
/*
 * Print detailed NTP packet information in human-readable format
 * 
 * WHAT TO DO:
 * 1. Print packet type header with label
 * 2. Extract and print bit fields using GET_NTP_* macros:
 *    - Leap Indicator, Version, Mode
 * 3. Print basic fields: stratum, poll, precision
 * 4. Decode and print reference_id using decode_reference_id()
 * 5. Print root_delay and root_dispersion values
 * 6. Print all timestamps using print_ntp_time()
 * 
 * KEY C FUNCTIONS TO USE:
 * - printf() - formatted output
 * - GET_NTP_LI(), GET_NTP_VN(), GET_NTP_MODE() - extract bit fields
 * - decode_reference_id() - decode reference field
 * - print_ntp_time() - format timestamps
 * 
 * EXAMPLE OUTPUT:
 * --- Response Packet ---
 * Leap Indicator: 0
 * Version: 4
 * Mode: 4
 * Stratum: 2
 * Poll: 6
 * Precision: -24
 * Reference ID: [0x179b2826] 23.155.40.38
 * Root Delay: 23
 * Root Dispersion 1669
 * Reference Time: 2025-09-15 08:58:17.614668 (Local Time)
 * Original Time (T1): 2025-09-15 09:09:34.232246 (Local Time)
 * Receive Time (T2): 2025-09-15 09:09:34.348225 (Local Time)
 * Transmit Time (T3): 2025-09-15 09:09:34.348244 (Local Time)
 */
void print_ntp_packet_info(const ntp_packet_t* packet, const char* label, int packet_type) {
    printf("print_ntp_packet_info() - TO BE IMPLEMENTED - %s Packet\n", label);
    // TODO: Implement this function
    // Hint: Use printf for fields, GET_NTP_* macros for bit fields, decode_reference_id, print_ntp_time
}

//STUDENT TODO
/*
 * Print NTP synchronization results with user-friendly analysis
 * 
 * WHAT TO DO:
 * 1. Convert server and client timestamps to readable strings
 * 2. Print server time, local time, and round-trip delay
 * 3. Print time offset and final dispersion
 * 4. Analyze offset to determine if clock is ahead or behind
 * 5. Convert values to milliseconds for easier reading
 *
 * EXAMPLE OUTPUT: 
=== NTP Time Synchronization Results ===
Server: pool.ntp.org
Server Time: 2025-09-15 09:09:34.348244 (local time)
Local Time:  2025-09-15 09:09:34.302108 (local time)
Round Trip Delay: 0.069842

Time Offset: 0.081058 seconds
Final dispersion 0.034921

Your clock is running BEHIND by 81.06ms
Your estimated time error will be +/- 34.92ms
 */
void print_ntp_results(const ntp_result_t* result) {
    /* Here are some local buffers that you should use*/
    char svr_time_buff[TIME_BUFF_SIZE];
    char cli_time_buff[TIME_BUFF_SIZE];

    printf("print_ntp_results() - TO BE IMPLEMENTED\n");
    //Hint:  Note that you really dont have to do much here other than
    //       Print out data that is passed in teh result arguement
}