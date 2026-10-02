# dp Project, Part 2: Streams and Head-of-Line Blocking

> Start with the project overview in [`../README.md`](../README.md). It explains what dp is, why we build it, how it relates to TCP and QUIC, and how we expect you to use AI.

## Overview

In Part 1 you made dp (the Drexel Protocol) reliable over UDP: one stream, stop-and-wait, one packet in flight. It works, but it has two limits that matter in practice:

1. **It is slow.** The sender waits a full round trip for every packet.
2. **Everything is one ordered byte stream.** If an application sends several independent things (say, three files, or the HTML, CSS, and images of a web page), a single lost packet holds up all of them, even the parts that already arrived. This is **head-of-line (HOL) blocking**, and it is exactly what happens when HTTP/2 runs over a single TCP connection.

QUIC addresses both with a send window and **multiple independent streams** inside one connection, which is a big part of why HTTP/3 runs over QUIC instead of TCP. In Part 2 you add those ideas to dp and then measure the difference.

As before, dp is **QUIC-inspired, not QUIC**. There is no TLS, congestion control, flow control, or QUIC wire format.

## Your starting point

Part 2 is independent of Part 1. You do **not** build on your own Part 1 code: this starter is complete on its own. Connection setup, close, timers, and the network simulator already work, so everyone starts from the same base and you can focus on what is new in Part 2.

## Learning goals

By the end of Part 2 you should be able to:

1. Explain why a sliding window outperforms stop-and-wait, and implement one with per-packet timers.
2. Explain why a retransmission gets a **new packet number** while its data keeps the same **stream offset**.
3. Reassemble data that arrives out of order, per stream, and deliver it in order.
4. Explain head-of-line blocking and show it in your own measurements.
5. Interpret protocol traces to explain *why* your implementation behaved as it did.

## What changed from Part 1

| | Part 1 | Part 2 |
|---|---|---|
| Streams | One (stream 0) | Up to 8 per connection, independent |
| Sending | Stop-and-wait | Window of up to 8 DATA packets in flight (`-w`) |
| Retransmission | Same packet number | **New** packet number, same stream ID and offset |
| Offset field | `message_offset` | `stream_offset`: position in the stream |
| End of data | `message_length` + FIN | FIN only (`message_length` is gone) |
| Receiver | Accepts only the next packet | Buffers out-of-order data per stream, delivers in order |
| Duplicates detected by | Packet number | Stream offset |
| Simulator | Delay by sleeping | Delay queue (packets can be **reordered**), plus `-D` to drop specific packet numbers |

The header is still 28 bytes. ACK packets echo the acknowledged DATA packet's number, stream, and offset, which makes traces easy to read.

## How Part 2 compares to TCP

| Problem | dp Part 2 | TCP |
|---|---|---|
| Waiting a round trip per packet | Fixed window: up to 8 packets in flight | Sliding window sized by flow and congestion control |
| Which packet was lost? | Per-packet timer; resend only that chunk (selective) | Timeout plus fast retransmit; selective ACKs let it resend only what is missing |
| Identifying a retransmission | **New** packet number, same stream offset | Same byte sequence number as the original |
| Independent data | Up to 8 streams, each delivered in order on its own | One byte stream; independent data must wait in line |
| One lost packet | Delays only its own stream | Delays everything behind it (head-of-line blocking) |

Single mode in this assignment behaves like TCP; multi mode behaves like QUIC. Your experiment measures the difference.

## How Part 2 works

### Sender

The application registers each stream's data with `dp_stream_write()`, then calls `dp_flush()`.

Stream data is cut into **chunks** of up to 480 bytes. A chunk is identified by `(stream_id, offset)`. Each time a chunk is sent, it goes out in a DATA packet with a **fresh packet number**:

```text
pkt=4   sid=1 off=480    DROP
...                       (timer for pkt 4 expires)
pkt=362 sid=1 off=480    the same chunk, new packet number
```

`dp_flush()` keeps up to `window` packets in flight. Each in-flight packet has its own deadline. When a deadline passes, that packet is declared lost and only its chunk is queued for retransmission. New data is taken from the streams round-robin, so streams share the window fairly.

Because packet numbers are never reused, an ACK always identifies exactly one transmission. There is no question of whether an ACK refers to the original or the retransmission.

### Receiver

Each stream has its own buffer and a record of which 480-byte chunks have arrived. Chunks may arrive in any order. The stream's **in-order point** (`contiguous`) advances only when every byte before it has arrived. `dp_stream_read()` hands the application newly in-order bytes from any stream.

Every DATA packet is ACKed, including duplicates. A duplicate is recognized by its stream offset (the chunk is already stored), not by its packet number.

### The experiment: two ways to send three files

The client sends three files, interleaved one chunk per packet, round-robin. There are two modes with **identical packet patterns**:

```text
multi   file 1 -> stream 1      each stream delivered in order on its own
        file 2 -> stream 2      (like HTTP/3 over QUIC)
        file 3 -> stream 3

single  files 1, 2, 3 -> application frames -> stream 0
                                one ordered byte stream for everything
                                (like HTTP/2 over a single TCP connection)
```

Your transport code is the same in both modes. What differs is where in-order delivery is enforced. If a packet carrying part of file 1 is lost:

- in **multi** mode, only stream 1 waits; files 2 and 3 finish on time;
- in **single** mode, everything behind the lost chunk on stream 0 waits, including files 2 and 3.

The server prints when each file completes:

```text
dp-server: RESULT file=3 stream=3 bytes=8192 complete_ms=0.9 verify=PASS
```

## What you must implement

All TODOs are in `dp-proto.c`. Each TODO comment lists exactly what to do.

| TODO | Function | What it does |
|---|---|---|
| 1 | `transmit_chunk()` | Send a chunk in a DATA packet with a new packet number; record it in the window |
| 2 | `handle_ack()` | Match an ACK to its in-flight packet by packet number and free the slot |
| 3 | `check_timeouts()` | Declare overdue packets lost; queue their chunks for retransmission |
| 4 | `dp_flush()` | The sliding-window loop: fill, wait, process ACKs, check timers |
| 5 | `accept_data()` | Validate and store a DATA packet in its stream; detect duplicates by offset; handle FIN |
| 6 | `advance_contiguous()` and `next_delivery()` | Advance each stream's in-order point; hand new in-order bytes to the application |

TODOs 1-4 are the sender. TODOs 5-6 are the receiver. The tests need both halves.

**Provided (do not modify):** the network simulator, connection setup and close, timers and receive helpers, the round-robin chunk scheduler (`next_chunk()`), buffer growth (`ensure_capacity()`), the application (`dp-client.c`, `dp-server.c`, `dp-app.h`), `dp-log.c`, `test.sh`, `hol.sh`, `params.sh`, `evidence.sh`, and the `Makefile`. Keeping these identical for everyone makes results comparable.

## Building and testing

```bash
make            # build dp-client, dp-server, dp-log
make test       # 9 test cases, about 45 seconds when working
make merge      # merge client/server logs into merged-*.log
make hol        # head-of-line loss-rate experiment, about 2 minutes
make clean
```

Until you implement the TODOs, every test fails quickly with a `NOT IMPLEMENTED` message. `testing.md` describes every test, the pass criteria, the log format, and what to look for in traces. Read it before debugging.

Useful manual runs (two terminals):

```bash
./dp-server -v
./dp-client -m multi -v              # one stream per file
./dp-client -m single -D 4 -v        # all files on stream 0, drop DATA packet 4
./dp-client -m multi -w 1 -v         # window of 1 (stop-and-wait)
```

## Your personalized parameters

Each student gets their own forced-drop packet number and file sizes, derived from your Drexel ID (for example `abc123`):

```bash
./params.sh abc123
DREXEL_ID=abc123
DROP=6
SIZES=156,20,11
```

Your interpretation answers are based on runs with **your** parameters, so they will differ from everyone else's.

## Evidence

When all tests pass, generate your evidence:

```bash
make evidence ID=abc123
```

This takes about 4 minutes and creates an `evidence/` directory:

| File | Contents |
|---|---|
| `SUMMARY.txt` | Your parameters, test results, file completion times, experiment table |
| `params.txt` | Your DROP and SIZES |
| `test-output.txt` | Full `make test` output |
| `merged-<test>.txt` | Merged trace for each test case |
| `client-p-*.txt`, `server-p-*.txt`, `merged-p-*.txt` | Your four personalized runs: `p-multi`, `p-single` (window 8) and `p-multi-w1`, `p-single-w1` (window 1) |
| `hol-results.txt` | Loss-rate experiment table |

Commit the whole `evidence/` directory. Re-run `make evidence` after any code change so the evidence matches your final code.

## Interpretation questions

The questions are in `questions.md`. Answer them in that file. In summary:

| # | Topic | Points |
|---|---|---:|
| P | Prediction (committed **before** you generate evidence) | 5 |
| 1 | Your dropped packet and its retransmission | 8 |
| 2 | Head-of-line blocking in your runs vs your prediction | 8 |
| 3 | Data that arrived but could not be delivered | 7 |
| 4 | Window size 1 | 8 |
| 5 | Duplicates with no loss, and a deliberate change to your code | 8 |
| 6 | The loss-rate experiment | 8 |
| 7 | Our design vs QUIC | 3 |

**About the prediction.** Before running `make evidence`, fill in the Prediction section of `questions.md` using only your parameters and your understanding of the protocol, then **commit and push it**. The TA checks that this commit comes before the commit that adds `evidence/`. You are graded on the reasoning, not on whether the prediction was right.

## Use of AI tools

AI assistance is encouraged. Read the **Using AI on this project** section of [`../README.md`](../README.md) for what that means here. In short: the code is 20% of the grade, most points come from explaining your own evidence, and your answers must match your own traces.

## Submission

All work is submitted through your **private GitHub repository**. Canvas receives only a link.

### Set up once

1. In your repository, create a folder named **`proj-dp-p2`** at the top level, next to your other assignments.
2. Copy **everything** from the course `p2/` folder into it, including the hidden `.gitignore`. Do not rename any files. For example, from the top of your repository:

   ```bash
   cp -r <path-to-course-repo>/PROJ-dp/p2/. proj-dp-p2/
   ```

   The trailing `/.` copies hidden files too.
3. Commit and push right away, before you change anything:

   ```bash
   cd proj-dp-p2
   git add -A .
   git commit -m "proj-dp-p2: starter code"
   git push
   ```

From here on, work inside `proj-dp-p2/`: build, test, generate evidence, and answer questions there.

### While you work

- **Commit as you go.** The TA looks for incremental work on `dp-proto.c`, not one final commit.
- **Commit your Prediction before generating evidence.** Fill in the Prediction section of `questions.md`, then `git add questions.md && git commit -m "prediction" && git push`, and only then run `make evidence`.
- The provided `.gitignore` keeps binaries and scratch logs out of your repository. The `evidence/` folder uses `.txt` files and **must** be committed.

### Final checklist

Run these inside `proj-dp-p2/`:

```bash
make clean
make
make test                       # all 9 PASS
make evidence ID=<your-drexel-id>   # regenerate from your final code (about 4 minutes)
git add -A .
git commit -m "proj-dp-p2: final submission"
git push
```

Then check on GitHub that `proj-dp-p2/` contains:

```text
proj-dp-p2/
├── dp-proto.c            your implementation
├── questions.md          your answers
├── evidence/             SUMMARY.txt, params.txt, test-output.txt, and the traces
├── .gitignore, README.md, testing.md, rubric.md
├── dp-proto.h, dp-app.h, dp-client.c, dp-server.c, dp-log.c
└── Makefile, test.sh, hol.sh, params.sh, evidence.sh
```

### Submit on Canvas

Submit **only the link** to your folder:

```text
https://github.com/<your-account>/<your-repo>/tree/main/proj-dp-p2
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
