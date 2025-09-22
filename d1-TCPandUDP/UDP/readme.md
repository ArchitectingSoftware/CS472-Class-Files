# UDP Echo Program - Student Guide

This UDP echo program demonstrates the fundamental concepts of UDP/IP network programming in C. It implements both a client and server that communicate using a custom binary protocol with length-prefixed messages.

## Program Overview

The program can run in two modes:
- **Client Mode**: Sends a message to the server and receives an echo response
- **Server Mode**: Listens for client messages and echoes them back with a prefix

## UDP Communication Flow

### 1. Socket Creation

Both client and server start by creating a UDP socket:

```c
int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
```

**Key Points:**
- `AF_INET`: IPv4 address family
- `SOCK_DGRAM`: Datagram socket (UDP)
- `0`: Let the system choose the appropriate protocol (UDP for SOCK_DGRAM)

### 2. Server: Address Binding

The server must bind to a specific address and port to receive messages:

```c
struct sockaddr_in server_addr;
// ... configure server_addr ...
bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

**Why Binding is Necessary:**
- Tells the OS which port the server will listen on
- Associates the socket with a specific network interface
- Allows clients to know where to send messages

### 3. Socket Options: SO_REUSEADDR

The program sets the `SO_REUSEADDR` option:

```c
int reuse = 1;
setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
```

**Why This Matters:**
- **Development Convenience**: Allows immediately restarting the server without waiting
- **TIME_WAIT State**: When a socket closes, it enters TIME_WAIT state (typically 60 seconds)
- **Address Already in Use**: Without SO_REUSEADDR, you get this error when restarting quickly
- **Best Practice**: Essential for server development and testing

### 4. Client: Server Address Configuration

The client configures the server's address but doesn't bind (it uses an ephemeral port):

```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(port);
inet_pton(AF_INET, addr, &server_addr.sin_addr);
```

**Key Concepts:**
- **No Binding**: Client doesn't need to bind; OS assigns a random port
- **Target Address**: Client must know server's IP and port
- **Network Byte Order**: Port must be converted to network byte order

### 5. Sending and Receiving Data

**Client Sending:**
```c
sendto(sockfd, pdu_buffer, pdu_length, 0, 
       (struct sockaddr*)&server_addr, sizeof(server_addr));
```

**Server Receiving:**
```c
recvfrom(sockfd, buffer, buffer_size, 0,
         (struct sockaddr*)&client_addr, &client_addr_len);
```

**Server Responding:**
```c
sendto(sockfd, response_pdu, response_length, 0,
       (struct sockaddr*)&client_addr, client_addr_len);
```

**UDP Characteristics:**
- **Connectionless**: No connection establishment needed
- **Stateless**: Each message is independent
- **Unreliable**: No guarantee of delivery, ordering, or duplicate protection
- **Message Boundaries**: Each sendto/recvfrom is one complete message

### 6. Socket Cleanup

Always close sockets when done:

```c
close(sockfd);
```

## Binary Protocol Design

This program implements a realistic network protocol instead of using simple strings:

### PDU (Protocol Data Unit) Structure

```c
typedef struct {
    uint16_t msg_len;     // Length of message data (network byte order)
    uint8_t  msg_data[1]; // Variable length message data
} echo_pdu_t;
```

### Wire Format

```
[2 bytes: length][N bytes: message data]
```

**Benefits of Length-Prefixed Protocol:**
- **No Null Termination**: Binary data safe, more efficient
- **Known Message Boundaries**: Receiver knows exactly how much data to expect
- **Network Realistic**: How real protocols work (HTTP, TCP, etc.)
- **Validation**: Can verify received data matches expected format

## Key C Library Functions

### Socket Functions

#### `socket(int domain, int type, int protocol)`
- **Purpose**: Creates a communication endpoint
- **Parameters**: 
  - `domain`: AF_INET (IPv4)
  - `type`: SOCK_DGRAM (UDP)
  - `protocol`: 0 (auto-select)
- **Returns**: Socket file descriptor or -1 on error

#### `bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`
- **Purpose**: Associates socket with a local address
- **Use**: Server only (client uses ephemeral port)
- **Critical**: Required for server to receive messages

#### `sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen)`
- **Purpose**: Sends UDP datagram to specified address
- **UDP Specific**: Must specify destination address each time
- **Returns**: Number of bytes sent or -1 on error

#### `recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)`
- **Purpose**: Receives UDP datagram and source address
- **UDP Specific**: Provides sender's address for response
- **Returns**: Number of bytes received or -1 on error

#### `close(int fd)`
- **Purpose**: Closes socket and releases resources
- **Important**: Always clean up sockets

### Socket Configuration Functions

#### `setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)`
- **Purpose**: Sets socket options
- **Common Use**: `SO_REUSEADDR` for server development
- **Levels**: `SOL_SOCKET` (socket level), `IPPROTO_TCP`, etc.

### Address Conversion Functions

#### `inet_pton(int af, const char *src, void *dst)`
- **Purpose**: Converts presentation (string) format to network format
- **Example**: "192.168.1.1" → binary IPv4 address
- **af**: Address family (AF_INET for IPv4)
- **Returns**: 1 on success, 0 if invalid format, -1 on error

#### `inet_ntop(int af, const void *src, char *dst, socklen_t size)`
- **Purpose**: Converts network format to presentation (string) format  
- **Example**: Binary IPv4 → "192.168.1.1"
- **Use**: Converting client addresses for logging
- **Returns**: Pointer to result string or NULL on error

#### `htons(uint16_t hostshort)` / `ntohs(uint16_t netshort)`
- **Purpose**: Convert between host and network byte order for 16-bit values
- **htons**: Host to Network Short (for port numbers)
- **ntohs**: Network to Host Short (for port numbers)
- **Why Needed**: Different CPU architectures use different byte ordering

### Memory and String Functions

#### `memset(void *s, int c, size_t n)`
- **Purpose**: Fills memory with constant byte
- **Common Use**: Zero out address structures
- **Example**: `memset(&addr, 0, sizeof(addr));`

#### `memcpy(void *dest, const void *src, size_t n)`
- **Purpose**: Copies memory from source to destination
- **Use**: Copying binary data in PDU construction
- **Safe**: Works with binary data (no null termination needed)

## Program Flow Diagrams

### Client Flow
1. Create socket
2. Configure server address
3. Create PDU from message string
4. Send PDU to server
5. Receive response PDU from server
6. Extract message from response PDU
7. Display result
8. Close socket

### Server Flow
1. Create socket
2. Set SO_REUSEADDR option
3. Configure and bind to local address
4. Enter receive loop:
   - Receive PDU from client
   - Extract message from PDU
   - Check for "exit" command
   - Create response message
   - Create response PDU
   - Send response PDU to client
5. Close socket

## Educational Benefits

### UDP Concepts Demonstrated
- **Connectionless Communication**: No handshake required
- **Message-Based**: Each send/receive is one complete message
- **Address Handling**: Client and server address management
- **Error Handling**: Network programming error patterns

### Network Programming Best Practices
- **Binary Protocols**: Length-prefixed messages instead of strings
- **Network Byte Order**: Proper endianness handling
- **Socket Options**: Development-friendly configuration
- **Resource Management**: Proper socket cleanup
- **Error Checking**: Comprehensive error handling

### C Programming Concepts
- **Structure Overlays**: Mapping structs onto byte buffers
- **Pointer Arithmetic**: Working with variable-length data
- **Type Casting**: Converting between data types safely
- **Memory Management**: Buffer handling and bounds checking

## Building and Running

### Compilation
```bash
make
```

### Running the Server
```bash
./udp_echo --server
./udp_echo --server --port 8080 --addr 0.0.0.0 "SERVER"
```

### Running the Client
```bash
./udp_echo --client
./udp_echo --client --port 8080 --addr 192.168.1.100 "Hello World"
./udp_echo --client --addr 127.0.0.1 "exit"  # Shuts down server
```

## Common Issues and Solutions

### "Address already in use"
- **Cause**: Previous server instance still has socket in TIME_WAIT
- **Solution**: SO_REUSEADDR option (already implemented)
- **Alternative**: Wait 60 seconds or use different port

### "Connection refused"
- **Cause**: No server listening on specified address/port
- **Solution**: Ensure server is running first

### "Invalid argument"
- **Cause**: Usually malformed address or invalid parameters
- **Solution**: Check IP address format and port number range

### Message Truncation
- **Cause**: Message longer than buffer size
- **Solution**: Increase BUFFER_SIZE or implement proper bounds checking

## Extended Learning

### Next Steps
1. **Add multiple client support**: Server handles multiple clients
2. **Implement reliability**: Add acknowledgments and retransmission
3. **Add encryption**: Secure the communication
4. **Performance testing**: Measure throughput and latency
5. **IPv6 support**: Extend to work with IPv6 addresses

### Related Protocols
- **TCP**: Connection-oriented, reliable counterpart
- **SCTP**: Modern alternative with message boundaries
- **QUIC**: UDP-based transport with TCP-like reliability

This UDP echo program provides a solid foundation for understanding network programming concepts and can be extended in many directions for further learning.