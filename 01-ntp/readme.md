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

## Protocol Design Investigation (Required)

**Complete this AFTER you have a working implementation.**

### The Goal

You've just implemented a 50-year-old protocol that still powers the internet. But WHY does it work the way it does? This component teaches you to use AI as a learning tool to deeply understand protocol design decisions you encountered in your implementation.

**This is NOT about getting AI to write code or answer homework questions. This is about using AI to investigate the "why" behind what you coded, using your actual implementation as the starting point.**

### Assignment Structure

Pick **TWO** investigation topics from the options below. For each topic:

1. Reference specific code you wrote
2. Use AI to investigate the design rationale through iterative questioning
3. Connect your findings back to your implementation
4. Demonstrate changed understanding

### Investigation Topics (Choose 2)

#### **Topic 1: The Four-Timestamp Algorithm**

**Your Implementation:**
You coded something like:
```c
delay = (T4 - T1) - (T3 - T2);
offset = ((T2 - T1) + (T3 - T4)) / 2;
```

**Investigate:**
- Why can't we just send "what time is it?" and calculate offset from the response?
- What would break with a 2-timestamp approach?
- How do the four timestamps separate network delay from clock offset?
- When you tested your client, what would your results have looked like with a simpler approach?

**Key Questions to Explore:**
- Start: "I implemented NTP with 4 timestamps. Why not just [simpler approach]?"
- Follow up: "When I tested against [server], I got [X]ms delay. What would happen with..."
- Challenge: "The math seems like we're over-complicating it. What edge case am I missing?"

#### **Topic 2: Bit Packing (li_vn_mode byte)**

**Your Implementation:**
You used:
```c
SET_NTP_LI_VN_MODE(packet, NTP_LI_UNSYNC, NTP_VERSION, NTP_MODE_CLIENT);
```

**Investigate:**
- Why pack 3 fields into 1 byte when modern systems have gigabytes of memory?
- What's the actual savings? Is it worth the complexity?
- Are there situations where this still matters today?
- How does this relate to other protocol design you've seen?

**Key Questions to Explore:**
- Start: "My NTP code packs 3 fields into 1 byte. Why this complexity in 2025?"
- Follow up: "Saving 2 bytes per packet seems trivial. When does this actually matter?"
- Challenge: "What would actually break if NTP used 3 separate bytes?"

#### **Topic 3: Network Byte Order**

**Your Implementation:**
You converted between host and network order:
```c
packet->transmit_ts.seconds = htonl(seconds);
// Later: seconds = ntohl(response.transmit_ts.seconds);
```

**Investigate:**
- Why do we need this conversion? What breaks without it?
- Why is big-endian the "network standard" when x86 is little-endian?
- In your testing, did you connect to servers on different architectures? How did byte order affect that?
- Could we eliminate this in modern protocols?

**Key Questions to Explore:**
- Start: "I convert to network byte order in my NTP code. What breaks if I skip this?"
- Follow up: "My development machine is [architecture]. If I removed htonl/ntohl..."
- Challenge: "Why not just standardize on little-endian since x86 dominates?"

#### **Topic 4: NTP Epoch (1900 vs 1970)**

**Your Implementation:**
You converted between epochs:
```c
unix_seconds = ntp_seconds - NTP_EPOCH_OFFSET;
// Where NTP_EPOCH_OFFSET = 2208988800
```

**Investigate:**
- Why did NTP choose 1900 instead of matching Unix's 1970?
- What's the Year 2036 problem and when does it happen?
- Your code converts every timestamp - couldn't we have avoided this?
- Why not update NTP to use Unix epoch?

**Key Questions to Explore:**
- Start: "My code converts between 1900 and 1970 epochs. Why two standards?"
- Follow up: "I saw Year 2036 mentioned in the RFC. What happens then to my code?"
- Challenge: "Wouldn't it be simpler if everyone just used Unix time?"

#### **Topic 5: UDP Instead of TCP**

**Your Implementation:**
Your code uses UDP sockets:
```c
sockfd = socket(AF_INET, SOCK_DGRAM, 0);  // DGRAM = UDP
```

**Investigate:**
- Why UDP when TCP is more reliable?
- What happens if your NTP request packet gets lost?
- When you tested, did you ever see timeouts? What does that tell you?
- Are there other time protocols that use TCP? Why or why not?

**Key Questions to Explore:**
- Start: "My NTP client uses UDP. Why not TCP for guaranteed delivery?"
- Follow up: "When I tested, [did/didn't] see packet loss. If I'd used TCP..."
- Challenge: "Wouldn't TCP's reliability be better for accurate time sync?"

#### **Topic 6: Stratum Hierarchy**

**Your Implementation:**
You set stratum to 0 in requests and received stratum values in responses:
```c
packet->stratum = 0;  // Client doesn't know its stratum
// Server responds with its stratum (1-4 typically)
```

**Investigate:**
- Why a hierarchy instead of all servers syncing directly with atomic clocks?
- When you tested different servers, did you see different stratum values? What does that tell you?
- How does this prevent timing loops?
- Could we design a better system today?

**Key Questions to Explore:**
- Start: "My NTP code uses a stratum hierarchy. Why not connect everyone to stratum 1?"
- Follow up: "I tested [server A] (stratum X) vs [server B] (stratum Y). How does..."
- Challenge: "With modern infrastructure, couldn't we flatten this hierarchy?"

#### **Topic 7: Fractional Seconds Representation**

**Your Implementation:**
You converted fractional seconds:
```c
fraction = (usec * NTP_FRACTION_SCALE) / USEC_INCREMENTS;
// Where NTP_FRACTION_SCALE = 2^32
```

**Investigate:**
- Why use 2^32 for fractional seconds instead of milliseconds or microseconds?
- What precision does this actually give? Does it matter for internet time sync?
- When you tested, how much did fractional seconds vary between packets?
- Are there simpler representations we could use?

**Key Questions to Explore:**
- Start: "My code converts microseconds to 1/2^32 increments. Why this format?"
- Follow up: "In my tests, offsets were ~[X]ms. Do we actually need picosecond precision?"
- Challenge: "Why not just use milliseconds like most programming languages?"

### Investigation Requirements (For Each Topic)

#### **1. Implementation Context (3-4 sentences)**
- What specific code did you write for this feature?
- What initially seemed odd, complex, or unnecessary?
- Reference actual lines from your implementation

**Example:**
```
In my build_ntp_request() function, I set packet->stratum = 0 for client requests, 
even though the comment said this means "unspecified." This seemed wrong - shouldn't 
I tell the server what level I'm at? When I tested against time.nist.gov, I received 
stratum=1 back, and against pool.ntp.org I got stratum=2. I started wondering why 
this hierarchy exists at all.
```

#### **2. AI Investigation Documentation**

Conduct a conversation with an AI assistant that shows:
- **Your initial question** - must reference your actual implementation
- **At least 5-6 follow-up questions** showing how your understanding evolved
- **Questions that explore alternatives** - "Why not just..." / "What if instead..."
- **Questions connecting to your test results** - "When I ran this, I saw X..."
- **Questions challenging the design** - "Couldn't we do this simpler way?"

**Required format:**
```
ME: [your question]
AI: [key parts of response - can be condensed]
ME: [follow-up based on that answer]
AI: [key parts of response]
[continue for 5-6+ exchanges]
```

**Quality indicators:**
- ✅ Each question builds on the previous answer
- ✅ You reference your specific implementation
- ✅ You use your test results as examples
- ✅ You challenge assumptions
- ✅ You explore "what if" alternatives

**Warning signs:**
- ❌ Single question with long AI response
- ❌ Generic questions not tied to your code
- ❌ No reference to your testing experience
- ❌ Just accepting the first explanation

#### **3. Design Rationale (2-3 paragraphs)**

Synthesize what you learned. Cover:
- **The core design decision** - what problem does this solve?
- **The tradeoffs** - what's gained and what's sacrificed?
- **Why alternatives don't work** - what would break with simpler approaches?
- **Connection to your code** - reference specific functions/lines

**This must be YOUR synthesis, not AI's explanation. Show you understand by:**
- Using your own examples
- Referencing your actual test results
- Explaining in your own words why the design makes sense
- Identifying what surprised you most

#### **4. Implementation Insight (1 paragraph)**

Answer: **How does understanding the "why" change how you view your code?**

Consider:
- Would you implement it differently now?
- Does the complexity now seem justified?
- What would you explain to another programmer?
- What's one "aha moment" from your investigation?

**Example:**
```
Before this investigation, the four-timestamp algorithm seemed overly complex - why 
not just get the server's time and be done? Now I understand that my 47ms round-trip 
delay to time.nist.gov would have caused a 23ms systematic error with a naive 
approach. The algorithm doesn't just get the time; it SEPARATES network delay from 
clock offset. When I look back at my calculate_offset() function, the seemingly odd 
formula ((T2-T1) + (T3-T4))/2 now makes perfect sense - it's averaging out the 
one-way delays. This isn't over-engineering; it's the minimum complexity needed for 
accuracy over a variable-delay network.
```

### Submission Format

Create a document: `protocol-investigation.md`

Structure it as:
```markdown
# NTP Protocol Design Investigation

## Investigation 1: [Topic Name]

### Implementation Context
[Your code and what puzzled you]

### AI Investigation
[Your conversation transcript]

### Design Rationale
[Your synthesis of what you learned]

### Implementation Insight
[How this changed your understanding]

---

## Investigation 2: [Topic Name]

### Implementation Context
[Your code and what puzzled you]

### AI Investigation
[Your conversation transcript]

### Design Rationale
[Your synthesis of what you learned]

### Implementation Insight
[How this changed your understanding]
```

### Complete Investigation Examples

**📘 IMPORTANT: See example_investigation.md for two complete, high-quality investigations.**

That file shows:
- **Investigation 1**: Four-Timestamp Algorithm (full example)
- **Investigation 2**: UDP Instead of TCP (full example)

Each demonstrates:
- Proper questioning technique (6+ iterative questions)
- Strong code connections with specific references
- Use of actual test results as examples
- Synthesis in student's own voice
- Clear "aha moments" and insights

**Use these as your quality standard.** If your investigation doesn't have similar depth, specificity, and progression, keep working on it.

---

## Deliverables

Submit the following files:
1. **`ntp-client.c`** - Your completed implementation
2. **`protocol-investigation.md`** - Your TWO protocol investigations (follow format above)
3. **`README.md`** - Brief description of your implementation approach and any challenges

## Grading Rubric

### NTP Client Implementation (60 points)

| Component | Excellent (54-60) | Good (48-53) | Satisfactory (42-47) | Needs Work (36-41) | Unsatisfactory (0-35) |
|-----------|-------------------|--------------|----------------------|--------------------|-----------------------|
| **Request Construction (15%)** | Request packet perfectly formatted, all fields correct, proper bit manipulation, network byte order handled | Request mostly correct, minor field or byte order issues | Request functional but some formatting issues | Request created but several errors affecting functionality | Request malformed or not implemented |
| **Time Conversion (15%)** | Timestamp conversions flawless, handles epochs correctly, proper precision | Conversions work with minor precision issues | Conversions functional but may lose precision | Conversions work for basic cases but have notable errors | Conversions incorrect or missing |
| **NTP Algorithm (15%)** | Perfect offset/delay calculations, handles edge cases | Algorithm correct with minor errors | Algorithm works for most cases, some calculation issues | Basic algorithm with several errors | Algorithm incorrect or missing |
| **Output & Debugging (5%)** | Excellent formatting, comprehensive packet display, clear results | Good output, minor formatting issues | Basic output showing essential information | Output present but hard to read or incomplete | Poor or missing output |
| **Code Quality (10%)** | Clean, well-commented code, excellent structure | Good organization, adequate comments | Functional, reasonably organized | Works but poorly organized/documented | Difficult to understand |

### Protocol Design Investigation (40 points total - 20 points per investigation)

| Criteria | Excellent (18-20) | Good (15-17) | Satisfactory (12-14) | Needs Work (0-11) |
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

- **Protocol Investigation**: 40 points
  - Investigation 1: 20 points
  - Investigation 2: 20 points

**Total: 100 points**

### Grade Scale
- **A (90-100%)**: Demonstrates mastery of both implementation and protocol understanding
- **B (80-89%)**: Solid implementation with good protocol analysis
- **C (70-79%)**: Working implementation with basic protocol understanding
- **D (60-69%)**: Minimal functionality and limited understanding
- **F (0-59%)**: Does not meet requirements

### Bonus Opportunities (+5% each, max +15%)
- **Multi-server averaging**: Query multiple servers and average results
- **Outlier detection**: Identify and handle servers with suspicious responses
- **IPv6 support**: Extend client to work with IPv6
- **Additional investigation**: Third protocol investigation of same quality

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