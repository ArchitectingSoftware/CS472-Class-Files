# Protocol Analysis with Wireshark

**Assignment Component:** Required (10 points)  
**Difficulty:** Intermediate - Requires Independent Research  
**Skills:** Network Protocol Analysis, Packet Capture, Self-Directed Learning

---

## The Challenge

You've implemented a custom network protocol with PDUs, key exchanges, and encrypted data. But how do you **prove** it works correctly? How do you see what's actually happening on the wire?

**Your task:** Use Wireshark to capture and analyze the network traffic between your client and server. Verify your protocol implementation matches the specification and document what you discover.

**Specifically, you need to:**
1. Learn how to use Wireshark for protocol analysis
2. Capture traffic between your client and server
3. Identify and analyze your PDUs in the captured packets
4. Verify the PDU structure (msg_type, direction, payload_len)
5. Document your findings and learning process using AI tools

**The approach:** Use AI tools (ChatGPT, Claude, Gemini, etc.) to research Wireshark independently. This is a required component, not extra credit.

---

## Why This Matters

**In network programming:**
- Your code might compile and seem to work, but is the protocol correct?
- Wireshark lets you see the actual bytes on the network
- It's the definitive way to verify protocol implementations
- Essential for debugging when client and server don't communicate correctly

**Professional reality:**
- Every network engineer uses Wireshark or tcpdump
- It's the standard tool for protocol analysis and debugging
- You'll use it throughout your career for network troubleshooting
- Understanding packet captures is crucial for security analysis

**For this assignment:**
- Validates your PDU structure is correct
- Shows your key exchange actually happens
- Proves encryption is working (encrypted bytes look random)
- Helps debug if your client/server aren't communicating

---

## Getting Started: Key Questions to Explore

Use AI tools to research and discover answers to these questions:

### Understanding Phase

1. **What is Wireshark?** What does it do and why is it used?

2. **How do you install Wireshark?** On your Linux environment (tux or VM)?

3. **What is a packet capture?** What information does it contain?

4. **How do you capture localhost traffic?** Your client and server run on the same machine - how do you capture that?

### Capture Phase

5. **What interface do you capture on?** Loopback (lo)? Ethernet (eth0)? How do you know?

6. **How do you filter traffic?** You only want to see your client-server communication, not all network traffic.

7. **What do TCP packets look like in Wireshark?** How do you identify your PDUs within TCP segments?

8. **How do you save a capture?** What file format should you use?

### Analysis Phase

9. **How do you examine packet contents?** How do you see the raw bytes of your PDU?

10. **What do encrypted bytes look like?** How can you tell the difference between plaintext and encrypted data?

11. **How do you verify PDU fields?** Can you see msg_type, direction, and payload_len in the capture?

12. **How do you follow a TCP stream?** Wireshark can reconstruct the entire conversation - how?

---

## Learning Strategy: Using AI Effectively

### Research Approach

1. **Start broad**: "What is Wireshark?" → "How do I capture localhost traffic?"
2. **Get specific**: Tell the AI you're analyzing a custom protocol with binary PDUs on TCP
3. **Share your structure**: Show the AI your PDU structure when asking how to analyze it
4. **Iterate**: Capture something, ask AI how to interpret what you see
5. **Validate**: Compare what Wireshark shows with what your code sends

### When You Get Stuck

- Share screenshots of Wireshark with AI (describe what you see)
- Ask about specific byte values you don't understand
- Request help interpreting filters or display options
- Compare different AI tools' explanations

### Critical Thinking

**Remember:**
- Wireshark shows raw network data - it's ground truth
- If Wireshark shows something different than your code claims to send, Wireshark is right
- Encrypted data looks random - that's how you know it's encrypted
- PDU boundaries might not align with TCP packet boundaries

---

## What You Need to Deliver

### File: `protocol-analysis.md`

Create this file in your assignment directory with the following sections:

### 1. Learning Process (2 points)

Document how you learned Wireshark:
- What AI tools did you use?
- What questions did you ask? (Include 3-4 specific prompts)
- What resources did the AI point you to?
- What challenges did you encounter learning Wireshark?

### 2. Capture Setup (2 points)

Explain your capture configuration:
- What interface did you capture on? Why?
- What capture filter did you use (if any)?
- What display filter helped you find your traffic?
- How did you identify your specific client-server connection?

### 3. PDU Analysis (4 points)

Analyze your protocol implementation. For each message type, provide:

**A. Plain Text Message (MSG_DATA)**
- Screenshot showing the PDU in Wireshark
- Identify: msg_type value, direction value, payload_len value
- Verify the payload contains your text message
- Confirm the PDU structure matches protocol.h

**B. Key Exchange (MSG_KEY_EXCHANGE)**
- Screenshot of the request PDU
- Screenshot of the response PDU
- Verify: direction changes from request to response
- Confirm: response payload contains the crypto_key_t (what size?)

**C. Encrypted Message (MSG_ENCRYPTED_DATA)**
- Screenshot showing encrypted PDU
- Point out: the payload looks random (encrypted bytes)
- Verify: payload_len matches encrypted data size
- Compare: encrypted payload vs plain text payload appearance

**D. Exit Command**
- Screenshot of MSG_CMD_CLIENT_STOP or MSG_CMD_SERVER_STOP
- Verify: payload_len is 0 for command messages

### 4. Protocol Verification (2 points)

Answer these verification questions:
- Do your PDU headers always have the correct structure (4 bytes)?
- Are msg_type values correct for each message?
- Is direction always DIR_REQUEST from client, DIR_RESPONSE from server?
- Does payload_len always match the actual payload size?
- Did you find any bugs in your implementation through this analysis?

### Technical Requirements for Screenshots

For each screenshot:
- Show the packet list (top pane)
- Show the packet details (middle pane) - expand the relevant sections
- Show the packet bytes (bottom pane) - this shows the raw hex
- Highlight or circle the relevant PDU fields
- Add brief annotations explaining what we're looking at

**Example annotation:**
```
Screenshot: msg_data_request.png
- Packet #42 in the capture
- TCP payload starts at offset 0x0000
- First byte (0x02) = MSG_DATA
- Second byte (0x01) = DIR_REQUEST
- Bytes 2-3 (0x00 0x0C) = payload_len = 12 bytes
- Following 12 bytes = "hello server" in ASCII
```

---

## Testing Checklist

Before submitting, verify you've captured and analyzed:

- [ ] Client connection establishment (TCP 3-way handshake)
- [ ] At least one MSG_DATA exchange
- [ ] Complete MSG_KEY_EXCHANGE (request + response)
- [ ] At least one MSG_ENCRYPTED_DATA exchange
- [ ] At least one command message (client stop or server stop)
- [ ] TCP connection teardown (FIN packets)

---

## Grading Rubric

**10 points total:**

**Learning Process (2 points)**
- 2 pts: Clear documentation of AI-assisted learning with specific examples
- 1 pt: Vague description of learning process
- 0 pts: No evidence of learning process

**Capture Setup (2 points)**
- 2 pts: Clear explanation of interface, filters, and identification
- 1 pt: Basic explanation, missing some details
- 0 pts: Unclear or incorrect setup

**PDU Analysis (4 points)**
- 4 pts: All message types captured and analyzed with clear screenshots
- 3 pts: Most message types analyzed, minor issues
- 2 pts: Some message types missing or incomplete analysis
- 1 pt: Minimal analysis, many gaps
- 0 pts: No meaningful PDU analysis

**Protocol Verification (2 points)**
- 2 pts: Thorough verification, identifies any bugs found
- 1 pt: Basic verification, incomplete
- 0 pts: No verification or incorrect

---

## Hints for Success

### Capturing Localhost Traffic

Your client and server run on the same machine (127.0.0.1), so:
- Capture on the **loopback interface** (usually `lo`)
- Use a **display filter** to show only your port (e.g., `tcp.port == 1234`)
- Your traffic won't appear on eth0 or wlan0

### Finding Your PDUs

- Your PDUs are inside TCP packets
- Right-click a packet → "Follow TCP Stream" to see the whole conversation
- Use "Decode As" if Wireshark doesn't recognize your protocol
- Look at the "Data" section in packet details - that's your PDU

### Interpreting Encrypted Data

- Encrypted bytes look random in the hex view
- Compare encrypted payload to plain text payload - very different
- Encrypted data has no patterns, no readable ASCII
- This proves encryption is working!

### Common Issues

- **Can't see any packets**: Wrong interface, check you're on `lo`
- **Too much traffic**: Use display filter `tcp.port == YOUR_PORT`
- **Can't find PDU**: Look in TCP payload, not IP header
- **Bytes look wrong**: Remember network byte order (big-endian)

---

## Resources

- Wireshark documentation (ask AI for link or Google it)
- Your protocol specification (protocol.h)
- Your implementation code
- Your AI tool of choice (ChatGPT, Claude, Gemini, etc.)

---

## Example: What Good Analysis Looks Like

Here's what a strong PDU analysis entry might include:

### MSG_DATA Request Analysis

**Screenshot:** `msg_data_plain.png`

**Packet Details:**
- Packet #15 in capture
- Source: 127.0.0.1:45678
- Destination: 127.0.0.1:1234
- TCP payload: 19 bytes total

**PDU Structure Breakdown:**
```
Offset  Value    Field          Interpretation
------  -------  -------------  --------------------------
0x00    0x02     msg_type       MSG_DATA (value 2)
0x01    0x01     direction      DIR_REQUEST (value 1)
0x02    0x00 0x0F payload_len   15 bytes (network byte order)
0x04-   "Hello..."              15 bytes of ASCII text
0x12
```

**Verification:**
- ✓ msg_type correct (MSG_DATA = 2)
- ✓ direction correct (DIR_REQUEST = 1)
- ✓ payload_len correct (15 bytes = 0x000F)
- ✓ Actual payload is 15 bytes
- ✓ Total PDU size: 4 bytes header + 15 bytes payload = 19 bytes

**Notes:**
The server response (packet #16) has direction = 0x02 (DIR_RESPONSE) and includes "echo " prefix, making the payload 21 bytes.

---

## Final Thought

Wireshark shows you **ground truth** - what's actually on the network. Your code might claim to send certain bytes, but Wireshark proves what actually happened. This is invaluable for:
- Verifying your implementation
- Debugging protocol issues
- Understanding how networks really work
- Building confidence in your code

The goal isn't just to capture some packets - it's to **understand your protocol at the byte level** and verify it works exactly as specified.

**Good luck with your analysis!**