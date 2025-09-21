# What Time is It? - Network Time Protocol (NTP) Client Assignment

## Overview

Ever wondered how your computer knows the exact time? When you connect to the internet, your system automatically synchronizes with time servers around the world using the Network Time Protocol (NTP). In this assignment, you'll build your own NTP client to understand how network time synchronization works at the protocol level.

## Introduction to Network Time Protocol (NTP)

### What is NTP?

The Network Time Protocol (NTP) is one of the oldest Internet protocols still in use today, designed to synchronize computer clocks across networks. NTP can typically maintain time accuracy within tens of milliseconds over the public Internet and can achieve better than one millisecond accuracy in local area networks.

### How NTP Works - The Big Picture

NTP operates on a hierarchical system of time sources called **strata**:

- **Stratum 0**: Reference clocks (atomic clocks, GPS receivers) - not directly on the network
- **Stratum 1**: Primary time servers directly connected to Stratum 0 sources
- **Stratum 2**: Secondary servers that sync with Stratum 1 servers
- **Stratum 3+**: Additional levels, each syncing with the level above

This hierarchy prevents timing loops and ensures everyone has a clear path back to authoritative time sources. The lower the stratum number, the closer you are to the reference clock and typically the more accurate the time.

**Important**: When building an NTP client request, we set the stratum field to **0** (unspecified). This might seem counterintuitive, but it's the correct behavior! A stratum of 0 in a client request means "I don't know what stratum I am" - essentially saying "I'm asking for time, not providing it." The server will respond with its own stratum level (usually 1-4 for public servers), telling you how many hops away from the reference clock it is.

If we incorrectly set a stratum value like 2 in our request, it would imply we're already a synchronized time server ourselves, which would be misleading and could confuse the time hierarchy.

When your computer requests time synchronization, it exchanges timestamps with NTP servers to calculate two critical values:
- **Clock Offset**: How far off your local time is from the server's time
- **Round-trip Delay**: How long network packets take to travel between client and server

### The NTP Algorithm - Four Timestamps

NTP uses a simple but elegant algorithm based on four timestamps:

1. **T1**: Client transmit time (when you send the request)
2. **T2**: Server receive time (when the server gets your request)
3. **T3**: Server transmit time (when the server sends the response)
4. **T4**: Client receive time (when you get the response)

From these four timestamps, NTP calculates:
- **Delay** = (T4 - T1) - (T3 - T2)
- **Offset** = ((T2 - T1) + (T3 - T4)) / 2

## Understanding the NTP Packet Format

NTP uses a fixed 48-byte packet format sent over UDP port 123. Here are the key fields you need to understand:

### Header Fields (First 4 bytes)

The first byte of every NTP packet contains three fields packed together using bit manipulation. Understanding this is crucial for correctly building NTP packets.

**The Packed Byte: `li_vn_mode`**

NTP packs three small fields into one byte for efficiency:
- **Leap Indicator (2 bits)**: Clock sync status - use 3 for client requests
- **Version Number (3 bits)**: Always 4 for modern NTP
- **Mode (3 bits)**: 3 = client request, 4 = server response

**STUDENTS: Use the provided macros - no manual bit math required!**
```c
SET_NTP_LI_VN_MODE(packet, NTP_LI_UNSYNC, NTP_VERSION, NTP_MODE_CLIENT);
```

**Visual Example - Bit Packing Walkthrough:**

Let's build li_vn_mode for a client request:
- Leap Indicator (LI) = 3 (unsynchronized)  
- Version (VN) = 4 (NTP version 4)
- Mode = 3 (client request)

Step 1: Convert to binary
- LI = 3  = 11 (binary)
- VN = 4  = 100 (binary) 
- Mode = 3 = 011 (binary)

Step 2: Pack into single byte
```
Bit positions: 7 6 | 5 4 3 | 2 1 0
Values:        1 1 | 1 0 0 | 0 1 1
Result: 11100011 = 0xE3 = 227 decimal
```

Step 3: Use the macro (students don't do math manually!)
```c
SET_NTP_LI_VN_MODE(packet, 3, 4, 3);
// This sets packet->li_vn_mode = 0xE3
```

**Why pack multiple fields into one byte?** Network protocols minimize packet size for efficiency. Rather than using separate bytes for each small field, NTP packs related information together. Your code uses macros to handle this complexity.

- **Leap Indicator (2 bits)**: Warns about leap seconds
  - **0 = No warning**: Normal operation
  - **1 = Last minute has 61 seconds**: Leap second will be added
  - **2 = Last minute has 59 seconds**: Leap second will be deleted  
  - **3 = Clock unsynchronized**: What clients should send (means "I don't know")
  
  *What's a leap second?* Earth's rotation isn't perfectly constant, so occasionally we add or remove a second to keep atomic time aligned with astronomical time. Most students can ignore this - just use "3" for client requests.

- **Version Number (3 bits)**: NTP protocol version
  - Always **4** for modern NTP (versions 1-3 are obsolete)

- **Mode (3 bits)**: What type of NTP message this is
  - **3 = Client request**: "I'm asking for time"
  - **4 = Server response**: "Here's the time you requested"
  - Other modes exist for server-to-server communication

**Other Important Fields:**

- **Stratum (8 bits)**: Distance from reference clock
  - **0 = Unspecified** (for client requests - means "I don't know my level")
  - **1 = Primary server** (directly connected to atomic clock/GPS)
  - **2-15 = Secondary servers** (each level further from reference)

- **Poll Interval (8 bits)**: Maximum time between messages (log₂ seconds)
  - **6 = 64 seconds** (2⁶ = 64, a reasonable default for clients)

- **Precision (8 bits)**: Clock precision (log₂ seconds, signed)
  - **-20 = ~1 microsecond** (2⁻²⁰ ≈ 1 microsecond precision)

### Timestamp Fields (32 bytes)

**Understanding Time Epochs**

An **epoch** is simply the starting point for counting time in a computer system. Think of it like "Year Zero" for a calendar system. Different systems chose different starting points:

**Unix Time (used by your computer):**
- Starts: January 1, 1970, 00:00:00 UTC
- Right now: ~1727789000 seconds since 1970
- Why 1970? Unix was developed in early 1970s

**NTP Time (used by time servers):**  
- Starts: January 1, 1900, 00:00:00 UTC
- Right now: ~3936777800 seconds since 1900
- Why 1900? Covers entire 20th century and beyond

**Timeline Visualization:**
```
1900 -------- 1970 -------- 2025
 |              |              |
NTP             Unix           Now
epoch           epoch          
starts          starts         
 |<-- 70 years ->|<-- 55 years ->|
 |<------- 125 years total ----->|
```

**The Conversion:**
- NTP_EPOCH_OFFSET = 2,208,988,800 seconds = exactly 70 years
- Your Code Will Do This:
  - unix_time = ntp_time - 2208988800    // NTP to Unix
  - ntp_time = unix_time + 2208988800    // Unix to NTP

**MEMORY TRICK: NTP time is always BIGGER (it started counting earlier)**

**Why This Matters**: When you get an NTP timestamp, you must convert it to Unix time by subtracting the difference between epochs (2,208,988,800 seconds = 70 years). Your code will frequently convert between these two time systems.

**NTP Time Representation:**
- **32-bit seconds**: Whole seconds since 1900
- **32-bit fraction**: Fractional seconds (1/2³² increments ≈ 232 picoseconds)

This gives NTP incredible precision while covering a 136-year time range.

The packet contains four timestamps:
- **Reference Timestamp**: When the server's clock was last set
- **Origin Timestamp**: Client's transmit time (T1)
- **Receive Timestamp**: Server's receive time (T2)
- **Transmit Timestamp**: Server's transmit time (T3)

### Network Byte Order

**Critical**: All multi-byte fields must be in network byte order (big-endian). Use `htonl()` when creating packets and `ntohl()` when reading them.

### Common Mistakes and How to Spot Them

**Epoch Conversion Mistakes:**

1. **Forgot to convert epochs**
   - Symptom: Times show up as 1900 or way in the future
   - Fix: Always add/subtract NTP_EPOCH_OFFSET

2. **Converting in wrong direction**  
   - Symptom: Times are 70 years off
   - Fix: NTP times are BIGGER, Unix times are SMALLER

3. **Converting twice**
   - Symptom: Times are 140 years off
   - Fix: Convert only once at the boundary between systems

**Quick Sanity Check:**
- Valid NTP time for 2025: ~3.9 billion seconds
- Valid Unix time for 2025: ~1.7 billion seconds
- If your numbers don't match these ranges, check your conversion!

## Assignment Objectives

In this assignment, you will implement the core NTP protocol functionality while we handle all the networking complexity for you. Your job is to:

1. **Build proper NTP request packets** by filling in the correct header fields and timestamps
2. **Parse NTP response packets** by extracting and converting timestamp data
3. **Implement the NTP algorithm** to calculate time offset and network delay
4. **Handle time format conversions** between NTP time (since 1900) and Unix time (since 1970)

### What's Provided

- Complete command-line argument processing
- Socket creation and network communication
- DNS hostname resolution
- Error handling and program structure
- Comprehensive header file with all protocol definitions
- Debugging helper functions

### What You Must Implement

- `build_ntp_request()`: Create a properly formatted NTP request packet
- `ntp_timestamp_to_double()`: Convert NTP timestamps to Unix time
- `double_to_ntp_timestamp()`: Convert Unix time to NTP format
- `calculate_ntp_offset()`: Implement the core NTP synchronization algorithm
- `print_ntp_packet_info()`: Display packet contents for debugging
- `print_ntp_results()`: Show final results with quality assessment

## Learning Objectives

By completing this assignment, you will:

### Technical Skills
- **Protocol Implementation**: Understand how binary network protocols work in practice
- **Bit Manipulation**: Master packed C structures and bit field operations
- **Network Byte Order**: Handle endianness in network programming
- **Time Representation**: Work with different time formats and epochs
- **Algorithm Implementation**: Code a real-world network synchronization algorithm

### Conceptual Understanding
- **Network Protocols**: See how standardized protocols enable internet functionality
- **Time Synchronization**: Understand distributed system clock coordination
- **Precision vs. Accuracy**: Learn the difference and why both matter
- **Network Effects**: See how network delay affects distributed algorithms

### Professional Development
- **Reading Specifications**: Interpret formal protocol documentation
- **Debugging Network Code**: Use packet analysis and structured debugging
- **Code Organization**: Structure complex programs with clear separation of concerns

## Getting Started

1. **Examine the code structure**: Start by reading through the provided scaffold to understand the program flow
2. **Study the header file**: Understand the packet structure and available macros
3. **Use debugging tools**: Try `./ntp-client -d` to see epoch conversion examples
4. **Start with packet building**: Implement `build_ntp_request()` first
5. **Test incrementally**: Use the provided print functions to verify your packet contents
6. **Implement time conversion**: Work on the timestamp conversion functions
7. **Complete the algorithm**: Implement the core NTP calculation
8. **Polish the output**: Make your results display user-friendly

## Testing Your Implementation

```bash
# Compile
make

# Test with default server
./ntp-client

# Test with specific servers
./ntp-client -s time.nist.gov
./ntp-client -s pool.ntp.org

# Debug mode - see epoch conversion and bit field examples
./ntp-client -d

# Check structure sizes (should be exactly 48 bytes)
make check-structs
```

### Expected Results
- Time offsets typically under 100 milliseconds for good servers
- Round-trip delays vary based on network distance
- Stratum values of 1-4 for most public servers

### Debugging Tools Available

Your implementation includes several debugging helpers:

- **Debug Mode (`-d` flag)**: Shows epoch conversion examples with real timestamps
- **`debug_print_bit_fields()`**: Shows bit field breakdown with binary representation
- **Sanity Check Ranges**: Verify your timestamps fall within expected ranges

## Analysis Questions

In addition to your code implementation, answer the following questions to demonstrate your understanding of key NTP concepts. Your answers should be clear and demonstrate understanding (1-2 paragraphs each). Include sample outputs from testing different servers.

### Question 1: Time Travel Debugging
Your NTP client reports your clock is 30 seconds ahead, but you just synchronized yesterday. List three possible causes and how you'd investigate each one. Consider both technical issues (hardware, software, network) and real-world scenarios that could affect time synchronization.

### Question 2: Network Distance Detective Work
Test your NTP client with two different servers - one geographically close to you (like a national time service) and one farther away. Compare the round-trip delays you observe.

Based on your results, explain why the physical distance to an NTP server affects time synchronization quality. Why might you get a more accurate time sync from a "worse" time source that's closer to you rather than a "better" time source that's farther away? What does this tell us about distributed systems in general?

Include your actual test results and delay measurements in your answer.

### Question 3: Protocol Design Challenge
Imagine a simpler time protocol where a client just sends "What time is it?" to a server, and the server responds with "It's 2:30:15 PM".

Explain why this simple approach wouldn't work well for accurate time synchronization over a network. In your answer, discuss what problems network delay creates for time synchronization and why NTP needs to exchange multiple timestamps instead of just sending the current time. What additional information does having all four timestamps (T1, T2, T3, T4) provide that a simple request-response couldn't?

## Deliverables

Submit the following files:
- `ntp-client.c` - Your completed implementation
- `answers.md` - Your responses to the three analysis questions (include sample outputs from different servers)
- `README.md` - Brief description of your approach and any challenges faced

## Grading Rubric

| Component | Excellent (90-100%) | Good (80-89%) | Satisfactory (70-79%) | Needs Work (60-69%) | Unsatisfactory (0-59%) |
|-----------|-------------------|---------------|---------------------|-------------------|----------------------|
| **NTP Request Construction (20%)** | Request packet perfectly formatted with all fields correctly set, proper bit manipulation, network byte order handled correctly | Request packet mostly correct, minor issues with some fields or byte order | Request packet functional but has some incorrect fields or formatting issues | Request packet created but with several errors affecting functionality | Request packet malformed or not implemented |
| **Time Conversion Functions (15%)** | Timestamp conversions flawless, handles NTP epoch correctly, proper precision maintained | Timestamp conversions work correctly with minor precision issues | Conversions functional but may lose some precision or have edge case issues | Conversions work for basic cases but have notable errors | Conversions incorrect or not implemented |
| **NTP Algorithm Implementation (20%)** | Perfect implementation of NTP algorithm, correct offset and delay calculations, handles all edge cases | Algorithm implemented correctly with minor calculation errors | Algorithm works for most cases but has some calculation issues | Basic algorithm implemented but with several computational errors | Algorithm incorrect or not implemented |
| **Output and Debugging (10%)** | Excellent output formatting, comprehensive packet information display, clear results presentation | Good output with minor formatting issues, most information displayed correctly | Basic output that shows essential information but lacks polish | Output present but difficult to read or missing important information | Poor or missing output functions |
| **Code Quality and Documentation (10%)** | Clean, well-commented code that follows best practices, excellent variable naming and structure | Good code organization with adequate comments and clear structure | Code is functional and reasonably organized with some comments | Code works but is poorly organized or documented | Code is difficult to understand or follow |
| **Analysis Questions (20%)** | Demonstrates deep understanding of NTP concepts, provides insightful analysis with clear explanations and supporting evidence from actual test results | Shows solid understanding with mostly correct analysis and good explanations, includes some test data | Basic understanding evident, answers are generally correct but may lack depth or supporting data | Shows some understanding but answers have gaps or minor errors, limited test evidence | Poor understanding, answers are incomplete or incorrect |
| **Testing and Validation (5%)** | Comprehensive testing with multiple servers, demonstrates understanding of results, includes edge case handling, uses debug features effectively | Good testing with multiple servers, shows reasonable results interpretation | Basic testing with successful synchronization shown | Limited testing, may not work with all servers | Insufficient testing or non-functional implementation |

### Grade Scale
- **A (90-100%)**: Implementation demonstrates mastery of network protocols and time synchronization concepts
- **B (80-89%)**: Solid understanding with minor technical issues
- **C (70-79%)**: Basic functionality achieved with some gaps in implementation
- **D (60-69%)**: Minimal functionality, significant issues remain
- **F (0-59%)**: Implementation does not meet basic requirements

### Bonus Opportunities (+5% each, max +15%)
- **Multi-server averaging**: Query multiple servers and average the results
- **Outlier detection**: Identify and handle servers with suspicious responses
- **IPv6 support**: Extend the client to work with IPv6 NTP servers
- **Comprehensive error handling**: Add robust validation and error recovery

## Tips for Success

1. **Start early**: Network programming can have unexpected challenges
2. **Use the debugging tools**: Try the `-d` flag and `debug_print_bit_fields()` function
3. **Read the header file carefully**: All the constants and macros you need are provided
4. **Test with multiple servers**: Different servers may expose different bugs
5. **Pay attention to byte order**: Network protocols are strict about endianness
6. **Understand the math**: The NTP algorithm is simple but precise implementation matters
7. **Use the visual guides**: Reference the bit packing and epoch conversion examples

## Resources

- [RFC 5905 - Network Time Protocol Version 4](https://tools.ietf.org/html/rfc5905)
- [NTP Pool Project](https://www.pool.ntp.org/) - Public NTP servers
- [NIST Time Services](https://www.nist.gov/pml/time-and-frequency-division/services/internet-time-service-its)

Remember: The goal isn't just to make it work, but to understand how network time synchronization enables the modern internet!