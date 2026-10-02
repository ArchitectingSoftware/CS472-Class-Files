# dp Project, Part 1: Interpretation Questions

**Name:**
**Drexel ID:**
**Parameters** (paste the output of `./params.sh <your-id>`):

```text

```

## Ground rules

- Answer every question in this file, below the question.
- Base every answer on **your** `evidence/` files. When a question asks for evidence, quote the exact log lines (copy them, keep the timestamps) and name the file they came from.
- An answer that does not match your evidence, or that could have been written without running your code, will not receive credit, however well written.
- Short and precise beats long and general. Most answers need a few sentences plus the quoted lines.

Useful commands:

```bash
grep 'pkt=17 ' evidence/merged-p-data.txt          # one packet's story (note the space)
grep -E 'TIMEOUT|DROP|DUP' evidence/merged-p-ack.txt
grep -v NET evidence/client-p-delay.txt            # status lines only
```

---

## P. Prediction (5 points)

**Complete and commit this section BEFORE you run `make evidence`.** Use only your parameters and your understanding of the protocol. The TA checks that this commit comes before the commit that adds `evidence/`. You are graded on your reasoning, not on whether you turn out to be right.

1. In your `p-data` run the client loses DATA packet `DATA_DROP` once. Describe, step by step, what the client and server will do from that moment until packet `DATA_DROP + 1` is sent. Will the server log a `DUP`? Why or why not?
2. In your `p-ack` run the server loses the ACK for packet `ACK_DROP` once. Same question: step by step, and will the server log a `DUP`?
3. From the client's point of view (only what the client can observe), will these two runs look different? Why?
4. Your `p-delay` run sends `SIZE_KB` KB with a random delay of 0 to `DELAY` ms on every packet in each direction. Estimate how long the transfer will take. Show your arithmetic.

*Your prediction:*

---

## 1. A timeout is not proof of loss (8 points)

Use `evidence/merged-p-data.txt` and `evidence/merged-p-ack.txt`.

a. Quote the lines around the TIMEOUT in each run (from the drop through the next ACK the client receives).

b. In one run the DATA was lost; in the other the DATA arrived and the ACK was lost. List exactly what the client's **transport code** (`send_data_packet()`) observed in each case. Are they different?

   The client's `network:` statistics line *does* differ between `client-p-data.txt` and `client-p-ack.txt`. Quote both lines, explain why they differ, and explain why `send_data_packet()` cannot use that information.

c. Give one more cause of a timeout that involves no loss at all, and point to a TIMEOUT in `evidence/merged-slowack.txt` that shows it. Explain why the client's response is the same in all three cases.

*Answer:*

---

## 2. Duplicate detection (8 points)

Use `evidence/merged-p-ack.txt`.

a. Quote the `DUP` line and the lines just before and after it. Which packet number is it?

b. Explain, step by step, how the server knew this packet was a duplicate. Name the variable in your `dp-proto.c` that it compared against.

c. The server sent an ACK for the duplicate. What would happen to the client if it did not? What would happen to the received message if the server copied the duplicate's payload instead of discarding it? Use your actual `SIZE_KB` and `ACK_DROP` to say exactly which bytes would end up wrong.

*Answer:*

---

## 3. Packet number vs message offset (7 points)

Use `evidence/merged-p-data.txt`.

a. Your lost packet `DATA_DROP` was retransmitted. What were its packet number and message offset on the original transmission and on the retransmission? (The offset is not printed in the log; compute it from the packet number and explain how.)

b. Packet number and message offset both increase by a fixed step here. Explain why they are still two different ideas: what does each one identify, and who uses it? TCP uses a single byte sequence number for both jobs. Why can TCP get away with that?

c. Suppose the protocol later allowed a retransmission to carry a new packet number (QUIC does this). Which field would the receiver then have to use to detect duplicates, and why?

*Answer:*

---

## 4. FIN vs message length (5 points)

Use `evidence/merged-p-data.txt`.

a. Quote the last DATA packet of your transfer. How many payload bytes did it carry, and how do you know from the log line? Check your answer against `SIZE_KB`.

b. Every DATA packet already carries `message_length`. What does FIN tell the receiver that `message_length` does not? Think about Part 2, where one connection will carry several streams that end at different times.

*Answer:*

---

## 5. Stop-and-wait performance (10 points)

Use `evidence/client-p-delay.txt` and `evidence/merged-p-delay.txt`.

a. Report the measured transfer time and the number of DATA packets from the client's `sent` line. Compare with your Prediction.

b. Build a simple model: each DATA packet waits for one round trip, and each direction adds on average half of `DELAY` ms. What does the model predict? By how much (percent) does the measurement differ?

c. Explain the difference. Look at a few consecutive packets in the merged log: how long does one DATA/ACK exchange actually take, compared with the two `delay=` values printed for it?

d. Your `clean` test sent 1 MiB in a few seconds on localhost, where the round trip is a fraction of a millisecond. If the round-trip time were 100 ms, how long would stop-and-wait take for 1 MiB? What change to the protocol would fix this? (Part 2 does it.)

*Answer:*

---

## 6. Socket timeouts and deadlines (6 points)

a. In `merged-p-data.txt`, how long after the drop did the TIMEOUT fire? Where does that number come from in the code?

b. `SO_RCVTIMEO` makes `recvfrom()` give up after a set time. Why would a network program that waits forever for a reply be a poor design? Give one example outside this assignment and what the program should do when the timeout fires.

c. The provided `recv_until()` waits against a fixed deadline rather than calling `recvfrom()` with a fresh `SO_RCVTIMEO` each time. Describe a situation in this protocol where the fresh-timeout approach would wait much longer than 250 ms.

*Answer:*

---

## 7. Closing reliably (6 points)

Use `evidence/merged-loss50.txt` (nearly the same for everyone, since the seeds are fixed).

a. Find a CONN_ACK that was dropped. What did the client do, and which of your functions answered the client's next CONNECT? Quote the lines.

b. Find a CLOSE_ACK that was dropped. Quote the lines showing the client's retransmitted CLOSE being answered.

c. The server keeps answering for about two seconds after the client goes quiet. What would go wrong if it exited immediately after sending its first CLOSE_ACK? Can the client ever be completely sure the server received its final message? (This is a version of the Two Generals problem.) TCP faces the same problem when closing; which TCP state plays the role of dp's linger?

*Answer:*
