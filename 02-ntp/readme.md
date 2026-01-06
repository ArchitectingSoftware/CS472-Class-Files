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
```
NTP_EPOCH_OFFSET = 2,208,988,800 seconds = exactly 70 years

Your Code Will Do This:
unix_time = ntp_time - 2208988800    // NTP to Unix
ntp_time = unix_time + 2208988800    // Unix to NTP

MEMORY TRICK: NTP time is always BIGGER (it started counting earlier)
```

NTP timestamps contain:
- **Transmit Timestamp**: When the packet was sent
- **Receive Timestamp**: When the server received your request
- **Reference Timestamp**: When the server's clock was last synchronized
- **Originate Timestamp**: Echo of client's transmit time

Each timestamp is 64 bits (8 bytes):
- **32 bits**: Seconds since January 1, 1900
- **32 bits**: Fractional seconds (1/2³² resolution ≈ 232 picoseconds)

## What You'll Learn

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
- **Learning with AI**: Use modern tools to investigate protocol design decisions

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

---

## Protocol Design Investigation: Learning Through Implementation (30 points)

**Complete this AFTER you have a working implementation.**

The best way to understand protocol design is to implement it. You've just built an NTP client - now reflect on what you learned through that process and dig deeper into design decisions or concepts that puzzled you while coding.

### Requirements

**Pick 2 topics from the list below** that genuinely puzzled you during implementation. For each:
- Document what confused you in YOUR code
- Show how you explored the design through research and testing
- Synthesize your understanding in your own words
- Explain your "aha moment", or articulate things that still confuse you

**Points:** 15 points per investigation (30 total)

---

### Suggested Topics (Pick 2)

Choose topics that:
- Actually puzzled you while coding
- You tested and observed in practice
- You can reference with specific code you wrote

#### **Topic 1: Four-Timestamp Algorithm**

**What you implemented:**
You calculated offset and delay using T1, T2, T3, T4:
```c
delay = (T4 - T1) - (T3 - T2);
offset = ((T2 - T1) + (T3 - T4)) / 2;
```

**Why investigate this:**
- This formula seems overly complex - why not just use T3 (server time)?
- What problem does this actually solve?
- When you tested, what delays did you see? How did the algorithm handle them?
- Could we simplify this?

#### **Topic 2: Packed Bit Fields (li_vn_mode)**

**What you implemented:**
You packed 3 fields into one byte using bit manipulation:
```c
SET_NTP_LI_VN_MODE(packet, NTP_LI_UNSYNC, NTP_VERSION, NTP_MODE_CLIENT);
// Results in: li_vn_mode = 0xE3
```

**Why investigate this:**
- Why pack fields when we have plenty of bandwidth?
- How does this relate to network protocol design principles?
- When you debugged, did this packing ever cause issues?
- Why not just use 3 separate bytes?

#### **Topic 3: Network Byte Order (Endianness)**

**What you implemented:**
You converted all multi-byte fields:
```c
packet->transmit_ts.seconds = htonl(seconds);
// Later: seconds = ntohl(response.transmit_ts.seconds);
```

**Why investigate this:**
- Why is this necessary? What breaks without it?
- When you tested against different servers, how did this matter?
- Why standardize on big-endian when most processors are little-endian?
- Could modern protocols skip this?

#### **Topic 4: NTP Epoch (1900 vs 1970)**

**What you implemented:**
You converted between two time standards:
```c
unix_seconds = ntp_seconds - NTP_EPOCH_OFFSET;
// Where NTP_EPOCH_OFFSET = 2208988800 (70 years in seconds)
```

**Why investigate this:**
- Why two different epoch standards?
- What's the Year 2036 problem?
- When you tested, did you notice the huge numbers in NTP timestamps?
- Why not update NTP to use Unix epoch?

#### **Topic 5: UDP Instead of TCP**

**What you implemented:**
Your code uses UDP sockets:
```c
sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // DGRAM = UDP
```

**Why investigate this:**
- Why UDP when TCP guarantees delivery?
- When you tested, did you ever see timeouts? What does that tell you?
- What happens if packets get lost?
- Couldn't TCP's reliability be better for time synchronization?

#### **Topic 6: Stratum Hierarchy**

**What you implemented:**
You set stratum to 0 in requests and observed different values in responses:
```c
packet->stratum = 0;  // Client doesn't know its stratum
// Server responds with its stratum (1-4 typically)
```

**Why investigate this:**
- Why a hierarchy instead of everyone syncing with atomic clocks?
- When you tested different servers, what stratum values did you see?
- How does this prevent timing loops?
- Is this hierarchy still necessary with modern infrastructure?

#### **Topic 7: Fractional Seconds Representation**

**What you implemented:**
You converted fractional seconds to NTP format:
```c
fraction = (usec * NTP_FRACTION_SCALE) / USEC_INCREMENTS;
// Where NTP_FRACTION_SCALE = 2^32
```

**Why investigate this:**
- Why use 2^32 scale instead of milliseconds or microseconds?
- What precision does this actually provide?
- When you tested, how much did timestamps vary?
- Do we need picosecond precision for internet time sync?

---


### Submission Format

**File:** place your solutions to the above in a file called `protocol-investigation.md` in your course repo.  It should have roughly the following structure:

**Structure:**
```markdown
# NTP Protocol Design Investigation

## Investigation 1: [Topic You Chose]

### Implementation Context
[What you coded and what puzzled you - reference specific code]

### Investigation Journey  
[Your research/exploration process - 5-6+ steps of discovery]

### Design Rationale
[Your synthesis - why this design, what tradeoffs, why alternatives fail]

### Implementation Insight
[Your "aha moment" - how this changed your understanding]

---

## Investigation 2: [Your Second Topic]

### Implementation Context
[What you coded and what puzzled you]

### Investigation Journey
[Your research/exploration process]

### Design Rationale
[Your synthesis in your own words]

### Implementation Insight
[What you learned from implementing]
```

---

### What Makes a Strong Investigation

**Excellent investigations show:**
- Genuine puzzlement from YOUR implementation experience
- Specific code references from YOUR functions
- Test results from YOUR runs
- Progressive deepening of understanding (5-6+ steps)
- Synthesis in your own words (not copy-paste)
- Clear "aha moment" when it clicked
- Connection back to specific code you wrote

**Weak investigations show:**
- Generic questions not tied to your code
- No reference to your testing experience
- Copy-pasted explanations without synthesis
- Single question with long response (no progression)
- No connection to what you actually implemented

---


### Common Mistakes to Avoid

**Don't:** Pick topics you didn't actually implement or test  
**Do:** Pick topics that genuinely confused you while coding

**Don't:** Ask one big question and paste a long AI response  
**Do:** Show 5-6+ steps demonstrating growing understanding

**Don't:** Copy explanations from AI/documentation without synthesis  
**Do:** Explain in your own words using your test results as examples

**Don't:** Say "the design is good" without explaining why  
**Do:** Explain the tradeoffs and why alternatives fail

**Don't:** Keep investigation separate from your code  
**Do:** Reference specific functions, lines, test results from YOUR implementation

---

**Remember:** This is about what you LEARNED by IMPLEMENTING, not just what you can research. The best investigations show: "I coded this, it puzzled me, I explored it, and NOW I get why it's designed this way."

---

## Distributed Systems Investigation (10 points)

**NEW COMPONENT:** In addition to the protocol investigations, you'll complete a separate investigation exploring how time synchronization fits into distributed systems.

### What You'll Learn

This investigation explores fundamental distributed systems concepts:
- **Real-world failures** caused by time synchronization issues
- **Physical vs logical time** and why timestamps aren't enough
- **CAP theorem** and fundamental tradeoffs
- **Eventual consistency** and when perfect sync isn't needed
- **Your NTP client's role** in the distributed systems landscape

### Why This Matters

Your NTP implementation is one piece of a larger puzzle. This investigation helps you understand:
- Why time synchronization is critical in distributed systems
- Where NTP is sufficient and where it's not
- How your implementation fits into production systems
- Fundamental concepts every systems engineer should know

### The Investigation

**File to create:** `time-in-distributed-systems.md`

**See the complete instructions in:** [`ai-distributed-investigation.md`](ai-distributed-investigation.md)

The investigation has 5 sections:
1. **Learning Process** (2 pts) - Document your AI-assisted research
2. **Real-World Failure** (2 pts) - Analyze a major time-related incident
3. **Physical vs Logical Time** (2 pts) - Understand Lamport clocks vs NTP
4. **CAP & Eventual Consistency** (2 pts) - Core distributed systems concepts
5. **Your NTP Client in Context** (2 pts) - Connect theory to your code

**This is a required component worth 10 points.**

---

## Deliverables

Submit the following files:
1. **`ntp-client.c`** - Your completed implementation
2. **`protocol-investigation.md`** - Your TWO protocol investigations (follow format above)
3. **`time-in-distributed-systems.md`** - Your distributed systems investigation (see AI-Distributed-Investigation.md)
4. **`README.md`** - Brief description of your implementation approach and any challenges

## Grading Rubric

### NTP Client Implementation (60 points)

| Component | Excellent (54-60) | Good (48-53) | Satisfactory (42-47) | Needs Work (36-41) | Unsatisfactory (0-35) |
|-----------|-------------------|--------------|----------------------|--------------------|-----------------------|
| **Request Construction (15%)** | Request packet perfectly formatted, all fields correct, proper bit manipulation, network byte order handled | Request mostly correct, minor field or byte order issues | Request functional but some formatting issues | Request created but several errors affecting functionality | Request malformed or not implemented |
| **Time Conversion (15%)** | Timestamp conversions flawless, handles epochs correctly, proper precision | Conversions work with minor precision issues | Conversions functional but may lose precision | Conversions work for basic cases but have notable errors | Conversions incorrect or missing |
| **NTP Algorithm (15%)** | Perfect offset/delay calculations, handles edge cases | Algorithm correct with minor errors | Algorithm works for most cases, some calculation issues | Basic algorithm with several errors | Algorithm incorrect or missing |
| **Output & Debugging (5%)** | Excellent formatting, comprehensive packet display, clear results | Good output, minor formatting issues | Basic output showing essential information | Output present but hard to read or incomplete | Poor or missing output |
| **Code Quality (10%)** | Clean, well-commented code, excellent structure | Good organization, adequate comments | Functional, reasonably organized | Works but poorly organized/documented | Difficult to understand |

### Protocol Design Investigation (30 points total - 15 points per investigation)

| Criteria | Excellent (13-15) | Good (10-12) | Satisfactory (7-9) | Needs Work (0-6) |
|----------|-------------------|--------------|----------------------|-------------------|
| **Implementation Context** | Clearly describes specific code written, identifies genuine puzzlement with concrete examples from testing | Describes implementation with some specifics, identifies area of confusion | Basic description of code, vague sense of what was unclear | Generic or missing context, no code references |
| **Investigation Quality** | 6+ substantive questions showing clear progression, explores alternatives, challenges assumptions, uses test results | 5 questions with reasonable progression, some exploration of alternatives | 3-4 questions with limited progression, surface-level exploration | 1-2 questions or just copying AI responses |
| **Design Understanding** | Demonstrates deep grasp of design rationale and tradeoffs, explains why alternatives fail, uses concrete examples | Solid understanding with minor gaps, explains main concepts adequately | Basic understanding, covers key points without depth | Poor understanding, significant misconceptions |
| **Code Connection** | Strong synthesis connecting investigation to specific implementation details, references actual code and test results | Good connection to implementation with some specific references | Weak connection, general statements about code | Little or no connection to actual implementation |
| **Personal Insight** | Clear "aha moment," shows changed understanding, articulates what surprised them and why | Shows learning occurred, reasonable insights | Surface-level insights, limited evidence of changed understanding | Generic statements, no evidence of learning |

### Point Distribution Summary
- **NTP Client Implementation**: 60 points
  - Request Construction: 15 points
  - Time Conversion: 15 points  
  - NTP Algorithm: 15 points
  - Output & Debugging: 5 points
  - Code Quality: 10 points

- **Protocol Investigation**: 30 points
  - Investigation 1: 15 points
  - Investigation 2: 15 points

- **Distributed Systems Investigation**: 10 points
  - Learning Process: 2 points
  - Real-World Failure: 2 points
  - Physical vs Logical Time: 2 points
  - CAP & Eventual Consistency: 2 points
  - Your NTP Client in Context: 2 points

**Total: 100 points**


### Bonus Opportunities (+5 points each, max +10 points)
- **Multi-server averaging**: Query multiple servers and average results
- **Outlier detection**: Identify and handle servers with suspicious responses

**Maximum possible points: 110 (100 base + 10 bonus)**

## Tips for Success

### Implementation Phase
1. **Start early**: Network programming can have unexpected challenges
2. **Use the debugging tools**: Try the `-d` flag and `debug_print_bit_fields()` function
3. **Read the header file carefully**: All the constants and macros you need are provided
4. **Test with multiple servers**: Different servers may expose different bugs
5. **Pay attention to byte order**: Network protocols are strict about endianness
6. **Understand the math**: The NTP algorithm is simple but precise implementation matters

### Investigation Phase
1. **Get your code working first**: You can't investigate what you haven't implemented
2. **Reference your actual code**: Generic questions get generic answers
3. **Use your test results**: "When I tested against X, I saw Y" makes better questions
4. **Challenge the design**: Ask "why not..." and "what if..." questions
5. **Let curiosity guide you**: Pick topics that genuinely puzzled you
6. **Iterate your questions**: Your first question won't give you everything
7. **Synthesize, don't copy**: We want YOUR understanding, not AI's explanation

### What Makes a Great Investigation
- You explore alternatives that seem simpler
- You use your actual test results as examples
- You can explain why the "obvious" approach doesn't work
- You understand the tradeoffs, not just the solution
- You connect findings back to specific code you wrote
- Someone reading your investigation learns something new

## Resources

- [RFC 5905 - Network Time Protocol Version 4](https://tools.ietf.org/html/rfc5905)
- [NTP Pool Project](https://www.pool.ntp.org/) - Public NTP servers
- [NIST Time Services](https://www.nist.gov/pml/time-and-frequency-division/services/internet-time-service-its)

## Academic Integrity

This assignment explicitly requires you to use AI for the protocol investigation component. However:

**For the Implementation (ntp-client.c):**
- You may use AI to help understand concepts, debug errors, or clarify documentation
- You must write and understand all code yourself
- Simply asking AI to "complete the TODO functions" violates academic integrity

**For the Investigation (protocol-investigation.md):**
- You MUST use AI to explore protocol design
- Your investigation documents YOUR learning process
- The quality of your questions and synthesis is what we grade
- Just copying AI responses without investigation gets minimal credit

**The Goal:** Learn to use AI as a tool for understanding, not as a tool for completion.

If you're unsure whether something is appropriate, ask before submitting.

---

Remember: The goal isn't just to make it work, but to understand WHY it works the way it does. Network protocols are designed by engineers solving real problems - your job is to understand those problems and appreciate the elegance of the solutions!