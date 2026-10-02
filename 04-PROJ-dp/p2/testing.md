# dp Project, Part 2: Testing Guide

This guide explains the test harness, what each test does, how a test decides PASS or FAIL, and how to read the logs. The logs are also the evidence you quote in `questions.md`, so learn to read them early.

## Quick start

```bash
make                     # build dp-client, dp-server, dp-log
make test                # 9 test cases, about 45 seconds when working
make merge               # merge each client/server log pair into merged-<test>.log
make hol                 # loss-rate experiment, about 2 minutes
make evidence ID=abc123  # everything you submit, about 4 minutes
make clean               # remove binaries, objects, and top-level logs (not evidence/)
```

Until you implement the TODOs, every test fails within about a second with a `NOT IMPLEMENTED` message in `client-*.log` or `server-*.log`.

## What `make test` does

For each case, `test.sh`:

1. starts `dp-server` in the background with the case's settings and `-v`;
2. runs `dp-client` with the case's settings and `-v`;
3. waits for the server to exit on its own (it lingers about 2 s after close to answer a retransmitted CLOSE);
4. checks the result and moves on. Every case runs even if an earlier one fails.

Impairment (`-l`, `-d`, `-D`) applies only to packets the endpoint **sends**. Giving both endpoints the same settings impairs both directions.

## The nine test cases

All cases send the default files (128, 32, 8 KB = 172032 bytes) unless noted.

| Test | Mode | Impairment | What it exercises |
|---|---|---|---|
| `clean-multi` | multi | none | Window, three streams, in-order delivery per stream |
| `clean-single` | single | none | Same data on one stream with application frames |
| `loss10-multi` | multi | 10% loss both ways | Timeouts, retransmission with new packet numbers |
| `loss10-single` | single | 10% loss both ways | Same, one ordered stream |
| `reorder-multi` | multi | 0-40 ms delay both ways | Packets arrive **out of order**; receiver must buffer |
| `both20-single` | single | 20% loss + 0-20 ms delay both ways | Everything at once |
| `slowack-multi` | multi | server ACKs delayed 0-400 ms; files 16, 8, 4 KB | Timeouts and duplicates with **no loss** |
| `hol-multi` | multi | client always drops DATA packet 4 | File 3 must finish in **under 100 ms** |
| `hol-single` | single | client always drops DATA packet 4 | File 3 must finish **at or after 200 ms** |

Seeds are fixed so drop patterns repeat while you debug. Timing still varies a little between runs.

## How a test passes

All of these must be true:

| Check | Catches |
|---|---|
| Client exits 0 | Connect, flush, or close failed (for example, a chunk exceeded the retry limit) |
| Server exits 0 | Receive error, protocol violation, incomplete file, or close never completed |
| `dp-server: received N bytes` with the exact N | Missing data |
| `dp-server: verify: PASS` | Bytes in the wrong place, or a file delivered to the wrong stream |
| HOL cases only: file 3's `complete_ms` | Streams not independent (`hol-multi`) or in-order delivery not enforced (`hol-single`) |

Each file has its own byte pattern, so data delivered to the wrong file or offset is caught.

A failing case prints the failed checks:

```text
test: hol-multi FAIL: file3-blocked(257.1ms,expected<100)
```

## Status lines

From a passing `loss10-multi` run:

```text
dp-client: mode=multi files=3 bytes=172032 window=8
dp-client: sent 172032 bytes, all acknowledged in 3618.6 ms
dp-client: network: attempted=463 dropped=47 delivered=416 delayed=0 avg-delay=0.00ms retransmits=100
```

```text
dp-server: RESULT file=3 stream=3 bytes=8192 complete_ms=256.2 verify=PASS
dp-server: RESULT file=2 stream=2 bytes=32768 complete_ms=1283.6 verify=PASS
dp-server: RESULT file=1 stream=1 bytes=131072 complete_ms=3092.8 verify=PASS
dp-server: received 172032 bytes
dp-server: verify: PASS
dp-server: network: attempted=416 dropped=53 delivered=363 delayed=0 avg-delay=0.00ms duplicates=53
```

- `RESULT`: one line per file, printed the moment its last byte is delivered in order. `complete_ms` is measured from `dp_accept()`. In single mode, `stream=0` for every file.
- `attempted`, `dropped`, `delivered`: every packet this endpoint tried to send, how many the simulator dropped, and how many actually left. `delivered = attempted - dropped` once the program exits.
- `retransmits` (client): chunks sent more than once.
- `duplicates` (server): chunks received more than once.

## NET lines

With `-v`, each packet event is logged with an absolute timestamp:

```text
dp-client: NET: 1790881099.374514 OUT DATA      pkt=4 sid=1 off=480 bytes=508 DROP (forced)
dp-client: NET: 1790881099.634103 TIMEOUT DATA      pkt=4 sid=1 off=480 retransmit with new pkt
dp-client: NET: 1790881099.634164 OUT DATA      pkt=362 sid=1 off=480 bytes=508
dp-client: NET: 1790881099.636924 IN  ACK       pkt=362 sid=1 off=480 bytes=28
```

| Field | Meaning |
|---|---|
| `OUT` / `IN ` | Sent (or dropped) by this endpoint / received from the peer |
| `TIMEOUT` | The packet's deadline passed; its chunk is queued for retransmission |
| `DUP` | The server received a chunk it already had |
| `pkt=N` | Packet number. For an ACK, the number of the DATA packet it acknowledges |
| `sid=S off=O` | Stream and byte offset of the chunk (DATA and ACK only) |
| `FIN` | Last chunk of its stream |
| `bytes=N` | On-the-wire size: 28-byte header + payload (508 for a full chunk) |
| `DROP` | Discarded by the simulator at random (`-l`) |
| `DROP (forced)` | Discarded because of `-D` |
| `delayed=Nms` | This packet waited N ms in the simulator's delay queue. The line is logged when it actually left |

Two things to keep in mind:

- **A chunk is identified by `sid` and `off`, not by `pkt`.** To follow one piece of data through retransmissions, search for `sid=1 off=480 ` (with the trailing space), not for a packet number.
- **Delayed packets can overtake each other.** With `-d`, the order packets arrive in is not the order they were sent.

## Merged logs

`make merge` (and `make evidence`) combine the two logs by timestamp and show milliseconds since the first event:

```text
  285.120 ms  dp-client: NET: OUT DATA      pkt=1 sid=1 off=0 bytes=508
  285.436 ms  dp-server: NET: IN  DATA      pkt=1 sid=1 off=0 bytes=508
  540.606 ms  dp-client: NET: TIMEOUT DATA      pkt=1 sid=1 off=0 retransmit with new pkt
  540.701 ms  dp-client: NET: OUT DATA      pkt=18 sid=1 off=0 bytes=508
  540.843 ms  dp-server: NET: IN  DATA      pkt=18 sid=1 off=0 bytes=508
  540.867 ms  dp-server: NET: DUP DATA      pkt=18 sid=1 off=0 already have this data
  659.055 ms  dp-server: NET: OUT ACK       pkt=1 sid=1 off=0 bytes=28 delayed=373ms
  659.154 ms  dp-client: NET: IN  ACK       pkt=1 sid=1 off=0 bytes=28
  663.386 ms  dp-server: NET: OUT ACK       pkt=18 sid=1 off=0 bytes=28 delayed=121ms
  663.587 ms  dp-client: NET: IN  ACK       pkt=18 sid=1 off=0 bytes=28
```

(From `slowack-multi`. Packet 1 arrived, but its ACK was delayed past the 250 ms timeout, so the chunk was sent again as packet 18. The server recognized the duplicate by its offset. The late ACK for packet 1 arrives after the sender has given up on that number, so the sender ignores it.)

Status lines are not included in merged logs.

## What to look for

1. **The window at work** (`merged-clean-multi.log`): several `OUT DATA` lines before the first `IN ACK`. Compare with Part 1, where every DATA waited for its ACK.
2. **Round-robin interleaving**: in multi mode, `sid` cycles 1, 2, 3, 1, 2, 3... until a file runs out.
3. **Reordering** (`merged-reorder-multi.log`): the server's `IN DATA` lines arrive out of packet-number order, yet the files verify.
4. **Retransmission with a new number** (`merged-loss10-multi.log`): find a DROP, its TIMEOUT about 250 ms later, and the same `sid`/`off` sent under a new `pkt`.
5. **Duplicates without loss** (`merged-slowack-multi.log`): DUP lines with no DROP anywhere.
6. **Head-of-line blocking** (`server-hol-multi.log` vs `server-hol-single.log`): compare file 3's `complete_ms`.

Sanity checks:

- In loss-only cases (`loss10-*`), the number of client TIMEOUT lines equals the total number of DROP lines across both logs: every lost DATA or ACK costs exactly one timeout.
- `clean-*` and `reorder-multi` should have no TIMEOUT lines.
- The client's `retransmits` equals its TIMEOUT count.

## Debugging tips

- **Run one case by hand** with the arguments from `test.sh`, in two terminals.
- **Start small:** `./dp-client -k 1,1,1 -m multi -v` sends three 1 KB files (three chunks each, the last one 64 bytes).
- **Sender stuck?** Check that `dp_flush()` returns when nothing is in flight, and that `handle_ack()` decrements `inflight_count`.
- **Server never completes a file?** Check FIN handling and `advance_contiguous()` for the short last chunk.
- **`hol-multi` fails but other tests pass?** Your receiver is probably delivering streams in connection order instead of independently.
- **Try other seeds.** The TA will.
