# QUIC Investigation

## Submission Instructions

Place this completed file in the `05-ReliableUDP` folder of your GitHub Classroom repository. The TA will look for it at:

```
05-ReliableUDP/quic-investigation.md
```

Do not move it to a subfolder or rename it.

---

## Overview and Instructions

Use an AI model of your choice (Claude, ChatGPT, Gemini, etc.) to investigate the QUIC protocol concepts described below. You were introduced to QUIC at a high level in class — this investigation asks you to go deeper on your own, then connect what you learn back to the du-proto implementation you just built.

**The goal is not to summarize QUIC.** The goal is to use AI as an investigation tool to deepen your understanding of transport protocol design, and to critically evaluate your own implementation in light of what you discover.

Provide your answers directly in this file, replacing each `_Your answer here._` placeholder with your response. Each answer should be 2–4 paragraphs. You are expected to reference specific parts of your du-proto implementation — function names, data structures, and design decisions — in your responses. Vague answers that do not connect back to the code will not receive full credit.

---

## Question 1 — QUIC Streams vs. du-proto's Single Channel

Use AI to investigate how QUIC implements streams and what problem they are designed to solve. Pay particular attention to the concept of *head-of-line blocking* and how QUIC streams address it.

Then reflect on du-proto, which supports a single send/receive channel between a client and server.

**1a.** If you wanted to transfer two files simultaneously using du-proto, what would break or become complicated?

_Your answer here._

**1b.** Head-of-line blocking is a well-known limitation of TCP. Does du-proto suffer from the same issue, a different issue, or is the concept not applicable given du-proto's design? Explain your reasoning with reference to how `dpsend()` and `dprecv()` are structured.

_Your answer here._

**1c.** QUIC streams are multiplexed over a single UDP connection. What would you need to add to the `dp_pdu` structure in `du-proto.h` to begin supporting multiple streams? You do not need to implement this — describe the design change and explain why it would be necessary.

_Your answer here._

---

## Question 2 — QUIC Connection IDs vs. du-proto's Socket-Based Identity

Use AI to investigate what a QUIC connection ID is, why it was introduced, and what real-world problem it solves. A useful starting point: think about what happens to an active connection when a mobile device switches from WiFi to LTE.

Then reflect on du-proto, which identifies a connection implicitly through UDP socket state — the IP address and port of the client and server are the effective identity of the session.

**2a.** What would happen to an active du-proto file transfer if the client's IP address changed mid-transfer? Walk through specifically what would break at the socket and protocol level.

_Your answer here._

**2b.** QUIC connection IDs decouple the connection identity from the network path. Is there anything in the current du-proto design that could serve as a starting point for a connection ID concept, or would it need to be built entirely from scratch? Reference specific code or data structures in your answer.

_Your answer here._

**2c.** Connection IDs also have a security motivation. Use AI to explore this briefly — what type of attack do connection IDs help mitigate, and does du-proto have any exposure to a similar threat?

_Your answer here._

---

## Question 3 — Protocol Design Tradeoffs

This question asks you to step back from specific features and think about design philosophy.

du-proto was intentionally kept simple. QUIC is intentionally comprehensive. Use AI to help you think through the following.

**3a.** QUIC's connection establishment includes built-in TLS 1.3 and is designed to complete in 1-RTT, or even 0-RTT for repeat connections. Your du-proto completes connection setup in a single `dpconnect()`/`dplisten()` exchange with no security layer. What has du-proto traded away to achieve this simplicity, and in what real-world deployment scenarios would those tradeoffs be unacceptable?

_Your answer here._

**3b.** Based on your investigation, identify one additional QUIC feature — beyond streams and connection IDs — that you think would be most valuable to add to du-proto if you were to extend it. Justify your choice in terms of specific limitations you observed in du-proto while completing this assignment.

_Your answer here._

---

## AI Conversation Log

Briefly describe how you used AI during this investigation. This does not need to be a full transcript — a honest, reflective summary is what matters. Address the following:

- What were your initial prompts, and did they produce useful results right away or did you need to refine your approach?
- Was there a moment where the AI gave you an answer that seemed incomplete, inconsistent, or that you had to verify? Describe it.
- What was the most useful follow-up question you asked, and why did it help?

_Your answer here._

---

## A Note on How to Use AI Effectively for This

There is a wrong way and a right way to approach this. The wrong way is to ask *"summarize QUIC streams for me"* and paste the result. The right way is to use AI as a knowledgeable conversation partner — start broad, then go narrow, and push back when something is unclear.

A prompt that will serve you well: *"I just built a stop-and-wait protocol over UDP with a simple PDU structure. Explain QUIC streams to me starting from that context."* You will get dramatically better results than a generic question, and you will actually learn something.