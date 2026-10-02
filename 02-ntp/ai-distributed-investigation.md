# Time in Distributed Systems Investigation

**Points:** 20 (required component)  
**File to submit:** `time-in-distributed-systems.md`

---

## Overview

You've implemented an NTP client that measures how far your computer's clock is from a time server, and how uncertain that measurement is. But why does time synchronization matter so much in distributed systems? This investigation will help you understand the fundamental role of time in distributed computing and where your NTP implementation fits into the bigger picture.

**Goal:** Understand core distributed systems concepts (logical clocks, CAP theorem, eventual consistency) and how physical time synchronization relates to them.

---

## Investigation Structure

Your investigation should have **5 sections** covering these topics:

1. **Learning Process** (4 points) - Document your AI-assisted learning
2. **Real-World Failure** (4 points) - Research a major time-related incident
3. **Physical vs Logical Time** (4 points) - Understand why timestamps aren't enough
4. **CAP & Eventual Consistency** (4 points) - Fundamental distributed systems tradeoffs
5. **Your NTP Client in Context** (4 points) - Connect concepts to your implementation

---

## Section 1: Learning Process (4 points)

**Document your AI-assisted learning journey.**

Answer these questions:

1. **What AI tools did you use?**
   - ChatGPT, Claude, Gemini, Perplexity, etc.

2. **What were your initial prompts?**
   - List 3-5 specific prompts you used to start learning
   - Example: "Why do distributed systems need synchronized clocks?"
   - Example: "What is the CAP theorem in simple terms?"

3. **What was most confusing initially?**
   - What concept didn't make sense at first?

4. **How did you get clarity?**
   - What follow-up questions helped?
   - What analogies or examples made it click?

5. **What did you have to verify?**
   - Name one thing an AI told you that turned out to be wrong, overstated, or that you could not confirm, and how you checked it.
   - If everything checked out, describe how you verified one specific claim (for example, details of your Section 2 incident).
   - AI tools sometimes invent incidents, dates, and numbers that sound completely plausible. Checking is part of using them well.

**Format:** Write 1-2 paragraphs describing your learning process, including specific prompts you used and what you verified.

---

## Section 2: When Time Goes Wrong - A Real Failure (4 points)

**Research ONE major incident where time/synchronization issues caused significant problems.**

### Pick ONE of these incidents:

1. **2012 Leap Second (June 30, 2012)**
   - Reddit, LinkedIn, Yelp, Foursquare, Mozilla, and others had outages or overloaded servers at the same moment
   - A Linux kernel bug in handling the inserted leap second left timers misbehaving, and many programs (Java applications, MySQL) spun at 100% CPU

2. **Cloudflare DNS Leap Second (January 1, 2017)**
   - A leap second made the system clock appear to go backwards by one second
   - Cloudflare's DNS software computed a negative duration, which it assumed was impossible, and crashed
   - About 0.2% of DNS queries failed at the peak; the worst-hit machines were patched in about 90 minutes

3. **Microsoft Azure Leap Day (February 29, 2012)**
   - Code that created security certificates computed "one year from today" by adding 1 to the year, producing February 29, 2013, a date that does not exist
   - The failure cascaded into a major, hours-long outage across Azure

4. **GPS Timing Error (January 26, 2016)**
   - While a 25-year-old GPS satellite was being decommissioned, a ground-system software error made GPS broadcast UTC time about 13 microseconds off for roughly 12 hours
   - Telecom networks and BBC digital radio transmitters that lock to GPS time reported problems

You may choose a different time-related incident if it has a published postmortem or official report; check with the instructor first.

**Cite your sources.** Include a link to at least one **primary source**: the company's own postmortem, an official report, or the actual bug report. News articles and AI summaries are fine for background, but they are not enough on their own. AI tools sometimes invent incidents or details that sound real, so confirm the facts you use.

### Answer these questions:

1. **What happened?** (2-3 sentences)
   - Brief description of the incident
   - When it occurred and who was affected

2. **What went wrong with time specifically?**
   - Did clocks drift apart between servers?
   - Did time jump forward or backward?
   - Was it a leap second issue?
   - Did NTP fail or behave unexpectedly?

3. **What was the impact?**
   - How long was the outage/incident?
   - Money lost or services affected?
   - Number of users impacted?

4. **Connection to distributed systems concepts:**
   *(Come back to this after completing Sections 3-4)*
   - After learning about CAP and consistency, analyze:
   - Was the root problem a clock being *wrong*, or software assuming something about time that is not always true (for example, "time never goes backwards" or "every year has a February 29")?
   - Could logical clocks have helped prevent this? Why or why not?
   - Did the affected system favor availability or consistency when things went wrong?

**Format:** Two paragraphs - one describing the incident, one analyzing it (complete the analysis after Section 4) - plus your source link(s).

---

## Section 3: Physical Time vs Logical Time (4 points)

**The fundamental challenge:** In distributed systems, you can't rely on wall-clock time alone to order events.

### Part A: Why Physical Clocks Fail (2 points)

**Use AI to research and answer:**

1. **The core problem:**
   - Even with NTP, why can't we perfectly synchronize clocks across machines?
   - What does this mean for determining event order?

2. **Simple scenario:**
   - Server A records event with timestamp: `2:00:00.100`
   - Server B records event with timestamp: `2:00:00.050`
   - Which event happened first?
   - Can we know for certain? Why or why not?

3. **Use your own numbers:**
   - Your NTP client prints a "Final Dispersion," an estimate of how wrong a synchronized clock could still be.
   - If two servers' clocks could each be off by that much, how far apart must two timestamps be before you can trust their order?

**Answer in 3-4 sentences** showing you understand "timestamps ≠ guaranteed ordering"

### Part B: Logical Clocks - The Alternative (2 points)

**Research Lamport Clocks using AI:**

Ask your AI: "What are Lamport clocks and how do they work?"

Then answer:

1. **What's the key idea?**
   - How do logical clocks order events WITHOUT using wall-clock time?
   - Brief explanation of the counter mechanism (3-4 sentences)
   - You don't need to implement it, just understand the concept

2. **What can logical clocks tell us?**
   - What they CAN determine: (hint: happened-before relationships)
   - What they CANNOT determine: (hint: actual time intervals)

3. **If logical clocks solve ordering, why do we still need NTP?**
   - Give 2-3 examples where you need actual wall-clock time
   - Examples: logs, certificates, user-facing timestamps, cache expiration, etc.

**Format:** 2 paragraphs - one explaining Lamport clocks, one explaining when you still need wall-clock time.

---

## Section 4: CAP Theorem and Eventual Consistency (4 points)

**The fundamental tradeoffs in distributed systems.**

### Part A: CAP Theorem (2 points)

**Use AI to research CAP theorem:**

Ask: "Explain the CAP theorem with simple examples"

Then answer:

1. **What does CAP stand for?**
   - **C = Consistency:** (define in one sentence)
   - **A = Availability:** (define in one sentence)
   - **P = Partition tolerance:** (define in one sentence)

2. **The theorem states:** "When a network partition happens, a distributed system must choose between _____ and _____."

   You will often see CAP summarized as "pick any two of three." That phrasing is misleading: in a real network, partitions are not optional, so the actual choice is what to give up *while* a partition lasts. Explain why in one or two sentences.

3. **Real-world examples:**
   During a network partition, which does each of these favor, consistency or availability? Explain briefly.
   - DNS (answers from cached records, which may be out of date)
   - Amazon DynamoDB with its default reads (look up what "eventually consistent reads" means)
   - A bank's account balances replicated across two data centers

4. **How does time sync relate to consistency?**
   - Many strongly consistent systems (for example, those built on the Raft or Paxos protocols) order operations *without* trusting clocks at all. Why is that a safer design?
   - Google Spanner is a famous exception: it uses tightly synchronized clocks (GPS and atomic clocks) and deliberately waits out its clock uncertainty before committing a transaction. Ask your AI why that wait makes timestamps safe to compare.
   - If Spanner had to wait out an uncertainty as large as your client's Final Dispersion, how long would every commit take? (2-3 sentences)

### Part B: Eventual Consistency (2 points)

**Use AI to research:**

Ask: "What is eventual consistency in distributed systems?"

Then answer:

1. **Define eventual consistency** (2-3 sentences)
   - What does "eventual" mean?
   - Give one example (like DNS propagation)

2. **How do clocks actually get corrected?**
   - Your NTP client only *measures* the offset. It never changes your clock.
   - Real time services (ntpd, chrony, and the services built into macOS and Windows) do correct it. Ask your AI: "Do NTP daemons step the clock or slew it, and when?"
   - Why do they usually *slew* (speed up or slow down the clock slightly) instead of *stepping* (jumping straight to the right time)? Connect this to what went wrong in the 2012 leap second or Cloudflare incident.
   - Is a slewing clock "eventually consistent" with the server? Explain.

3. **Key insight:**
   - In an eventually consistent system, do you need perfect clock synchronization?
   - Why or why not? Think about what "eventual" means

**Format:** One paragraph for definitions, one paragraph connecting NTP to eventual consistency.

---

## Section 5: Where Your NTP Client Fits (4 points)

**Connect everything you learned to your actual implementation.**

### Part A: What Your Measurements Show (2 points)

1. **Measure:**

   Run your NTP client 3 times, using **at least two different servers** (for example `pool.ntp.org` and `time.nist.gov`). Wait at least a minute between runs: public servers limit how often a client may ask. Record:

   ```
   Run | Server         | Stratum | Offset (ms) | Delay (ms) | Final Dispersion (ms)
   1   |                |         |             |            |
   2   |                |         |             |            |
   3   |                |         |             |            |
   ```

   Typical NTP accuracy over the Internet: 10-100 milliseconds

2. **Interpret:**
   - The offset formula assumes the request and the reply spent equal time on the network. If they did not, the offset can be wrong by at most **half the delay**. For each run, is your offset larger than delay / 2? What does that tell you about whether your clock is actually off?
   - Do the different servers agree with each other? What would it mean if they did not?
   - Your computer's operating system normally keeps its clock synchronized on its own. Based on your numbers, does yours appear to be doing that? Explain using your measurements.

### Part B: Understanding the Big Picture (2 points)

**Synthesis questions - connect all the concepts:**

1. **Can your NTP client solve the logical ordering problem from Section 3?**
   - Yes or No?
   - Explain why in 1-2 sentences

2. **What does your NTP client actually provide?**
   
   Pick the BEST answer and explain your choice:
   - [ ] A way to order all events in a distributed system
   - [ ] A shared reference time for logs, certificates, and coordination
   - [ ] Perfect synchronization between all machines
   - [ ] A replacement for logical clocks like Lamport clocks
   
   **Your choice:** _____
   
   **Why?** (2-3 sentences)

3. **Complete the picture:**
   Fill in these statements based on everything you learned:
   
   - "Physical clocks (NTP) are needed for: _______________________"
   - "Logical clocks (Lamport) are needed for: _______________________"
   - "In a real distributed system, you typically need: _______________________"

**Format:** Answer each question clearly, showing you understand how all these concepts fit together.

---

## Deliverable Format

**File name:** `time-in-distributed-systems.md`

### Required Elements:

- Clear section headers using markdown (# Section 1, ## Part A, etc.)
- Proper markdown formatting (code blocks, lists, emphasis)
- 2-4 pages total (roughly 800-1500 words)
- Evidence of AI tool usage (show specific prompts in Section 1), including one thing you verified
- A link to at least one primary source for your Section 2 incident
- Your measurement table from Section 5 (at least two servers)

### Markdown Formatting Examples:

```markdown
# Section 1: Learning Process

I used ChatGPT to research distributed systems concepts...

## Part A: Initial Prompts

My first prompt was: "Why do distributed systems need synchronized clocks?"

### Code Examples:
\```
Run 1: offset = 23 ms
\```

**Key insight:** Timestamps don't guarantee ordering
```

---

## Grading Rubric (20 points)

| Section | Points | Excellent (Full Points) | Satisfactory (Partial) | Needs Work (Minimal) |
|---------|--------|-------------------------|------------------------|----------------------|
| **Learning Process** | 4 | Clear prompts with evidence of iterative learning, and a specific claim they verified | Some prompts but vague process, or no verification | No specific prompts or evidence |
| **Real-World Failure** | 4 | Accurate incident with a primary source + thoughtful analysis connecting to concepts | Incident described but weak analysis or no primary source | Inaccurate or invented details, or no connection |
| **Physical vs Logical** | 4 | Strong understanding that timestamps ≠ ordering, explains Lamport clocks clearly | Basic understanding with gaps | Superficial or incorrect |
| **CAP & Consistency** | 4 | All concepts correct, clear understanding of tradeoffs | Most concepts correct, some confusion | Missing or incorrect |
| **Your Implementation** | 4 | Measurement table from 2+ servers, correct delay / 2 reasoning, and thoughtful synthesis | Measurements but weak interpretation | Missing measurements or superficial |

### Grading Notes:

**Full Points (18-20):**
- Shows deep understanding of all concepts
- Clear connection to real-world failures
- Thoughtful analysis of their own implementation
- Evidence of genuine learning through AI interaction

**Satisfactory (14-17):**
- Understands most concepts with minor gaps
- Describes incident but analysis could be deeper
- Basic connection to their implementation
- Some evidence of AI-assisted learning

**Needs Work (10-13):**
- Surface-level understanding
- Weak or missing analysis
- Little connection between concepts
- Minimal evidence of learning

**Insufficient (0-9):**
- Major misconceptions
- Missing sections
- No evidence of AI usage
- No connection to implementation

---

## Tips for Success

### Research Strategy:
1. **Start with Section 1 prompts** - use AI to understand basic concepts
2. **Research your incident early** - read multiple sources, not just one
3. **Ask follow-up questions** - if you don't understand an AI response, dig deeper
4. **Use examples** - ask AI for concrete examples of each concept

### Understanding vs Copying:
- **Good:** "I asked AI about Lamport clocks, and here's what I understood..."
- **Bad:** Copy-pasting long AI explanations without synthesis

### Connecting to Your Code:
- Run your NTP client and use actual measurements
- Think about what your implementation can and cannot do
- Be honest about limitations

### Time Management:
- Budget 2-3 hours for research and writing
- Don't try to do it all in one sitting
- Come back to Section 2's analysis after learning the concepts

---

## Why This Matters

After completing this investigation, you'll understand:

1. **Real consequences** - Time failures cost real money and cause real outages
2. **Fundamental limits** - Why timestamps alone can't solve distributed ordering
3. **Core tradeoffs** - CAP theorem shapes every distributed system
4. **Eventual consistency** - Many systems succeed without perfect synchronization
5. **Your contribution** - Where NTP fits in the distributed systems toolbox

Your NTP client is a small but important piece of the distributed systems puzzle. This investigation helps you see the full picture.

---

## Resources

- Your AI tool of choice (ChatGPT, Claude, Gemini, etc.)
- [Lamport's "Time, Clocks" paper](https://lamport.azurewebsites.net/pubs/time-clocks.pdf) - optional, if you want the original
- Your NTP implementation and test results
- Incident reports: search for the company's own postmortem or an official report (AI can help you find them, but read the source itself)

---

**Remember:** This investigation is about YOUR learning journey. Use AI as a teaching assistant, not as a ghostwriter. We want to see what YOU understand, not what AI can explain.

Good luck! 