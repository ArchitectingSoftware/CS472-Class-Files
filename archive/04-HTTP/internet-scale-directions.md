# Internet-Scale HTTP Infrastructure - Independent Research

**Assignment Component:** Required (20 points)  
**Time Estimate:** 60-90 minutes  
**Skills:** Systems Thinking, Infrastructure Understanding, Self-Directed Learning

---

## The Big Picture

You've implemented an HTTP client with Keep-Alive that saves approximately 300ms by reusing connections. That's a real optimization! But what happens when your client needs to serve millions of users across the globe? What if your server gets hit with a DDoS attack? What if users in Australia are 250ms away from your server just due to the speed of light?

**The reality:** Even a perfectly implemented HTTP client needs massive infrastructure support to work at internet scale. Services like Cloudflare, Akamai, and AWS CloudFront exist to solve problems that the HTTP protocol itself cannot address.

**Your task:** Use AI tools to independently research Content Delivery Networks (CDNs) and internet-scale infrastructure. Understand what happens when this infrastructure fails and connect it back to the HTTP client you just built.

---

## Why This Matters

**In this assignment:**
- You optimized connection reuse (Keep-Alive)
- You measured timing differences with `/delay/2`
- You learned how HTTP/1.1 works at the protocol level

**At internet scale:**
- Your optimizations still matter, but they're just one small piece
- CDNs cache content at the edge (maybe 0 requests to origin server!)
- Geographic distribution matters (speed of light: 40ms per 4,000 miles)
- DDoS protection matters (can your server handle 10M requests/second?)
- The infrastructure itself can become a single point of failure

**Career relevance:**
- Every modern web service uses CDNs (Netflix, Spotify, GitHub, etc.)
- Understanding infrastructure tradeoffs is crucial for system design
- Outages teach us about resilience and dependencies
- "The internet is down" is often "Cloudflare is down"

---

## Research Questions to Explore

Use AI tools (ChatGPT, Claude, Gemini, etc.) to research and understand:

### Core Infrastructure Understanding
1. **What is a CDN (Content Delivery Network)?** How does it work at a high level?
2. **What is Cloudflare?** What services do they provide beyond just caching?
3. **How does a request flow through a CDN?** What happens between your client and the origin server?
4. **Why can't HTTP/Keep-Alive solve these problems alone?**

### Failure and Scale Analysis
5. **Find a recent major CDN outage** (Cloudflare, AWS CloudFront, Fastly, etc. - within last 1-2 years)
6. **What broke? What sites/services went down?**
7. **Why does one company's failure affect so much of the internet?**
8. **What are the tradeoffs of centralized infrastructure?**

### Connection to Your Work
9. **How does CDN caching compare to your Keep-Alive optimization?**
10. **Would your HTTP client benefit from connecting to a CDN edge server vs httpbin.org directly?**

---

## What You Need to Deliver

**File:** `internet-scale.md`

Create this markdown file in your `04-HTTP` directory with the following sections. Be concise but demonstrate understanding.

---

### Section 1: Learning Process (3 points)

**Briefly document:**
- What AI tool(s) did you use?
- Give 2-3 example prompts you used to learn about CDNs/Cloudflare
- What was the most surprising thing you learned?

**What to include:**
- Be specific about which AI tools (not just "I used AI")
- Show actual prompts you typed, not paraphrased descriptions
- Reflect on something genuinely interesting or unexpected you discovered

---

### Section 2: CDN Overview (7 points)

**Answer these questions:**

**What is a CDN and what problems does it solve?**
- Explain in 3-4 sentences what a CDN does
- List 3 specific problems CDNs solve that HTTP/your client cannot

**How does Cloudflare specifically help?**
- Name at least 3 services Cloudflare provides (beyond just caching)
- Briefly explain each in 1 sentence

**Guidelines for strong answers:**
- Be specific - avoid vague terms like "makes things faster"
- Connect to technical concepts: latency, bandwidth, server load, attacks
- Think about what your HTTP client can vs cannot do
- Use concrete examples when possible (distances, times, numbers)

---

### Section 3: Outage Analysis (6 points)

**Research a recent major CDN outage and answer:**

**What outage did you research?**
- Which company? (Cloudflare, Fastly, AWS, Akamai, etc.)
- When did it happen? (month/year)
- How long did it last?

**What broke and what was the impact?**
- What technical failure occurred? (Be specific but concise)
- What websites/services went offline? (Name at least 3-5 major ones)
- How widespread was the disruption?

**Why did one company's failure cause such widespread impact?**
- Explain the centralization problem
- What does this teach us about infrastructure dependencies?

**Guidelines for strong answers:**
- Research actual outages - Cloudflare, Fastly, AWS CloudFront all had major incidents 2021-2024
- Be specific: exact dates, duration, what technically failed
- Name real services that went down (Discord, Shopify, etc.)
- Explain WHY one provider's failure cascaded so widely
- Think about tradeoffs: efficiency vs resilience, centralization vs redundancy

**Note:** Any major CDN outage from 2023-2025 is acceptable. Search for terms like "Cloudflare outage 2024" or "Fastly CDN outage" to find incidents.

---

### Section 4: Connecting to Your HTTP Client (4 points)

**Answer these questions:**

**Compare your Keep-Alive optimization to CDN caching:**
- Your client saved time by reusing connections
- How is CDN caching similar? How is it different/better?

**Would your client benefit from connecting to a CDN edge server instead of directly to httpbin.org?**
- Assume you're in Philadelphia and httpbin.org's origin server is in Virginia
- Cloudflare has an edge server in Philadelphia
- What advantages would the CDN provide? Be specific.

**What's the bigger lesson?**
- Why isn't "just implement the protocol correctly" enough for internet-scale services?

**Guidelines for strong answers:**
- Use specific numbers from your timing.txt results
- Think about: latency (ms), number of requests to origin, connection overhead
- Consider what happens with 1000 users vs just you
- Address multiple concerns: speed, reliability, security, scale
- Connect back to your actual HTTP client implementation
- Think about layers: protocol layer vs infrastructure layer

---

## Hints for Success

### Finding Good Outages to Research

Ask your AI: "What was a major CDN outage in 2023 or 2024? I need to analyze the impact and what went wrong."

Good sources:
- Cloudflare's blog (they publish detailed post-mortems)
- Tech news sites (TechCrunch, Ars Technica, The Verge)
- Hacker News discussions (often have technical analysis)
- Company status pages and incident reports

### Understanding the Scale

**Think about these numbers:**
- Your Keep-Alive saves: approximately 100-300ms per request
- Cross-country latency: approximately 40-80ms (speed of light limit)
- CDN cache hit: approximately 5-10ms (nearby edge server)
- Origin server load: 1000 requests becomes 10 requests (99% cache hit rate)

**The point:** Every layer matters. Your optimization is real, but it's one piece of a much larger system.

### Common Misconceptions to Avoid

**Incorrect:** "CDNs just make things faster"  
**Correct:** CDNs improve speed, security, reliability, and scalability

**Incorrect:** "HTTP/1.1 Keep-Alive is the same as CDN caching"  
**Correct:** Keep-Alive reuses connections; CDNs cache content and distribute it geographically

**Incorrect:** "Outages only affect one company"  
**Correct:** When a major CDN fails, it can take down thousands of websites simultaneously

---

## Submission

Place your completed `internet-scale.md` file in your `04-HTTP` directory in your GitHub repository. This file should be included when you submit your repository link in Canvas.

---

## Final Thought

You built an HTTP client that correctly implements Keep-Alive connection reuse. That's a real optimization that matters in production systems. But internet-scale infrastructure needs much more:

- **Your client** saves milliseconds per request
- **CDNs** save round trips to distant servers
- **Caching** reduces origin load by 90-99%
- **DDoS protection** keeps services online during attacks
- **Geographic distribution** serves users worldwide efficiently

The tradeoff? Centralization creates efficiency but also creates massive single points of failure. When Cloudflare goes down, huge portions of the internet go with it.

Understanding this infrastructure helps you think like a systems engineer, not just a protocol implementer. Both skills matter.

**Good luck with your research!**