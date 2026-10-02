# The dp Project: Reliable Transport over UDP

CS472 Computer Networks

## Why we are doing this

In class we study TCP, the protocol that gives almost every application a reliable, ordered byte stream. TCP runs on top of IP, which promises nothing: packets can be lost, delayed, reordered, or duplicated, and nobody tells the sender. TCP hides all of that so well that it is easy to forget how much work it is doing.

In this project you do that work yourself. You will build **dp** (the **Drexel Protocol**), a transport protocol that runs on top of **UDP**. UDP is a thin layer over IP: your packets travel between real sockets, and nothing guarantees they arrive. Everything dp promises, your code has to provide.

Along the way you will run into the same problems TCP's designers did:

- How does the sender know a packet arrived?
- What if the data arrives but the acknowledgment is lost?
- How long should the sender wait before trying again?
- What happens when the same data arrives twice?
- Why is waiting for every acknowledgment so slow?
- Why does one lost packet hold up data that has nothing to do with it?
- How do two machines agree that a connection is over?

By the end, you should be able to look at TCP and recognize what each of its mechanisms is for, because you will have needed most of them yourself.

## Why QUIC is our inspiration

**QUIC** is a real, modern transport protocol that also runs on top of UDP. It was developed at Google, standardized by the IETF in 2021 (RFC 9000), and is the foundation of **HTTP/3**. It carries a large share of today's web traffic.

QUIC solves the same problems as TCP, but its designers had decades of TCP experience to learn from, and some of its choices are easier to understand once you have hit the problems yourself. dp borrows a few of them: connection IDs, packet numbers that are never reused, and independent streams inside one connection.

**We are not reimplementing QUIC.** Real QUIC also includes encryption (TLS 1.3), congestion control, flow control, and a carefully engineered wire format. That would be a massive undertaking. Instead, dp is a small protocol that lets us explore a few of QUIC's ideas and see *why* they exist.

## Two deliverables

dp is built in two parts. Each part is a separate assignment with its own starter code, tests, questions, and grade.

| | Part 1 (`p1/`) | Part 2 (`p2/`) |
|---|---|---|
| Title | Reliable Delivery over UDP | Streams and Head-of-Line Blocking |
| You build | Connection setup and close, packet numbers, ACKs, timeouts and retransmission, duplicate detection, fragmentation and reassembly, FIN | A sliding window, selective retransmission with new packet numbers, multiple independent streams, per-stream reassembly |
| Sending style | Stop-and-wait (one packet in flight) | Up to 8 packets in flight |
| Big question | What does it take to make UDP reliable? | Once it is reliable, why is one ordered stream not enough? |
| Submit in | `proj-dp-p1/` | `proj-dp-p2/` |

**The two parts are independent.** Each comes with its own complete starter code, so you can do Part 2 even if your Part 1 is not perfect. Each part is graded on its own.

## dp, TCP, and QUIC side by side

Use this table as a map. Each dp mechanism you build has a counterpart in TCP and in QUIC.

| | dp Part 1 | dp Part 2 | TCP | QUIC |
|---|---|---|---|---|
| Runs on | UDP | UDP | IP (in the OS kernel) | UDP (usually in the application) |
| Connection setup | CONNECT / CONNECT_ACK | same | Three-way handshake (SYN, SYN-ACK, ACK) | Combined with the TLS 1.3 handshake |
| Connection identity | Connection ID | Connection ID | IP addresses and ports | Connection IDs |
| Identifying a packet | Packet number, reused on retransmission | Packet number, **new** on every transmission | Byte sequence number, reused on retransmission | Packet number, never reused |
| Where data belongs | Message offset | Stream offset | The same byte sequence number | Stream offset |
| Acknowledgments | One ACK per packet | One ACK per packet | Cumulative ACKs (plus selective ACKs) | ACK frames covering ranges of packets |
| Packets in flight | One (stop-and-wait) | Fixed window of 8 | Window set by flow and congestion control | Window set by flow and congestion control |
| Detecting loss | Fixed 250 ms timeout | Fixed 250 ms timeout per packet | Timeout adapted to measured round-trip time, plus fast retransmit | Timers and packet-count thresholds based on measured round-trip time |
| Streams | One | Up to 8, independent | One byte stream | Many, independent |
| End of data | FIN on the last packet | FIN per stream | FIN | FIN per stream |
| Closing | CLOSE / CLOSE_ACK, then linger | same | FIN exchange, then TIME_WAIT | CONNECTION_CLOSE, then a draining period |
| Not in dp | Encryption, congestion control, flow control | same | Encryption via TLS on top | Encryption built in |

## How each part works

Both parts follow the same pattern:

1. **Read the part's `README.md`.** It explains what that part adds to dp and what you implement.
2. **Implement the TODOs** in `dp-proto.c`. Everything else (the client and server programs, the network simulator, the test harness, the log tools) is provided so you can focus on the protocol.
3. **Run the tests** (`make test`). A test passes only when the receiver gets every byte exactly once and in the right place.
4. **Get your personalized parameters** (`./params.sh <your-drexel-id>`). Every student gets different values.
5. **Write your Prediction** in `questions.md` and **commit it** before generating evidence.
6. **Generate your evidence** (`make evidence ID=<your-drexel-id>`). This runs the tests and several experiments with your parameters and saves the traces.
7. **Answer the questions** in `questions.md`, using your own traces.

Grading in both parts:

| Component | Points |
|---|---:|
| Code (`dp-proto.c`) | 20 |
| Test evidence | 25 |
| Interpretation questions | 55 |

## Submitting your work

You submit through your **private GitHub repository** for this course. Canvas receives only a link.

| Part | Folder in your repository | Copy into it |
|---|---|---|
| Part 1 | `proj-dp-p1/` | everything in `p1/`, including the hidden `.gitignore` |
| Part 2 | `proj-dp-p2/` | everything in `p2/`, including the hidden `.gitignore` |

Each folder sits at the top level of your repository, next to your other assignments. Work inside that folder: build, test, generate evidence, and answer the questions there. Do not rename the provided files.

When you are done, submit the link to the folder on Canvas, for example:

```text
https://github.com/<your-account>/<your-repo>/tree/main/proj-dp-p1
```

Each part's `README.md` has a final checklist. In short: your `dp-proto.c`, your answered `questions.md`, and the committed `evidence/` folder, with a commit history that shows your Prediction committed before your evidence.

## Using AI on this project

AI tools are part of how software gets built now, and I want you to use them. Ask an AI to explain a concept, to help you read a trace, to figure out why your timer never fires, or to quiz you on why duplicates happen. Used that way, it is one of the best learning tools you will ever have.

I also know an AI can write most of this code for you, and I am not going to pretend I can stop that. So here is how the project is set up, and why:

- **Code is only 20% of the grade.** Most of the points come from explaining what *your* dp did, using evidence from *your* runs with *your* parameters.
- **Your answers must match your own evidence.** A polished answer that does not match your traces earns nothing.
- **Expect dp questions on upcoming quizzes.** The bi-weekly quizzes will include questions specific to the dp project, such as a short dp trace where you explain what happened and why. There, it is just you.
- **Copying another student's evidence is an academic integrity violation,** and the personalized parameters make it easy to spot.

Here is the part I really want you to hear. Letting AI do the work and collecting the grade can feel like getting one over on the instructor. That feeling does not last. It gets replaced, usually in an interview or at your first job, the moment someone asks how reliable transport works, or why a timeout does not prove a packet was lost, and you realize you do not know.

I have dealt with impostor syndrome in my own career. What got me through it was a foundation I had built myself: I could tell myself "I can do this" because I had actually done it. If you let AI carry you through, the impostor syndrome you get later is different and much worse: "I hope nobody figures out that I don't really know what I'm doing." That is a miserable place to be, and you can avoid it.

So use AI the way a good engineer does: to go faster and understand more, not to skip the understanding. If you can explain every line of your traces, you have earned the grade and the confidence that comes with it.
