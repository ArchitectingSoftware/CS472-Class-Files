/*
 * NTP Protocol Header - Network Programming Assignment
 * 
 * This header file defines the complete NTP (Network Time Protocol) 
 * packet structure and related constants. This is a great example of
 * how network protocols are implemented in C using packed structures.
 * 
 * KEY LEARNING CONCEPTS:
 * - Binary protocol data units (PDUs)
 * - Packed C structures for network protocols  
 * - Network byte order considerations
 * - Bit field manipulation with macros
 * - Fixed-size protocol messages
 * - Time representation and conversion
 * - Network time synchronization algorithms
 * 
 * NTP PROTOCOL OVERVIEW:
 * NTP uses UDP port 123 and fixed 48-byte packets. The protocol
 * synchronizes clocks by exchanging timestamps and calculating
 * network delay and clock offset using a four-timestamp algorithm.
 * 
 * STUDENT IMPLEMENTATION ORDER:
 * Complete the functions in this suggested order:
 * 1. Time Conversion Functions (get time, convert formats)
 * 2. Network Byte Order Functions (host/network conversions)  
 * 3. NTP Packet Construction (build request packets)
 * 4. Protocol Analysis (decode fields, calculate offsets)
 * 5. Display Functions (format output)
 */

#ifndef NTP_PROTOCOL_H
#define NTP_PROTOCOL_H

#include <stdint.h>
#include <arpa/inet.h>
#include <sys/time.h>

/*
 * =============================================================================
 * NTP PROTOCOL CONSTANTS
 * =============================================================================
 */

// Network parameters
#define NTP_PORT 123                    // Standard NTP UDP port
#define NTP_PACKET_SIZE 48             // Fixed NTP packet size

// NTP time constants with clear explanations
#define NTP_EPOCH_YEAR 1900
#define UNIX_EPOCH_YEAR 1970
#define YEARS_BETWEEN_EPOCHS 70
#define NTP_EPOCH_OFFSET 2208988800ULL  // Seconds from 1900 to 1970
#define NTP_FRACTION_SCALE 4294967296ULL // 2^32 for fractional seconds
#define USEC_INCREMENTS 1000000         // The number of micro-seconds in a sec

// NTP protocol version
#define NTP_VERSION         4           // We implement NTP version 4

// NTP mode values (3-bit field)
#define NTP_MODE_RESERVED   0           // Reserved
#define NTP_MODE_SYMACTIVE  1           // Symmetric active
#define NTP_MODE_SYMPASSIVE 2           // Symmetric passive  
#define NTP_MODE_CLIENT     3           // Client request
#define NTP_MODE_SERVER     4           // Server response
#define NTP_MODE_BROADCAST  5           // Broadcast mode
#define NTP_MODE_CONTROL    6           // Control message
#define NTP_MODE_PRIVATE    7           // Private use

// NTP leap indicator values (2-bit field)
#define NTP_LI_NONE         0           // No leap second warning
#define NTP_LI_ADD_SECOND   1           // Last minute has 61 seconds
#define NTP_LI_DEL_SECOND   2           // Last minute has 59 seconds  
#define NTP_LI_UNSYNC       3           // Clock not synchronized

/*
 * =============================================================================
 * TIME EPOCHS - Why NTP and Unix Use Different Starting Points
 * =============================================================================
 * 
 * Think of epochs like "Year Zero" for different calendar systems:
 * 
 * Unix Time (used by your computer):
 * - Starts: January 1, 1970, 00:00:00 UTC
 * - Right now: ~1727789000 seconds since 1970
 * - Why 1970? Unix was developed in early 1970s
 * 
 * NTP Time (used by time servers):  
 * - Starts: January 1, 1900, 00:00:00 UTC
 * - Right now: ~3936777800 seconds since 1900
 * - Why 1900? Covers entire 20th century and beyond
 * 
 * Timeline Visualization:
 * 1900 -------- 1970 -------- 2025
 *  |              |              |
 * NTP             Unix           Now
 * epoch           epoch          
 * starts          starts         
 *  |<-- 70 years ->|<-- 55 years ->|
 *  |<------- 125 years total ----->|
 * 
 * The Conversion:
 * NTP_EPOCH_OFFSET = 2,208,988,800 seconds = exactly 70 years
 * 
 * Your Code Will Do This:
 * unix_time = ntp_time - 2208988800    // NTP to Unix
 * ntp_time = unix_time + 2208988800    // Unix to NTP
 * 
 * MEMORY TRICK: NTP time is always BIGGER (it started counting earlier)
 */

/*
 * =============================================================================
 * NTP DATA STRUCTURES
 * =============================================================================
 */

/*
 * NTP Timestamp Structure (64 bits = 8 bytes)
 * 
 * Represents time as seconds and fractional seconds since 1900-01-01 00:00:00 UTC.
 * This gives NTP a range of about 136 years with ~232 picosecond resolution.
 * 
 * IMPORTANT: All multi-byte fields are in network byte order (big-endian)!
 * 
 * STUDENT NOTE: You'll convert between this format and standard Unix time
 * using the provided conversion constants and library functions like gettimeofday().
 */
typedef struct {
    uint32_t seconds;    // Seconds since NTP epoch (1900-01-01)
    uint32_t fraction;   // Fractional seconds in 1/2^32 increments
} __attribute__((packed)) ntp_timestamp_t;

/*
 * Complete NTP Packet Structure (384 bits = 48 bytes)
 * 
 * This is the entire NTP message format defined in RFC 5905.
 * The fixed size makes it perfect for C struct representation.
 * 
 * CRITICAL: The __attribute__((packed)) ensures no padding between fields,
 * which is essential for network protocols!
 * 
 * STUDENT NOTE: You'll manipulate this structure to:
 * - Build client request packets
 * - Parse server response packets  
 * - Convert between host and network byte order
 * - Extract timing information for calculations
 */
typedef struct {
    // First byte: bit fields packed together
    uint8_t li_vn_mode;         // LI(2) + VN(3) + Mode(3) bits
    
    // Configuration fields  
    uint8_t stratum;            // Stratum level (0-15)
    uint8_t poll;               // Poll interval (log2 seconds)
    int8_t precision;           // Clock precision (log2 seconds)
    
    // Root synchronization info (32-bit fixed point)
    // root_delay and root_dispersion encoded in Q16.16
    // format, upper 16 bits is seconds, lower 16 bits
    // are fractions of second, 1/2^16 or 65535 ticks
    // per second, see the Q1616 helper macros to decode
    uint32_t root_delay;        // Total roundtrip delay to reference
    uint32_t root_dispersion;   // Total dispersion to reference
    uint32_t reference_id;      // Reference clock identifier
    
    // Timestamp fields (the heart of NTP!)
    ntp_timestamp_t ref_time;   // Time when system clock was last set
    ntp_timestamp_t orig_time;  // Client request time (T1)
    ntp_timestamp_t recv_time;  // Server receive time (T2)  
    ntp_timestamp_t xmit_time;  // Server transmit time (T3)
} __attribute__((packed)) ntp_packet_t;

/*
 * Return codes for student functions
 */
#define RC_OK               0
#define RC_BAD_PACKET       -1
#define RC_BUFF_TOO_SMALL   -2

// Request and Response constants for printing
#define IS_REQUEST          0
#define IS_RESPONSE         1

// Local or UTC time constants for display
#define UTC_TIME            0
#define LOCAL_TIME          1
#define TIME_BUFF_SIZE      32 // Space to hold a timestamp string

/*
 * NTP Time Calculation Results
 * 
 * Structure to hold the calculated time synchronization results.
 * These values are computed using the standard NTP algorithm.
 * 
 * STUDENT NOTE: You'll populate this structure in calculate_ntp_offset()
 * using the four-timestamp NTP algorithm.
 */
typedef struct {
    double offset;      // Clock offset from server (seconds)
    double delay;       // Round-trip network delay (seconds)
    ntp_timestamp_t server_time; // Server time when response sent
    ntp_timestamp_t client_time;  // Local time when response received
    double final_dispersion; // Quality dispersion estimate
} ntp_result_t;

/*
 * =============================================================================
 * BIT FIELD MANIPULATION - SIMPLIFIED EXPLANATION
 * =============================================================================
 * 
 * The Packed Byte: `li_vn_mode`
 * 
 * NTP packs three small fields into one byte for efficiency:
 * - Leap Indicator (2 bits): Clock sync status - use 3 for client requests
 * - Version Number (3 bits): Always 4 for modern NTP
 * - Mode (3 bits): 3 = client request, 4 = server response
 * 
 * STUDENTS: Use the provided macros - no manual bit math required!
 * SET_NTP_LI_VN_MODE(packet, NTP_LI_UNSYNC, NTP_VERSION, NTP_MODE_CLIENT);
 * 
 * See "Bit Packing Visual Guide" below for detailed example.
 */

/*
 * BIT PACKING WALKTHROUGH - Step by Step Example
 * 
 * Let's build li_vn_mode for a client request:
 * - Leap Indicator (LI) = 3 (unsynchronized)  
 * - Version (VN) = 4 (NTP version 4)
 * - Mode = 3 (client request)
 * 
 * Step 1: Convert to binary
 * LI = 3  = 11 (binary)
 * VN = 4  = 100 (binary) 
 * Mode = 3 = 011 (binary)
 * 
 * Step 2: Pack into single byte
 * Bit positions: 7 6 | 5 4 3 | 2 1 0
 * Values:        1 1 | 1 0 0 | 0 1 1
 * Result: 11100011 = 0xE3 = 227 decimal
 * 
 * Step 3: Use the macro (students don't do math manually!)
 * SET_NTP_LI_VN_MODE(packet, 3, 4, 3);
 * // This sets packet->li_vn_mode = 0xE3
 * 
 * STUDENT NOTE: You don't need to do binary math! The macros handle
 * all the bit shifting. Just understand what values to pass in.
 */

// Macros for SETTING NTP packet fields (for requests)
#define SET_NTP_LI_VN_MODE(packet, li, vn, mode) \
    ((packet)->li_vn_mode = ((li) << 6) | ((vn) << 3) | (mode))

// Macros for GETTING NTP packet fields (for responses)
#define GET_NTP_LI(packet)      (((packet)->li_vn_mode >> 6) & 0x03)
#define GET_NTP_VN(packet)      (((packet)->li_vn_mode >> 3) & 0x07)
#define GET_NTP_MODE(packet)    ((packet)->li_vn_mode & 0x07)

// The servers returned dispersion and delay is encoded such that the 
// upper 16 bits are seconds, and the lower 16 bits are fractions
// of a second, here are helpers to decode Q16.16 encoded numbers
#define GET_NTP_Q1616_SEC(d) (d >> 16)
#define GET_NTP_Q1616_FRAC(d) (d & 0xFFFF0000)
// Returns a double number in seconds
#define GET_NTP_Q1616_TS(d) ( \
    (double)((GET_NTP_Q1616_SEC(d) * 1000) + \
             (GET_NTP_Q1616_FRAC(d) / 65535.0)) \
)

// Improved utility macros for time conversion
#define NTP_TO_UNIX_SECONDS(ntp_sec)    ((ntp_sec) - NTP_EPOCH_OFFSET)
#define UNIX_TO_NTP_SECONDS(unix_sec)   ((unix_sec) + NTP_EPOCH_OFFSET)

// Legacy macros for compatibility
#define NTP_TO_UNIX(ntp_sec)    NTP_TO_UNIX_SECONDS(ntp_sec)
#define UNIX_TO_NTP(unix_sec)   UNIX_TO_NTP_SECONDS(unix_sec)

/*
 * =============================================================================
 * COMMON EPOCH MISTAKES AND HOW TO SPOT THEM
 * =============================================================================
 * 
 * Mistake 1: Forgot to convert epochs
 * - Symptom: Times show up as 1900 or way in the future
 * - Fix: Always add/subtract NTP_EPOCH_OFFSET
 * 
 * Mistake 2: Converting in wrong direction  
 * - Symptom: Times are 70 years off
 * - Fix: NTP times are BIGGER, Unix times are SMALLER
 * 
 * Mistake 3: Converting twice
 * - Symptom: Times are 140 years off
 * - Fix: Convert only once at the boundary between systems
 * 
 * Quick Sanity Check:
 * - Valid NTP time for 2025: ~3.9 billion seconds
 * - Valid Unix time for 2025: ~1.7 billion seconds
 * - If your numbers don't match these ranges, check your conversion!
 */

/*
 * =============================================================================
 * STUDENT FUNCTION DECLARATIONS - GROUPED BY IMPLEMENTATION ORDER
 * =============================================================================
 * 
 * Complete these functions in the suggested order below. Each group builds
 * on the previous group's functionality.
 */

/*
 * GROUP 1: TIME CONVERSION FUNCTIONS
 * Start with these - they're fundamental to everything else
 * C Library Functions: gettimeofday(), localtime(), gmtime(), snprintf()
 */

// Get current system time in NTP timestamp format
void get_current_ntp_time(ntp_timestamp_t *ntp_ts);

// Convert NTP timestamp to double for mathematical operations
double ntp_time_to_double(const ntp_timestamp_t* timestamp);

// Convert NTP timestamp to human-readable string
void ntp_time_to_string(const ntp_timestamp_t *ntp_ts, char *buffer, size_t buffer_size, int local);

// Print NTP timestamp with label
void print_ntp_time(const ntp_timestamp_t *ts, const char* label, int local);

/*
 * GROUP 2: NETWORK BYTE ORDER FUNCTIONS  
 * Essential for network protocol compliance
 * C Library Functions: htonl(), ntohl()
 */

// Convert NTP timestamp from host to network byte order
void ntp_ts_to_net(ntp_timestamp_t* timestamp);

// Convert NTP timestamp from network to host byte order
void ntp_ts_to_host(ntp_timestamp_t* timestamp);

// Convert entire NTP packet from host to network byte order
void ntp_to_net(ntp_packet_t* packet);

// Convert entire NTP packet from network to host byte order
void ntp_to_host(ntp_packet_t* packet);

/*
 * GROUP 3: NTP PACKET CONSTRUCTION
 * Build the request packet that gets sent to the server
 * C Library Functions: memset(), memcpy()
 */

// Build an NTP client request packet
int build_ntp_request(ntp_packet_t* packet);

/*
 * GROUP 4: PROTOCOL ANALYSIS FUNCTIONS
 * Parse responses and perform NTP calculations
 * C Library Functions: inet_ntop(), mathematical operations
 */

// Decode the reference_id field based on stratum level
int decode_reference_id(uint8_t stratum, uint32_t ref_id, char *buff, int buff_sz);

// Calculate time offset and delay using NTP algorithm
int calculate_ntp_offset(const ntp_packet_t* request, 
                        const ntp_packet_t* response,
                        const ntp_timestamp_t* recv_time, 
                        ntp_result_t* result);

/*
 * GROUP 5: DISPLAY FUNCTIONS
 * Format and print results for the user
 * C Library Functions: printf(), sprintf()
 */

// Print NTP packet contents in human-readable format
void print_ntp_packet_info(const ntp_packet_t* packet, const char* label, int packet_type);

// Print calculated NTP results with quality assessment
void print_ntp_results(const ntp_result_t* result);

/*
 * =============================================================================
 * DEBUGGING HELPER FUNCTIONS
 * =============================================================================
 */

// Debug function to show bit field breakdown
void debug_print_bit_fields(const ntp_packet_t* packet);

// Demonstration function for epoch conversion
void demonstrate_epoch_conversion(void);

/*
 * =============================================================================
 * IMPLEMENTATION NOTES FOR STUDENTS
 * =============================================================================
 * 
 * NETWORK BYTE ORDER:
 * - All multi-byte integers in network protocols use big-endian byte order
 * - Use htonl() to convert TO network order (host to network long)
 * - Use ntohl() to convert FROM network order (network to host long)
 * - Single bytes (uint8_t) don't need conversion
 * 
 * NTP TIME REPRESENTATION:
 * - NTP uses 1900-01-01 as epoch, Unix uses 1970-01-01
 * - NTP_EPOCH_OFFSET = seconds between these epochs
 * - NTP fractional seconds: fraction / 2^32 gives decimal fraction
 * - To convert microseconds to NTP fraction: (usec * 2^32) / 1,000,000
 * 
 * REFERENCE ID DECODING:
 * - If stratum >= 2: reference_id contains an IPv4 address
 * - If stratum < 2: reference_id contains 4 ASCII characters (like "NIST")
 * - If reference_id == 0: should display "NONE"
 * 
 * NTP ALGORITHM (Four Timestamps):
 * T1 = Client request transmission time
 * T2 = Server request reception time  
 * T3 = Server response transmission time
 * T4 = Client response reception time
 * 
 * Calculations:
 * delay = (T4 - T1) - (T3 - T2)
 * offset = ((T2 - T1) + (T3 - T4)) / 2
 * 
 * DEBUGGING TIPS:
 * - Print intermediate values during calculations
 * - Use debug_print_bit_fields() to verify packet fields
 * - Use demonstrate_epoch_conversion() to understand time conversion
 * - Verify byte order conversions by examining raw packet bytes
 * - Test with known NTP servers like time.nist.gov
 * - Check that timestamps are reasonable (not 1900 or far future)
 */

// Instructor provided function prototypes - DO NOT MODIFY
void usage(const char* progname);
int resolve_hostname(const char* hostname, char* ip_str);
int create_udp_socket();
int send_ntp_request(int sockfd, const struct sockaddr_in* server_addr, 
                    const ntp_packet_t* packet);
int recv_ntp_response(int sockfd, ntp_packet_t* packet);
int query_ntp_server(const char* server_name, const char* ip_str);

#endif