# Assignment 5 — Protocol over UDP

## Overview

This assignment has two components: a **programming component** and an **AI investigation component**. The programming component asks you to extend the du-proto transport protocol and improve the du-ftp application protocol. The AI investigation component asks you to use an AI model to explore QUIC — a real-world production protocol that solves many of the same problems you are tackling here, but at a much larger scale.

The code in this repository simulates a transport layer protocol and an application protocol. The transport protocol is implemented in `du-proto.c` and `du-proto.h`. Because accessing the IP layer directly from C is not straightforward, du-proto is built on top of UDP. The application protocol in `du-ftp.c` and `du-ftp.h` implements a simple file transfer service on top of du-proto.

---

## Transport Protocol — du-proto

For clients, the entry point is `dpClientInit()`, which takes the server IP address and port number as arguments. For servers, `dpServerInit()` takes the listening port as an argument. Servers then block on `dplisten()` until a client connects via `dpconnect()`. After the connection is established, both sides exchange data using `dpsend()` and `dprecv()`.

---

## Application Protocol — du-ftp

The application protocol implements a minimal FTP-like solution. Familiarize yourself with `du-ftp.c` and `du-ftp.h`. The provided Makefile builds a `du-ftp` executable that can run in either client or server mode. By default, the client reads from the `./outfile` directory and the server writes to the `./infile` directory. As currently written, du-ftp is more of a hardcoded file transfer script than a real protocol — part of your job is to change that.

---

## Repository Structure

All work for this assignment — both the programming component and the AI investigation — must be placed in the `05-ReliableUDP` folder of your GitHub Classroom repository. The TA will look for your work there exclusively.

```
05-ReliableUDP/
├── du-proto.c
├── du-proto.h
├── du-ftp.c
├── du-ftp.h
├── Makefile
├── quic-investigation.md
└── extra_credit.txt        (only if attempting extra credit)
```

---

## Deliverables Summary

| Component | Points |
|---|---|
| Programming — Arbitrary-length send/receive | 35 |
| Programming — du-ftp protocol improvements | 40 |
| AI Investigation — QUIC | 25 |
| **Total** | **100** |
| Extra Credit (up to) | 10 |

> The assignment is weighted at **100 points** toward your final grade. The extra credit is worth up to **10 additional points** on this assignment.

---

## Programming Component — 75 Points

Place all files required for your program to build and run — including the Makefile — in the `05-ReliableUDP` folder of your GitHub Classroom repository. Submitting only modified files is not sufficient; the full set of source files must be present so the TA can build and run your code directly from that folder.

### Deliverable 1 — Arbitrary-Length Send and Receive (35 points)

Take a close look at `dpsend()` and `dprecv()`. Currently both functions return an error if the amount of data exceeds `DP_MAX_BUFF_SZ` (defined in `du-proto.h`). The underlying du-proto was built with a maximum datagram of `DP_MAX_BUFF_SZ`, but `dpsend()` and `dprecv()` can be extended to handle data of arbitrary length since they already accept a buffer pointer and size.

Update `dpsend()` and `dprecv()` to support sending and receiving buffers larger than `DP_MAX_BUFF_SZ` by breaking the data into multiple datagrams transparently. Demonstrate that your changes work correctly by testing them through the du-ftp client.

### Deliverable 2 — du-ftp as a Real Application Protocol (40 points)

The du-ftp code is meant to represent an application protocol, but as written it does not behave like one. File names are handled outside the protocol, there is no mechanism for exchanging error information (such as file not found), and there is no graceful close. Your goal is to turn du-ftp into a minimal but real file exchange protocol.

**2a — Define a PDU for du-ftp (15 points)**

Define a Protocol Data Unit for du-ftp that allows clients and servers to exchange:

- The name of the file being transferred
- Status information about the transfer (in progress, complete, etc.)
- Error codes (e.g., file not found, permission denied)
- A graceful close signal

Look at how the PDU is structured in `du-proto.h` for inspiration. Place your PDU definition in `du-ftp.h` as a C-style `typedef`.

**2b — Increase Block Size and Exercise Deliverable 1 (10 points)**

The default du-ftp implementation transfers 500 bytes per exchange, which is smaller than the du-proto maximum datagram size of 512 bytes. Now that you have fixed the arbitrary-length limitation in Deliverable 1, update du-ftp to use substantially larger transfer blocks and verify that files are transferred correctly end-to-end.

**2c — Implement the Protocol Behavior (15 points)**

Update `du-ftp.c` to use the PDU you defined in 2a. At minimum your implementation must:

- Have the client send the filename to the server as part of the protocol, rather than relying on it being configured externally
- Have the server respond with a status or error before the transfer begins
- Use the graceful close signal to terminate the session cleanly

---

## AI Investigation Component — 25 Points

Complete the investigation in `quic-investigation.md`, located in the `05-ReliableUDP` folder of your GitHub Classroom repository. Full instructions and all questions are contained in that file.

### Grading Breakdown

| Element | Points |
|---|---|
| Question 1 — QUIC Streams (3 parts) | 9 |
| Question 2 — Connection IDs (3 parts) | 9 |
| Question 3 — Design Tradeoffs (2 parts) | 5 |
| AI Conversation Log | 2 |
| **Total** | **25** |

### What Full Credit Looks Like

Each answer should be 2–4 paragraphs. To receive full credit your responses must:

- Demonstrate that you understand the QUIC concept being asked about, not just that you can describe it abstractly
- Connect the concept explicitly to your du-proto implementation, referencing specific functions, data structures, or design decisions by name
- Reflect your own reasoning, not just a reformatted AI response

Answers that summarize QUIC without engaging with du-proto, or that are clearly unedited AI output with no original analysis, will receive minimal credit.

---

## Extra Credit — Up to 10 Points

If you attempt any extra credit, include a file named `extra_credit.txt` in your submission. Describe which option you attempted and provide an honest account of your progress, including what works, what does not, and what you learned. This file is required — the TA will not search your code for extra credit work.

### Option A — Retry Logic (up to 4 points)

Currently du-proto performs no retries and has no recovery logic when something goes wrong. Implement retry behavior to harden the protocol against failures. A helper function `dprand()` is already in the code and can be used to simulate random error conditions for testing.

### Option B — Sequence Number Validation (up to 3 points)

Currently du-proto exchanges sequence numbers but does not validate that the numbers received are correct or in the expected order. Add validation logic to the implementation and describe what behavior you implemented when an out-of-order or unexpected sequence number is received.

### Option C — Socket Timeouts (up to 3 points)

The UDP sockets used by du-proto block indefinitely waiting for data. Research how to configure UDP socket options to time out after a reasonable period (for example, 10 seconds). Implement these changes and update the protocol behavior to handle a timeout gracefully — describe what your implementation does when a timeout occurs.