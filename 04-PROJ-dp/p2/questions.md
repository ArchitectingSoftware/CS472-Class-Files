# dp Project, Part 2: Interpretation Questions

**Name:**
**Drexel ID:**
**Parameters** (paste the output of `./params.sh <your-id>`):

```text

```

## Ground rules

- Answer every question in this file, below the question.
- Base every answer on **your** `evidence/` files. When a question asks for evidence, quote the exact log lines (copy them from the file, keep the timestamps) and name the file they came from.
- An answer that does not match your evidence, or that could have been written without running your code, will not receive credit, however well written.
- Short and precise beats long and general. Most answers need a few sentences plus the quoted lines.

Useful commands:

```bash
grep 'pkt=13 ' evidence/merged-p-single.txt        # one packet's story (note the space)
grep 'sid=1 off=480 ' evidence/merged-p-multi.txt   # one chunk's story, every packet that carried it
grep -E 'TIMEOUT|DROP|DUP' evidence/merged-p-multi.txt
grep RESULT evidence/server-p-*.txt
```

---

## P. Prediction (5 points)

**Complete and commit this section BEFORE you run `make evidence`.** Use only your parameters and your understanding of the protocol. The TA checks that this commit comes before the commit that adds `evidence/`. You are graded on your reasoning, not on whether you turn out to be right.

The client sends your three files round-robin, one chunk per DATA packet, starting with packet 1 = file 1, packet 2 = file 2, packet 3 = file 3, and so on. Your DATA packet `DROP` is always dropped the first time it is sent. The retransmission timeout is 250 ms.

1. Which file does your dropped packet belong to? Show how you worked it out.
2. In **multi** mode with window 8, which files do you expect to finish quickly (a few ms) and which about 250 ms later? Why?
3. In **single** mode with window 8, same question.
4. With **window 1**, do you expect multi and single mode to behave differently? Why or why not?

*Your prediction:*

---

## 1. Your dropped packet (8 points)

Use `evidence/merged-p-multi.txt`.

a. Quote the line where your DROP packet was dropped. Which stream and offset did it carry, and which file is that?

b. Quote the TIMEOUT line for it, and the line where the same chunk was sent again. What packet number did the retransmission use? Why is that number so much larger than `DROP`? (Hint: what was the sender doing during the 250 ms?)

c. The retransmission did not reuse packet number `DROP`. Describe a specific situation in which reusing the old number would let the sender misinterpret an ACK, and what wrong conclusion it could draw. (TCP does reuse sequence numbers on retransmission, as dp Part 1 did, and has to live with this problem.)

*Answer:*

---

## 2. Head-of-line blocking in your runs (8 points)

Use the `RESULT` lines in `evidence/server-p-multi.txt` and `evidence/server-p-single.txt` (also in `SUMMARY.txt`).

a. Make a small table of the three completion times in each mode.

b. For each mode, explain which files were delayed by the lost packet and why, in terms of streams and in-order delivery. Which mode behaves like HTTP/2 over a single TCP connection, and why?

c. Compare with your Prediction. Where were you right, where were you wrong, and what did you misunderstand (if anything)?

*Answer:*

---

## 3. Arrived but not delivered (7 points)

Use `evidence/merged-p-single.txt`.

a. Find the time your DROP packet was dropped and the time its retransmission arrived at the server. How many DATA packets did the server receive (`dp-server: NET: IN DATA`) between those two times? Show how you counted (a command is fine).

b. Those packets arrived and were ACKed, yet the application could not use any data from file 3 until the retransmission arrived. Explain why. Name the function and the variable in your `dp-proto.c` that enforce this.

c. In `merged-p-multi.txt`, the same packet was lost. Why could file 3's data be delivered in that run?

*Answer:*

---

## 4. Window size 1 (8 points)

Use the `RESULT` lines for `p-multi-w1` and `p-single-w1`, and compare with `p-multi` and `p-single`.

a. Report the completion times for the two window-1 runs.

b. With window 1, multi mode loses its advantage. Explain why, by describing what the **sender** is doing while the lost packet is outstanding. Quote one or two lines from `evidence/merged-p-multi-w1.txt` that show it.

c. What does this tell you about the relationship between multiple streams and having multiple packets in flight? Relate it to Part 1's stop-and-wait.

*Answer:*

---

## 5. Duplicates with no loss (8 points)

Use `evidence/merged-slowack-multi.txt`. In this test the server delays its ACKs by 0-400 ms and nothing is dropped.

a. Find one `DUP` line. Take its `sid` and `off`, and list **every** packet number that carried that chunk, with the time each was sent and whether each was ACKed. Quote the lines.

b. Explain why the server received this chunk more than once even though no packet was lost.

c. **Change your code on purpose.** In `accept_data()`, disable your duplicate check so a duplicate chunk is copied into the buffer again as if it were new. Rebuild and run `./test.sh`. Does `slowack-multi` still pass? Paste the result line. Explain why, and contrast with Part 1, where delivering a duplicate DATA packet twice would have corrupted the message. **Restore your code afterward** and re-run the tests.

*Answer:*

---

## 6. The loss-rate experiment (8 points)

Use `evidence/hol-results.txt`. (Every student uses the same file sizes here; your numbers will still differ from others' because of timing.)

a. For file 3, compute how much sooner multi mode finished than single mode at each loss rate (ratio or difference).

b. The advantage of independent streams shrinks as loss increases. Explain why. Think about how many losses each file experiences at 1% vs 5% loss, and about what file 3 is waiting for in each mode.

c. Completion times tend to cluster near multiples of about 250 ms. Why?

*Answer:*

---

## 7. Our design vs QUIC (3 points)

Our protocol carries exactly one chunk of one stream in each packet, and every DATA packet gets its own ACK. Real QUIC can put several frames (pieces of several streams, plus ACK information) in one packet, and acknowledges ranges of packet numbers.

Pick **one** of those two QUIC features and explain one concrete way it would change what you saw in your traces.

*Answer:*
