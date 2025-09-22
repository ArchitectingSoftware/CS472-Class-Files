# TCP Echo Program - Student Guide

This TCP echo program demonstrates the fundamental concepts of TCP/IP network programming in C. It implements both a client and server that communicate using a reliable, connection-oriented protocol with the same binary PDU format as the UDP version.

## Program Overview

The program can run in two modes:
- **Client Mode**: Establishes a persistent connection to the server and allows interactive messaging
- **Server Mode**: Listens for client connections and handles multiple clients sequentially

## TCP vs UDP: Key Differences

### Connection-Oriented Nature
Unlike UDP's connectionless approach, TCP requires:
1. **Connection Establishment**: Three-way handshake before data transfer
2. **Connection Management**: Persistent connection state on both ends
3. **Connection Termination**: Graceful connection closure process

### Reliability Features
TCP provides what UDP doesn't:
- **Guaranteed Delivery**: Lost packets are automatically retransmitted
- **Ordered Delivery**: Messages arrive in the order they were sent
- **Flow Control**: Prevents sender from overwhelming receiver
- **Error Detection**: Built-in checksums and acknowledgments

## TCP Communication Flow

### 1. Socket Creation

Both client and server start by creating a TCP socket:

```c
int sockfd = socket(AF_INET, SOCK_STREAM, 0);
```

**Key Differences from UDP:**
- `SOCK_STREAM`: Stream socket (TCP) vs `SOCK_DGRAM` (UDP)
- **Reliable, ordered byte stream** vs independent datagrams
- **Connection-oriented** vs connectionless

### 2. Server: Binding and Listening

The server must bind to an address and then listen for connections:

```c
// Bind to address (same as UDP)
bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

// Listen for incoming connections (TCP only)
listen(sockfd, BACKLOG);
```

**TCP-Specific Concepts:**
- **`listen()`**: Marks socket as passive, ready to accept connections
- **Backlog Queue**: Maximum number of pending connections (BACKLOG = 5)
- **Connection Queue**: OS maintains queue of incoming connection requests

### 3. Connection Establishment

**Client Side:**
```c
connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```

**Server Side:**
```c
int client_sock = accept(sockfd, (struct sockaddr*)&client_addr, &client_addr_len);
```

**Three-Way Handshake (handled by OS):**
1. Client sends SYN (synchronize)
2. Server responds with SYN-ACK (synchronize-acknowledge)
3. Client sends ACK (acknowledge)

**Key Points:**
- **`connect()`**: Initiates connection from client to server
- **`accept()`**: Server accepts incoming connection, returns new socket for this client
- **Blocking Operations**: Both connect() and accept() block until completion
- **New Socket**: accept() creates a new socket specifically for this client

### 4. Data Transfer: Stream vs Message Boundaries

**TCP is Stream-Based, Not Message-Based:**
```c
// Sending - may not send all bytes at once
ssize_t send(int sockfd, const void *buf, size_t len, int flags);

// Receiving - may not receive complete message
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
```

**Critical TCP Concept - Partial Sends/Receives:**
- **Partial Sends**: `send()` may send fewer bytes than requested
- **Partial Receives**: `recv()` may return partial data
- **No Message Boundaries**: TCP doesn't preserve message boundaries like UDP

**Our Solution - Helper Functions:**
```c
ssize_t send_all(int sockfd, const char* buffer, size_t length);
ssize_t recv_pdu(int sockfd, char *message, size_t max_length);
```

### 5. Protocol Data Units (PDUs) Over TCP

Since TCP is stream-based, we must implement our own message framing:

**PDU Structure (Same as UDP):**
```c
typedef struct {
    uint16_t msg_len;     // Message length in network byte order
    uint8_t  msg_data[1]; // Variable length message data
} echo_pdu_t;
```

**Wire Format:**
```
[2 bytes: length][N bytes: message data]
```

**Why PDUs Are Essential in TCP:**
- **Stream Nature**: TCP provides a continuous byte stream, not discrete messages
- **Message Boundaries**: We must define where one message ends and another begins
- **Length Prefixing**: Standard technique used by real protocols (HTTP, TLS, etc.)

### 6. Connection Termination and Detection

## How the Server Detects Client Disconnection

### Normal Client Exit ("exit" command)
```c
// Client sends normal message then closes socket
close(client_sockfd);
```

**Server Detection:**
```c
pdu_len = recv_pdu(client_sock, extracted_msg, sizeof(extracted_msg));
if (pdu_len == 0) {
    printf("Client disconnected gracefully.\n");
    // Server continues, waits for next client
}
```

### Abnormal Client Exit (Ctrl+C, crash)
**What Happens:**
1. Client process terminates immediately
2. OS automatically closes client socket
3. TCP sends FIN (finish) packet to server
4. Server's next `recv()` returns 0 (EOF)

**Server Response:**
```c
if (pdu_len == 0) {
    printf("Client disconnected.\n");
    break; // Exit client loop, wait for next client
}
```

## How the Client Detects Server Disconnection

### Server Shutdown ("exit server" command)
**Graceful Shutdown:**
1. Server sends response message
2. Server closes client socket
3. Client detects connection closure on next receive

### Server Crash or Network Failure
**What Happens:**
- Server process terminates unexpectedly
- Client's `send()` or `recv()` operations fail
- Client gets error codes or zero bytes received

**Client Detection:**
```c
if (send_pdu(sockfd, input_buffer) < 0) {
    printf("Error sending message. Server may have disconnected.\n");
    break;
}

pdu_len = recv_pdu(sockfd, extracted_msg, sizeof(extracted_msg));
if (pdu_len <= 0) {
    printf("Server closed connection.\n");
    break;
}
```

## Key C Library Functions

### TCP-Specific Socket Functions

#### `listen(int sockfd, int backlog)`
- **Purpose**: Marks socket as passive, ready to accept connections
- **TCP Only**: Not used in UDP (connectionless)
- **Backlog**: Maximum number of pending connections in queue
- **Returns**: 0 on success, -1 on error

#### `accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)`
- **Purpose**: Accepts an incoming connection request
- **Blocking**: Waits until a client connects
- **Returns**: New socket descriptor for the client connection
- **Key Point**: Creates a new socket specifically for this client

#### `connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`
- **Purpose**: Initiates connection to server
- **Client Only**: Only clients call connect()
- **Blocking**: Waits until connection is established or fails
- **Three-Way Handshake**: Triggers TCP connection establishment

### Data Transfer Functions

#### `send(int sockfd, const void *buf, size_t len, int flags)`
- **Purpose**: Sends data over TCP connection
- **Stream-Based**: May send fewer bytes than requested (partial send)
- **Returns**: Number of bytes actually sent, or -1 on error
- **Critical**: Must handle partial sends in real applications

#### `recv(int sockfd, void *buf, size_t len, int flags)`
- **Purpose**: Receives data from TCP connection
- **Stream-Based**: May receive partial data
- **Returns**: Number of bytes received, 0 if connection closed, -1 on error
- **EOF Condition**: Return value of 0 indicates connection closed

### Helper Functions We Implemented

#### `send_all(int sockfd, const char* buffer, size_t length)`
- **Purpose**: Ensures all bytes are sent (handles partial sends)
- **Implementation**: Loops until all data is transmitted
- **Real-World**: Essential pattern in TCP programming

#### `recv_pdu(int sockfd, char *message, size_t max_length)`
- **Purpose**: Receives a complete PDU message
- **Two-Phase**: First receives length header, then message data
- **Framing**: Solves TCP's lack of message boundaries

### Socket Configuration (Same as UDP)

#### `setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)`
- **SO_REUSEADDR**: Allows immediate rebinding (crucial for development)
- **TCP Specific Options**: TCP_NODELAY (disable Nagle's algorithm), TCP_KEEPALIVE

### Address Functions (Same as UDP)

#### Network Byte Order Conversion
- **`htons()`/`ntohs()`**: Convert port numbers
- **`inet_pton()`**: Convert IP address strings to binary
- **`inet_ntop()`**: Convert binary IP addresses to strings

## Connection State Management

### Server Connection Lifecycle

```
1. socket() → Create listening socket
2. bind()   → Bind to local address  
3. listen() → Mark as passive socket
4. accept() → Wait for/accept client (blocks)
   ↓
5. Client connects → New client socket created
6. Communication loop with this client
7. Client disconnects → Close client socket
8. Return to accept() for next client
   ↓
9. "exit server" received → Close listening socket, exit
```

### Client Connection Lifecycle

```
1. socket()  → Create client socket
2. connect() → Connect to server (blocks)
   ↓
3. Connected → Interactive communication loop
4. User types "exit" or "exit server" → Close socket
5. Process exits
```

## Program Flow Diagrams

### Server Flow (Multi-Client)
1. Create listening socket
2. Set SO_REUSEADDR option
3. Bind to local address
4. Listen for connections
5. **Outer Loop: Accept clients**
   - Accept client connection
   - Log client information
   - **Inner Loop: Handle this client**
     - Receive PDU from client
     - Process message (check for "exit server")
     - Send response PDU
     - Handle client disconnection
   - Close client socket
   - If not "exit server", wait for next client
6. Close listening socket and exit

### Client Flow (Interactive)
1. Create client socket
2. Connect to server
3. Display prompt and instructions
4. **Interactive Loop:**
   - Get user input
   - Check for exit conditions
   - Send message PDU to server
   - Receive response PDU from server
   - Display server response
   - Handle server disconnection
5. Close socket and exit

## Advanced TCP Concepts Demonstrated

### Connection-Oriented Reliability
- **Automatic Retransmission**: TCP handles packet loss transparently
- **Congestion Control**: TCP adapts to network conditions
- **Flow Control**: Prevents buffer overflow at receiver

### Stream Processing
- **No Message Boundaries**: Unlike UDP, TCP doesn't preserve message boundaries
- **Buffering**: Data may be buffered by the OS before transmission
- **Partial Operations**: Both send() and recv() can handle partial data

### Error Detection and Recovery
- **Connection Failures**: Detect and handle broken connections
- **Network Partitions**: Handle temporary network failures
- **Graceful Degradation**: Continue serving other clients when one fails

## Educational Benefits

### TCP Concepts Demonstrated
- **Connection Establishment**: Three-way handshake (handled by OS)
- **Reliable Data Transfer**: Guaranteed delivery and ordering
- **Connection Management**: Persistent connection state
- **Graceful Connection Termination**: Proper cleanup procedures
- **Multi-Client Handling**: Sequential client processing

### Network Programming Patterns
- **Accept Loop**: Standard server pattern for handling multiple clients
- **Partial Send/Receive Handling**: Critical for robust TCP applications
- **Message Framing**: Length-prefixed messages over streams
- **Error Handling**: Comprehensive network error management
- **Resource Management**: Proper socket lifecycle management

### Real-World Protocols
- **HTTP**: Uses similar request/response pattern over TCP
- **SMTP/IMAP**: Email protocols demonstrate persistent connections
- **FTP**: Shows control and data connections
- **SSH/TLS**: Encrypted communication over TCP

## Common TCP Issues and Solutions

### "Address already in use"
- **Cause**: Socket in TIME_WAIT state from previous connection
- **Solution**: SO_REUSEADDR option (implemented)
- **Note**: More common in TCP due to connection state

### "Connection refused"
- **Cause**: No server listening, or server not yet ready
- **Solution**: Ensure server is running and has called listen()

### "Connection reset by peer"
- **Cause**: Remote end closed connection abruptly
- **Solution**: Handle gracefully, don't treat as fatal error

### Partial Data Transfer
- **Issue**: send()/recv() may not transfer all requested data
- **Solution**: Use send_all() and proper PDU framing (implemented)

### Resource Leaks
- **Issue**: Not closing client sockets in multi-client server
- **Solution**: Always close client sockets after handling (implemented)

## Performance Considerations

### Nagle's Algorithm
- **Default**: TCP combines small packets to improve efficiency
- **Impact**: May add latency for interactive applications
- **Solution**: TCP_NODELAY socket option if needed

### Keep-Alive
- **Purpose**: Detect dead connections
- **Configuration**: TCP_KEEPALIVE socket options
- **Use Case**: Long-lived connections

### Buffer Management
- **Send Buffer**: OS buffers outgoing data
- **Receive Buffer**: OS buffers incoming data
- **Tuning**: SO_SNDBUF and SO_RCVBUF socket options

## Building and Running

### Compilation
```bash
make
```

### Running the Server
```bash
./tcp_echo --server
./tcp_echo --server --port 8080 --addr 0.0.0.0
```

### Running Multiple Clients
```bash
# Terminal 1: Server
./tcp_echo --server

# Terminal 2: Client 1
./tcp_echo --client

# Terminal 3: Client 2 (after Client 1 exits)
./tcp_echo --client
```

### Testing Server Shutdown
```bash
# In any client, type:
exit server
```

## Extended Learning Opportunities

### Concurrent Server
- **Current**: Sequential (one client at a time)
- **Enhancement**: Use fork() or threads for concurrent clients
- **Concepts**: Process/thread management, shared resources

### Advanced Protocol Features
- **Authentication**: Add user login capabilities
- **Encryption**: Integrate TLS/SSL for secure communication
- **Compression**: Add data compression to reduce bandwidth

### Performance Optimization
- **Benchmarking**: Measure throughput and latency
- **Tuning**: Optimize buffer sizes and socket options
- **Monitoring**: Add connection statistics and logging

### Error Recovery
- **Reconnection**: Automatic client reconnection on failure
- **Persistence**: Store and recover connection state
- **Load Balancing**: Distribute clients across multiple servers

This TCP echo program provides a comprehensive foundation for understanding connection-oriented network programming and demonstrates the key differences between TCP and UDP protocols in practical, hands-on scenarios.