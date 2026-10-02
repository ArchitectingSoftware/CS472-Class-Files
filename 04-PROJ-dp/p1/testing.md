# dp Project, Part 1: Testing Guide

This guide explains the provided test harness, what each test does, how a test decides PASS or FAIL, and how to read the logs it produces. Read it before you start debugging. The logs are also the evidence you will cite in `questions.md`.

## Quick start

```bash
make            # build dp-client, dp-server, dp-log
make test       # run all six test cases (about 90 seconds when working)
make merge      # combine client/server logs into merged-*.log
make evidence ID=abc123   # everything you submit, about 2 minutes
make clean      # remove binaries, object files, and top-level logs (not evidence/)
```

Until you implement the TODOs, every test fails quickly with a `NOT IMPLEMENTED` message in `client-*.log`. That is expected.

## What `make test` does

`make test` runs `test.sh`. For each test case it:

1. Starts `dp-server` in the background with that case's impairment settings and `-v`.
2. Waits 0.2 s, then runs `dp-client` with the case's transfer size, impairment settings, and `-v`.
3. Waits for the server to exit on its own. The server lingers about 2 s after close (see "Close linger" below). If the client failed, the harness waits only 1 s. If the server is still running after 15 s, it is killed and recorded as `hung`.
4. Checks the result (see "How a test passes").
5. Keeps going. Every case runs even if an earlier one fails, and the script prints a summary at the end.

Each endpoint applies impairment only to packets **it sends** (outbound). Giving both endpoints the same settings impairs the path in both directions.

## The six test cases

| Test | Size | Server settings | Client settings | What it exercises |
|---|---:|---|---|---|
| `clean` | 1024 KB | none | none | Basic fragmentation, reassembly, and ACKs over 2185 DATA packets |
| `loss10` | 64 KB | 10% loss | 10% loss | Retransmission after lost DATA and lost ACKs |
| `delay20` | 64 KB | 0-20 ms delay | 0-20 ms delay | Variable delay that stays well under the 250 ms timeout (no retransmissions expected) |
| `both20` | 64 KB | 20% loss, 0-20 ms delay | 20% loss, 0-20 ms delay | Loss and delay together |
| `loss50` | 8 KB | 50% loss | 50% loss | Heavy loss, including CONNECT and CLOSE being lost repeatedly |
| `slowack` | 8 KB | 0-400 ms delay | none | ACKs delayed past the 250 ms timeout: timeouts and retransmissions with **zero** loss |

Every impaired case uses a fixed random seed (`-s`) on each endpoint so the drop pattern is repeatable while you debug. Timing still affects the exact trace, so two runs can differ slightly.

## How a test passes

A case passes only if **all** of these are true:

| Check | Where it comes from | What it catches |
|---|---|---|
| Client exits 0 | `dp-client` return code | Connect, send, or close failed (for example, retries exhausted) |
| Server exits 0 | `dp-server` return code | Receive error, verification failure, or close never completed |
| `dp-server: received N bytes` with the exact expected N | `server-<test>.log` | Missing data, or data delivered twice |
| `dp-server: verify: PASS` | `server-<test>.log` | Bytes in the wrong place or corrupted |

The client fills its buffer with a known pattern (byte `i` is `(i * 31 + 17) & 0xff`), so the server can check every byte without an application protocol.

**Why check the server at all?** A sender can believe it finished while the receiver is missing data. For example, if your receiver ACKs a packet it did not actually accept, the sender moves on and never resends it. The client exits cleanly, but the server's byte count is wrong. Only a receiver-side check catches that.

When a case fails, the summary line tells you which checks failed:

```text
test: loss10 FAIL: server-did-not-receive-65536-bytes server-verify-failed
```

## Log files

`make test` writes two logs per case:

```text
client-<test>.log     everything dp-client printed (stdout and stderr)
server-<test>.log     everything dp-server printed (stdout and stderr)
```

`make merge` combines each pair into one timeline:

```text
merged-<test>.log
```

### Status lines

Status lines report milestones and final statistics. From a passing `loss10` run:

```text
dp-client: connected cid=255074937
dp-client: sent 65536 bytes
dp-client: close: PASS
dp-client: network: attempted=178 dropped=19 delivered=159 delayed=0 avg-delay=0.00ms
```

```text
dp-server: accepted cid=255074937
dp-server: received 65536 bytes
dp-server: verify: PASS
dp-server: close: PASS
dp-server: network: attempted=159 dropped=20 delivered=139 delayed=0 avg-delay=0.00ms
```

The `network:` line comes from the simulator's statistics:

- `attempted`: every packet this endpoint tried to send, including retransmissions.
- `dropped`: packets the simulator discarded.
- `delivered`: packets actually handed to `sendto()`. It should equal `attempted - dropped`.
- `delayed` / `avg-delay`: packets given a non-zero delay, and their average delay.

### NET lines (per-packet events)

With `-v`, every packet event is logged with an absolute timestamp (seconds.microseconds):

```text
dp-client: NET: 1790163959.476415 OUT CONNECT     pkt=0 bytes=28
dp-client: NET: 1790163959.476697 IN  CONN_ACK    pkt=0 bytes=28
```

| Field | Meaning |
|---|---|
| `dp-client:` / `dp-server:` | Which endpoint wrote the line |
| timestamp | Wall-clock time, used by `dp-log` to merge the two logs |
| `OUT` | This endpoint sent (or tried to send) a packet |
| `IN ` | This endpoint received a valid packet from its peer |
| `TIMEOUT` | No matching response arrived before the deadline; the packet is being retransmitted |
| type | `CONNECT`, `CONN_ACK`, `DATA`, `ACK`, `CLOSE`, `CLOSE_ACK` |
| `pkt=N` | The packet number. For an ACK or CLOSE_ACK, the number of the packet being acknowledged |
| `bytes=N` | Size on the wire: the 28-byte header plus payload. A full DATA packet is `28 + 480 = 508` |
| `DROP` | The simulator discarded this packet at random (`-l`). The peer never sees it |
| `DROP (forced)` | Discarded because of `-D` (first transmission of that packet number only) |
| `DUP` | The server received a DATA packet it had already delivered, and is ACKing it again |
| `delay=Nms` | The simulator will hold this packet for N ms before sending it |

Two details that matter when reading traces:

- **A delayed packet is logged when it is queued, not when it leaves.** A line such as `OUT ACK pkt=5 delay=331ms` at time T means the ACK actually left around T + 331 ms.
- **The simulator delays by sleeping.** While an endpoint is waiting to send a delayed packet, it cannot receive anything. Incoming packets queue up in the socket and are processed afterward.

### Merged logs

`dp-log` keeps only NET lines, sorts both endpoints' events by timestamp, and replaces absolute time with milliseconds since the first event:

```text
       0.000 ms  dp-client: NET: OUT CONNECT     pkt=0 bytes=28
       0.160 ms  dp-server: NET: IN  CONNECT     pkt=0 bytes=28
       0.191 ms  dp-server: NET: OUT CONN_ACK    pkt=0 bytes=28
       0.282 ms  dp-client: NET: IN  CONN_ACK    pkt=0 bytes=28
       0.327 ms  dp-client: NET: OUT DATA        pkt=1 bytes=508
       0.383 ms  dp-server: NET: IN  DATA        pkt=1 bytes=508
```

Status lines (`received`, `verify`, `network:`) are not in the merged log. Look at the individual logs for those.

Client and server run on the same machine, so their clocks agree and the merged order is reliable.

## What to look for

Work through these in your own merged logs. Several of them are exactly what `questions.md` asks you to find.

### 1. A normal exchange (`merged-clean.log`)

`OUT DATA pkt=N` at the client, `IN DATA pkt=N` at the server, `OUT ACK pkt=N` at the server, `IN ACK pkt=N` at the client, then `pkt=N+1`. Only one DATA packet is ever outstanding: that is stop-and-wait.

Check the packet count. 1 MiB at 480 bytes per packet is 2185 DATA packets (the last carries 256 bytes, so it shows `bytes=284`). The CLOSE uses the next packet number, 2186.

### 2. A lost DATA packet (`merged-loss10.log`)

```text
     257.528 ms  dp-client: NET: OUT DATA        pkt=6 bytes=508 DROP
     512.673 ms  dp-client: NET: TIMEOUT DATA        pkt=6 retransmitting
     512.782 ms  dp-client: NET: OUT DATA        pkt=6 bytes=508
     512.856 ms  dp-server: NET: IN  DATA        pkt=6 bytes=508
     512.874 ms  dp-server: NET: OUT ACK         pkt=6 bytes=28
```

The server never logs `IN DATA pkt=6` for the dropped copy. About 250 ms later the client times out and resends the **same** packet number.

### 3. A lost ACK and a duplicate (`merged-loss10.log`)

```text
       0.487 ms  dp-client: NET: OUT DATA        pkt=2 bytes=508
       0.525 ms  dp-server: NET: IN  DATA        pkt=2 bytes=508
       0.538 ms  dp-server: NET: OUT ACK         pkt=2 bytes=28 DROP
     257.077 ms  dp-client: NET: TIMEOUT DATA        pkt=2 retransmitting
     257.167 ms  dp-client: NET: OUT DATA        pkt=2 bytes=508
     257.250 ms  dp-server: NET: IN  DATA        pkt=2 bytes=508
     257.271 ms  dp-server: NET: DUP DATA        pkt=2 already delivered, re-ACK
     257.298 ms  dp-server: NET: OUT ACK         pkt=2 bytes=28
```

The server received `pkt=2` **twice**. The second copy is a duplicate. Your receiver must log it as `DUP`, ACK it again, and must not copy its payload into the buffer a second time. The server still reports exactly 65536 bytes.

From the client's side, examples 2 and 3 look the same: a TIMEOUT and a retransmission. Only the merged log shows which packet was actually lost.

### 4. A timeout with nothing lost (`merged-slowack.log`)

Look for a `TIMEOUT` where no `DROP` appears anywhere near it. The server's ACK was delayed longer than 250 ms, so the client gave up waiting and retransmitted a packet the server already had. Shortly after, the original ACK arrives late, and later the extra DATA copies reach the server as duplicates. Your sender must ignore ACKs for packets it has already finished.

### 5. Handshake and close under loss (`merged-loss50.log`)

Find retransmitted `CONNECT` or `CLOSE` packets. If a `CONN_ACK` is dropped, the client sends `CONNECT` again and the server must answer again. If a `CLOSE_ACK` is dropped, the client resends `CLOSE`, and the server must still be around to answer it.

### 6. Sanity checks on the numbers

- In the loss-only cases (`loss10`, `loss50`), each drop costs exactly one timeout, so the client's `TIMEOUT` count should equal the total number of `DROP` lines across both logs.
- In `slowack` there are timeouts but no drops.
- In `clean` and `delay20` there should be no timeouts at all. If you see any, something is waiting longer than it should.
- Client `delivered` should exactly equal the number of `IN` lines in the server log, and vice versa. Every packet handed to `sendto()` on localhost arrives.

Handy commands:

```bash
grep -c TIMEOUT client-loss10.log
cat client-loss10.log server-loss10.log | grep -c DROP
grep 'pkt=17 ' merged-loss10.log        # follow one packet's whole story
grep -v NET server-loss10.log           # status lines only
```

## Personalized runs (`make evidence`)

`make evidence ID=<your-drexel-id>` runs the test suite, merges every trace into `evidence/merged-<test>.txt`, and then runs three cases with your parameters from `./params.sh`:

| Run | Server | Client | What it shows |
|---|---|---|---|
| `p-data` | normal | `-D DATA_DROP` | One lost DATA packet: TIMEOUT, retransmission with the same number, no DUP |
| `p-ack` | `-D ACK_DROP` | normal | One lost ACK: TIMEOUT, retransmission, DUP at the server, re-ACK |
| `p-delay` | `-d DELAY` | `-d DELAY` | Stop-and-wait cost: total time with random delay on every packet, no loss |

All three use your `SIZE_KB`. The output goes to `evidence/client-p-*.txt`, `evidence/server-p-*.txt`, and `evidence/merged-p-*.txt`, plus a one-page `evidence/SUMMARY.txt`. Files end in `.txt` so a repository-wide `*.log` ignore rule does not hide them.

## Close linger

After answering `CLOSE`, the provided `dp_wait_for_close()` keeps the server alive until it has heard nothing for 8 timeouts (about 2 s). If its `CLOSE_ACK` was lost, the client resends `CLOSE`, and the server needs to still be listening to answer. This is why the server exits about 2 s after the client in every test.

## Debugging tips

- **Run a single case by hand** in two terminals, using the same arguments as `test.sh`:

  ```bash
  ./dp-server -l 0.10 -s 101 -v
  ./dp-client -k 64 -l 0.10 -s 202 -v
  ```

- **Start small.** Get `clean` passing with `-k 1` (3 packets) before trying 1 MiB.
- **Check the server first.** If the client passes but the server reports the wrong byte count, look at your receiver's duplicate handling and ACK rules.
- **A test that takes much longer than the others** usually means timeouts are firing when they should not, or a stale packet is resetting a timer.
- **Try other seeds.** Passing only with the provided seeds can hide bugs. Change the `-s` values in `test.sh`, or run cases by hand with different seeds. TAs will do this.
