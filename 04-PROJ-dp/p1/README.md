# dp Project, Part 1: Reliable Delivery over UDP

> Start with the project overview in [`../README.md`](../README.md). It explains what dp is, why we build it, how it relates to TCP and QUIC, and how we expect you to use AI.

## Overview

TCP gives applications a reliable, ordered byte stream. UDP does not: datagrams can be lost, delayed, or duplicated, and nothing tells the sender. In Part 1 you make **dp** (the Drexel Protocol) reliable on top of UDP, which means solving the same problems TCP solves.

dp is **QUIC-inspired**, but it is not QUIC and is not compatible with QUIC. Part 1 introduces ideas (connection IDs, packet numbers, streams, FIN) that Part 2 builds on.

The application hands an opaque byte buffer to `dp_send()` and gets the same bytes back from `dp_recv()`. Your transport is responsible for splitting the buffer into packets, delivering every packet reliably and in order, and reassembling the buffer.

## Learning goals

By the end of the assignment you should be able to:

1. Explain what UDP provides and what it does not.
2. Fragment and reassemble an application message across many UDP datagrams.
3. Implement stop-and-wait reliability with packet numbers, ACKs, timeouts, and retransmission.
4. Explain why a timeout does not prove a packet was lost, and handle the duplicates that result.
5. Use a socket receive timeout so a program never waits forever for a remote peer.
6. Read a merged protocol trace and explain *why* your implementation behaved as it did.

## The transport PDU

Every packet has a fixed 28-byte header followed by up to 480 bytes of payload:

```text
+----------------------+  2 bytes
| Magic                |
+----------------------+  1 byte
| Type                 |  CONNECT, CONNECT_ACK, DATA, ACK, CLOSE, CLOSE_ACK
+----------------------+  1 byte
| Flags                |  DP_FLAG_FIN on the last DATA packet
+----------------------+  4 bytes
| Connection ID        |
+----------------------+  4 bytes
| Packet Number        |
+----------------------+  4 bytes
| Stream ID            |  always 0 in Part 1
+----------------------+  4 bytes
| Message Offset       |
+----------------------+  4 bytes
| Message Length       |
+----------------------+  2 bytes
| Payload Length       |
+----------------------+  2 bytes
| Reserved             |
+----------------------+
| Payload (0-480)      |
+----------------------+
```

- **Connection ID:** identifies the logical connection independently of the UDP address and port.
- **Packet Number:** identifies a packet within the connection. DATA packets are numbered 1, 2, 3, ... An ACK carries the number of the packet it acknowledges.
- **Stream ID:** always 0. Part 1 has exactly one stream per connection. Part 2 adds more.
- **Message Offset:** where this packet's payload belongs in the application buffer.
- **Message Length:** total bytes in the application message.
- **FIN flag:** set on the final DATA packet.

## How Part 1 works

### Connection setup and close

```text
CLIENT                              SERVER
CONNECT (cid) ------------------->
            <---------------------- CONNECT_ACK
DATA / ACK exchanges ...
CLOSE --------------------------->
            <---------------------- CLOSE_ACK
```

Either handshake packet can be lost, so CONNECT and CLOSE are retransmitted on timeout. The server cannot know whether its CONNECT_ACK arrived: if it was lost, the client sends CONNECT again after the server has moved on to receiving data, and the server must answer again.

After answering CLOSE, the server stays alive for several timeouts of silence so it can answer a retransmitted CLOSE if its CLOSE_ACK was lost.

### Fragmentation

A 1200-byte message becomes:

```text
packet 1: offset 0,   480 bytes
packet 2: offset 480, 480 bytes
packet 3: offset 960, 240 bytes, FIN
```

### Stop-and-wait

Only one DATA packet is outstanding at a time. The sender waits for the matching ACK before sending the next packet. If no ACK arrives before the timeout (250 ms), it retransmits the **same** packet with the **same** packet number.

A timeout does **not** prove the DATA packet was lost. The DATA may have arrived and its ACK may have been lost or delayed:

```text
CLIENT                              SERVER
DATA #17 ------------------------->
                                    accept #17
            <-------------------X-- ACK #17 lost
  ...timeout...
DATA #17 ------------------------->
                                    duplicate: do not deliver again
            <---------------------- ACK #17
```

The receiver must recognize the duplicate, ACK it again, and not deliver its bytes twice.

## How Part 1 compares to TCP

| Problem | dp Part 1 | TCP |
|---|---|---|
| Is the peer there? | CONNECT / CONNECT_ACK | Three-way handshake (SYN, SYN-ACK, ACK) |
| Did the data arrive? | One ACK per packet | Cumulative ACKs |
| What if nothing comes back? | Fixed 250 ms timeout, retransmit the same packet | Adaptive timeout based on measured round-trip time, plus fast retransmit |
| Identifying data | Packet number plus message offset | One byte sequence number does both jobs |
| The same data twice | Receiver compares with the next expected packet number | Receiver compares with the next expected byte |
| How fast? | One packet in flight (stop-and-wait) | A window of many packets |
| Ending cleanly | CLOSE / CLOSE_ACK, then linger | FIN exchange, then TIME_WAIT |

Several questions ask you to connect what you see in your dp traces to how TCP handles the same situation.

## The network simulator (provided)

All outbound packets pass through `net_simulate()`, which can:

- drop packets at random with probability `-l` (0.0 to 1.0);
- delay packets by a random 0 to `-d` milliseconds;
- drop the **first** transmission of specific packet numbers with `-D` (client: DATA packets; server: ACKs);
- log every event with `-v`.

A drop is silent: `net_simulate()` returns 0 exactly as if the packet had been sent. Your code never learns that a packet was dropped. It finds out the same way a real sender does: no response arrives before the timeout.

Impairment applies only to packets an endpoint **sends**. Give both endpoints the same settings to impair both directions.

```bash
./dp-server -l 0.25 -d 100 -s 42 -v
./dp-client -k 1024 -l 0.25 -d 100 -s 43 -v
./dp-client -k 64 -D 17 -v          # lose DATA packet 17 once
./dp-server -D 9 -v                 # lose the ACK for packet 9 once
```

## What you must implement

All TODOs are in `dp-proto.c`. Each TODO comment lists exactly what to do.

| TODO | Function | What it does |
|---|---|---|
| 1 | `dp_connect()` | Send CONNECT and wait for CONNECT_ACK; retransmit the same CONNECT on timeout |
| 2 | `send_data_packet()` | Stop-and-wait for one DATA packet: send, wait for the matching ACK, retransmit on timeout |
| 3 | `dp_recv()` | Receive and reassemble one message: accept the expected packet, re-ACK duplicates (logging them as `DUP`), answer a repeated CONNECT, require FIN at the end |
| 4 | `dp_close()` | Send CLOSE and wait for CLOSE_ACK; retransmit on timeout |

### Provided helpers

Read these before you start:

- `recv_packet()` receives and validates one packet using `SO_RCVTIMEO`.
- `recv_until()` and `deadline_after()` wait for a packet against a fixed deadline. `SO_RCVTIMEO` restarts on every `recvfrom()`, so waiting with a plain timeout lets a stream of unrelated packets keep restarting the timer. Use one deadline per transmission.
- `send_ack()`, `send_connect_ack()`, `send_close_ack()` send header-only control packets.
- `log_net_event()` writes a NET log line.
- `dp_accept()`, `dp_send()`, and `dp_wait_for_close()` are complete.

**Provided (do not modify):** everything except the four TODO function bodies, including `net_simulate()`, `dp-client.c`, `dp-server.c`, `dp-log.c`, `test.sh`, `params.sh`, `evidence.sh`, and the `Makefile`. Keeping these identical for everyone makes results comparable.

## Building and testing

```bash
make            # build dp-client, dp-server, dp-log
make test       # 6 test cases, about 90 seconds when working
make merge      # merge client/server logs into merged-*.log
make clean
```

Until you implement the TODOs, every test fails quickly with a `NOT IMPLEMENTED` message. `testing.md` describes every test, the pass criteria, the log format, and what to look for in traces. Read it before debugging.

A test passes only when both endpoints succeed: the client completes connect, send, and close, and the server receives exactly the expected bytes and verifies every one.

## Your personalized parameters

Each student gets their own values, derived from your Drexel ID (for example `abc123`):

```bash
./params.sh abc123
DREXEL_ID=abc123
DATA_DROP=17       the client loses DATA packet 17 once
ACK_DROP=9         the server loses the ACK for packet 9 once
SIZE_KB=56         transfer size for your runs
DELAY=10           max random delay per direction for your timing run
```

Your interpretation answers are based on runs with **your** parameters, so they will differ from everyone else's.

## Evidence

When all tests pass, generate your evidence:

```bash
make evidence ID=abc123
```

This takes about 2 minutes and creates an `evidence/` directory:

| File | Contents |
|---|---|
| `SUMMARY.txt` | Your parameters, test results, and the key lines from each personalized run |
| `params.txt` | Your parameters |
| `test-output.txt` | Full `make test` output |
| `merged-<test>.txt` | Merged trace for each test case |
| `client-p-data.txt`, `server-p-data.txt`, `merged-p-data.txt` | Your run that loses DATA packet `DATA_DROP` |
| `client-p-ack.txt`, `server-p-ack.txt`, `merged-p-ack.txt` | Your run that loses the ACK for packet `ACK_DROP` |
| `client-p-delay.txt`, `server-p-delay.txt`, `merged-p-delay.txt` | Your timing run with 0 to `DELAY` ms delay each way |

Commit the whole `evidence/` directory. Re-run `make evidence` after any code change so the evidence matches your final code.

## Interpretation questions

The questions are in `questions.md`. Answer them in that file. In summary:

| # | Topic | Points |
|---|---|---:|
| P | Prediction (committed **before** you generate evidence) | 5 |
| 1 | A timeout is not proof of loss | 8 |
| 2 | Duplicate detection | 8 |
| 3 | Packet number vs message offset | 7 |
| 4 | FIN vs message length | 5 |
| 5 | Stop-and-wait performance | 10 |
| 6 | Socket timeouts and deadlines | 6 |
| 7 | Closing reliably | 6 |

**About the prediction.** Before running `make evidence`, fill in the Prediction section of `questions.md` using only your parameters and your understanding of the protocol, then **commit and push it**. The TA checks that this commit comes before the commit that adds `evidence/`. You are graded on the reasoning, not on whether the prediction was right.

## Use of AI tools

AI assistance is encouraged. Read the **Using AI on this project** section of [`../README.md`](../README.md) for what that means here. In short: the code is 20% of the grade, most points come from explaining your own evidence, and your answers must match your own traces.

## Part 2

The two parts are independent. Part 2 comes with its own complete starter code, so a Part 1 that is not perfect will not hold you back in Part 2. Each part is graded on its own.

## Submission

All work is submitted through your **private GitHub repository**. Canvas receives only a link.

### Set up once

1. In your repository, create a folder named **`proj-dp-p1`** at the top level, next to your other assignments.
2. Copy **everything** from the course `p1/` folder into it, including the hidden `.gitignore`. Do not rename any files. For example, from the top of your repository:

   ```bash
   cp -r <path-to-course-repo>/PROJ-dp/p1/. proj-dp-p1/
   ```

   The trailing `/.` copies hidden files too.
3. Commit and push right away, before you change anything:

   ```bash
   cd proj-dp-p1
   git add -A .
   git commit -m "proj-dp-p1: starter code"
   git push
   ```

From here on, work inside `proj-dp-p1/`: build, test, generate evidence, and answer questions there.

### While you work

- **Commit as you go.** The TA looks for incremental work on `dp-proto.c`, not one final commit.
- **Commit your Prediction before generating evidence.** Fill in the Prediction section of `questions.md`, then `git add questions.md && git commit -m "prediction" && git push`, and only then run `make evidence`.
- The provided `.gitignore` keeps binaries and scratch logs out of your repository. The `evidence/` folder uses `.txt` files and **must** be committed.

### Final checklist

Run these inside `proj-dp-p1/`:

```bash
make clean
make
make test                       # all 6 PASS
make evidence ID=<your-drexel-id>   # regenerate from your final code (about 2 minutes)
git add -A .
git commit -m "proj-dp-p1: final submission"
git push
```

Then check on GitHub that `proj-dp-p1/` contains:

```text
proj-dp-p1/
├── dp-proto.c            your implementation
├── questions.md          your answers
├── evidence/             SUMMARY.txt, params.txt, test-output.txt, and the traces
├── .gitignore, README.md, testing.md, rubric.md
├── dp-proto.h, dp-client.c, dp-server.c, dp-log.c
└── Makefile, test.sh, params.sh, evidence.sh
```

### Submit on Canvas

Submit **only the link** to your folder:

```text
https://github.com/<your-account>/<your-repo>/tree/main/proj-dp-p1
```

Open the link while logged in to GitHub and confirm it shows your files and your latest commit. If your default branch is not `main`, use its name in the link.

## Grading

See `rubric.md`. Summary:

| Component | Points |
|---|---:|
| Code (`dp-proto.c`) | 20 |
| Test evidence | 25 |
| Interpretation questions | 55 |
| **Total** | **100** |
