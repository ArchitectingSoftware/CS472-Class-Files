Investigation

Section 1:

1. The AI model I used was ChatGPT.

2. Prompts I gave it:
- "What is the purpose of NTP?" NTP is Network Time Protocol. It is used to synchronize the clocks of computers over a network. It has been in use since the 1980s. It is accurate to the millisecond over the Internet, and accurate to the microsecond on local networks. 
Computers do not keep perfect time. If two computers are communicating with unsynchronized clocks, then their logs and event orders won't line up. 
NTP synchronizes clocks with a series of timestamps.
The Client sends a request. Then the NTP Server receives the request. The Server then replies. The Client receives the reply. Through this, the client calculates the offset and delay of its clock.

- "How are offsets determined in NTP?" Offsets are calculated using the formula ((T2 - T1) + (T3 - T4))/2.

- "Why is UDP used instead of TCP?" I learned that the focus of UDP is to measure time intervals and delays. It isn't going to modify or resend lost packets. TCP, on the other hand will retransmit lost segments, which will create more delays unrelated to the latency of the network. My understanding is that UDP works within the raw time delay of the 
network, without working around it in any way, allowing NTP to measure these delays. 

- I asked "why network byte order conversions are necessary". This is because different machines use different byte orders when storing multi-byte integers. Network protocols use big-endian format as the standard, so if a machine uses little-endian format, the bytes would be interpreted completely wrong. 

3. The most confusing concept initially was how NTP servers could even provide a more accurate time to the client.

4. I asked ChatGPT "how NTP servers could accurately keep track of time". It responded that NTP servers themselves are not perfect clocks. They too need a reference, in the form of higher stratum devices. The highest stratum devices can be atomic clocks, GPS, or radio time signals. Stratum 1 servers are NTP servers connected to Stratum 0 devices. Stratum 2 NTP servers synchronize with Stratum 1 servers over a network. Stratum 3 or above synchronize with Stratum 2.

The AI used the analogy of a marching band, explaining that the Stratum 0 devices are like the conductor with the metronome. Stratum 1 are the first row of musicians, following directly from the conductor. Stratum 2 are the next row following the first row. Stratum 3 and above are the audience trying to clap or tap along. This analogy really helped paint how the accuracy of NTP servers is based on which Stratum they are. Moreover, it explains that NTP servers are not magically accurate all the time, dispelling my nebulous view of NTP servers.

Section 2:

The AWS 2014 Clock Drift:

In August of 2014, Amazon Web Services experienced clock drift in many of its EC2 instances. EC2 is a cloud based virtual server service that allows users to run virtual machines in the cloud. Virtual machines act as though they have their own clock, but in reality are just using the clock of their host machine. Normally, the VM's clock is synched to its host machine's clock.	Under heavy load, some of the VM's clocks were not properly or fully updated. The clock drift between servers lead to inconsistencies in data. DynamoDB uses timestamps for version control. With timestamps being out of order due to clock drift, newer data could be overwritten by older data that appeared to be the latest based on the inaccurate timestamps. DynamoDB also requires that the client's clock is close enough to the server's time. This led to many rejected client requests, as it appeared that their request was expired. This affected thousands of users for a few hours.


The system prioritized availability over consistency. AWS ensured that services would still be available even if nodes had skewed clocks. Logical clocks, on their own, could not have prevented this issue as real time based timestamps were required for authentification issues. This bug occurred because physical time was used as a coordination mechanism without any way of double checking its accuracy from DynamoDB. 


Section 3:

A.
Timestamps from different servers are not guaranteed to reflect accurate timestamps in reference to each other due to a variety of possible reasons including network delays and differences in hardware clock speeds. Despite Server B's timestamp being earlier, Server A's event could have actually happened earlier due to these differences, meaning it is impossible to know for certain what happened first purely through timestamps. 

B.
Lamport clocks are logical clocks used in distributed systems. They are used in order to order events without using physical clock time. The logical time of an event is determined if that event was caused by another even happening. If event B is caused by event A, then event A's logical time has an earlier timestamp than event B. This records the order of events without having to worry about clock drift. Lamport clocks work as such: Each process has a local counter "c". When a local event occurs in that process, "c" is incremented. "c" is included when a message is sent. When the message is received, "c" is set to max(local c, received c) + 1. 

Logical time can tell us the order at which events happened in a process. Logical time can not tell us the actual real world time it took for such events to happen, or how much time passed between events. We still need NTP because we still need to use real-world timestamps. Real time is important for logging which is essential to debugging. Authentification depends on the time interval between the requests. Scheduling events need to know real time in order to take place at the right real world time.



Section 4:

A.
1. CAP stands for Consistency Availability Partition tolerance. 
Consistency: Every read receives the most recent write/ all nodes see the same data at the same time.
Availability: Every request receives a response, even if some nodes fail.
Partition tolerance: The system continues to operate correctly even if network communication failures occur.

2. "You can only guarantee two of the three porperties at the same time".

3. Real world examples:
Traditional bank databases (MySQL): Prioritize Consistency + Availability (CA) (assuming partitions are rare or controlled).

Amazon DynamoDB: Prioritizes Availability + Partition tolerance (AP), allowing eventual consistency.

DNS system: Prioritizes Availability + Partition tolerance (AP), because some stale data is acceptable while ensuring the system keeps responding.


4. 
Strong Consistency systems benefit from tight clock synchronization to order transactions correctly and avoid anomalies. Systems that prioritize Availability and Partition tolerance can tolerate loose clocks, because some temporary inconsistency is acceptable. Essentially, the data will converge eventually.

B.
Eventual consistency is a property of distributed systems where updates to a shared data item will propagate to all replicas over time, so that if no new updates occur, all replicas will eventually converge to the same value. The term eventual emphasizes that consistency is not immediate but guaranteed after some propagation delay. An example is DNS propagation. When you update a domain’s IP address, some DNS servers will still return the old address for a short period until all servers eventually update.

NTP is eventually consistent. When you start an NTP client, your clock does not instantly reach the exact accurate time. Instead, the system gradually adjusts the clock toward the accurate time to avoid large jumps that could disrupt applications. Similarly, in an eventually consistent system, perfect clock synchronization is not always required, because over time all nodes will converge to the correct state or time.

Section 5:

A. 
1. I ran my NTP client 3 times and the offset values I gained were as such:
Run 1: offset = +0.0042 s
Run 2: offset = +0.0033 s
Run 3: offset = +0.0037 s

Average offset: +0.0037 s

2. My NTP client provides eventual consistency. This is because it doesn't ensure that all clocks are always synchronized to each other. Clock drift and network delay offsets are slightly different each time. Over time, however, repeated synchronizations cause the clocks to converge to around the same time.


B. 
1. My NTP client can not solve the logical ordering problem. NTP gives an estimate of real world time, but it doesn't know the logical order of events. Essentially, NTP doesn't care if event A has to occur before event B, and cannot distinguish that. 

2. My NTP client does provide an estimate of real world time and present them in a human readable way. This can be used for logging, debugging, and authentification. 

3. Physical clocks (NTP) are needed for real world timekeeping, timestamps, accurate logs, and coordinating clock-time based events.

   Logical clocks (Lamport) are needed for tracking the causal order of events.

   In a real distributed system, you typically need both physical and logical clocks to get the most accurate order of events. 







