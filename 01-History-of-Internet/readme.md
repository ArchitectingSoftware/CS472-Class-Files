# The History of the Internet from "A Protocol for Packet Network Intercommunication"

## Assignment Context: Why This Paper Matters

You're about to read one of the most important [papers](./cerf74.pdf) in computer science history. Written in 1974 by Vint Cerf and Bob Kahn, this paper essentially invented the Internet as we know it today. But here's the key insight: they weren't trying to create "the Internet." They were solving a much more specific problem - how to connect different packet-switched networks together.

This assignment asks you to engage with seminal material that shaped the digital world you live in. I expect you won't understand everything - that's perfectly normal and intentional. We'll be covering this material in depth throughout the course. Right now, the goal is to:

- **Appreciate the historical context:** Understand what networking looked like before this paper
- **Recognize foundational problems:** See what challenges had to be solved to make internetworking possible
- **Connect past to present:** Realize you encounter the solutions (and remaining problems) every day
- **Learn to learn with AI:** Use modern tools to investigate how these 1974 ideas evolved into today's protocols
- **Understand origins:** The Internet emerged from practical engineering needs, not a grand vision to "connect the world"

The Internet you use daily didn't spring into existence fully formed. It evolved from researchers trying to solve specific technical problems, starting with proving packet switching could work, then figuring out how to connect different packet networks. This paper represents that crucial second step.

## Historical Context: The Revolution Before the Revolution

Before diving into this 1974 paper, it's crucial to understand that Cerf and Kahn were solving the second major networking problem. The first problem - proving packet switching could work at all - had only recently been solved.

### The World Before Packet Switching (pre-1970s)

Circuit Switching dominated telecommunications:

- When you made a phone call, the network created a dedicated physical path from your phone to the destination
- That entire path was reserved for your conversation, even during silence
- Resources were "wasted" when not actively transmitting, but you got guaranteed bandwidth
- Perfect for voice calls, but terrible for computer communication (computers send data in bursts, not steady streams)

### The Packet Switching Revolution (1960s-early 1970s)

Packet Switching was a radical idea:

- Instead of reserving a path, break messages into small "packets"
- Each packet travels independently through the network
- Multiple conversations can share the same physical links
- Much more efficient for "bursty" computer data

But packet switching was unproven and controversial:

- Telephone companies thought it would never work reliably
- Many computer scientists were skeptical
- Early networks like ARPANET (1969) were essentially experiments to prove the concept

### Where This Paper Fits

By 1974, packet switching had been proven to work within individual networks. But now came an even bigger challenge: What if you wanted to connect different packet-switched networks together? This paper essentially invented the Internet by solving that problem.

## Part 1: Problem-Solution Mapping Table

After reading the Cerf-Kahn 1974 paper, create a table identifying key networking problems the authors faced and how their solutions evolved into today's Internet. Remember: they weren't just improving networking - they were inventing internetworking (the "inter-net").

### Table Format

Create a table with three columns as shown below. Complete 6-8 rows total, selecting from the problems listed or identifying others you notice in the paper.

| Problem (from 1974 paper OR modern problem) | Solution Proposed by Paper OR Why Not Addressed | How We See This Today |
|---------------------------------------------|------------------------------------------------|----------------------|
| Example: Different networks had different packet sizes | Gateway fragmentation - break large packets into smaller ones that fit the destination network | I can send huge video files or photos without worrying about size limits - it just works somehow. But sometimes when I'm streaming Netflix or YouTube, the video pauses to buffer or drops to lower quality, probably because of this fragmentation process struggling with my connection |
| Example: How do you know you're really connected to the legitimate website? | The paper doesn't address this at all - authentication and security weren't considered necessary for the basic internetworking problem they were solving | I see the little lock icon in my browser when shopping online, and I've heard about "phishing" sites that try to trick you. But most of the time I just trust that typing "amazon.com" actually takes me to Amazon |

### Key Problems to Consider

Choose from these major challenges - some the authors tackle, others they ignore or dismiss:

#### Problems the Paper Addresses:

1. **Addressing Across Networks:** How do you create addresses that work across different network types?
2. **Different Packet Sizes:** What happens when Network A supports 1000-byte packets but Network B only supports 500-byte packets?
3. **Reliability Across Multiple Networks:** How do you ensure data gets through when it has to cross several different networks?
4. **Flow Control:** How do you prevent fast senders from overwhelming slow receivers across network boundaries?
5. **Routing Between Networks:** How does data find its way from Network A through Network B to reach Network C?
6. **Process-to-Process Communication:** How do programs on different computers (on different networks) establish communication?
7. **Accounting/Billing:** How do you handle charging when data crosses networks owned by different organizations?

#### Problems the Paper Doesn't Address (or dismisses):

1. **Security/Authentication:** When you visit amazon.com, how do you know you're really connected to Amazon and not an imposter site?
2. **Privacy/Encryption:** How do you prevent others from reading your data as it crosses multiple networks?
3. **Network Performance at Scale:** What happens when millions of people try to use the internet simultaneously?
4. **Spam/Abuse Prevention:** How do you prevent bad actors from flooding the network with unwanted traffic?
5. **Content Delivery:** How do you efficiently deliver the same popular content (like a viral video) to millions of people?

### Example Questions to Get You Started

Think about these everyday internet experiences that you probably never question - each connects to a major problem this paper had to solve:

- How does Discord know to show "read" receipts when people see your message? What's happening behind the scenes to track that?
- Why can you send a 4K video file to someone across the world, but sometimes your internet "hiccups" during a video call? What's different about these two scenarios?
- How does your Instagram message find your friend when you're on Verizon WiFi, they're on AT&T cellular, and Instagram's servers are somewhere else entirely?
- Why do some websites load instantly while others take forever, even when they seem like similar sites?
- What happens when you hit "send" on a large file attachment? How does your email system know if it actually made it to the recipient?
- Why does your Netflix stream usually work smoothly, but sometimes buffers or drops to lower quality during "peak hours"?
- How can millions of people use TikTok simultaneously without the whole system crashing from overload?

### Tips for the Table

- **For problems the paper addresses:** Don't worry about technical details you don't understand yet - focus on the core problems and concepts
- **For problems the paper ignores:** Think about why the authors might not have considered this important in 1974. Was it not a problem yet? Was it too hard to solve? Did they think someone else would handle it?
- Use your own words in the solution column - don't copy technical jargon
- Be observant in the "How We See This Today" column - think about your daily Internet experience
- Look for the big picture - this paper essentially invented the architecture of the Internet, but couldn't predict everything

## Part 2: AI-Assisted Protocol Investigation

Now that you've mapped the problems and solutions, choose ONE problem from your table and use an AI assistant to conduct a deep investigation into how it actually works today. This is your opportunity to learn how to learn with modern tools - a crucial skill for any computer scientist.

### The Goal

You're not asking AI to do your homework. You're using AI as an interactive learning tool to trace how a 1974 idea evolved into modern reality. The quality of your investigation depends on the quality of your questions.

### Requirements

#### 1. Choose Your Investigation Target

Pick one problem from your table and formulate a specific modern scenario to investigate. Don't ask generic questions like "explain TCP" - instead, investigate concrete situations.

Good investigation scenarios:

- "What exactly happens when I send a photo from my iPhone on cellular to someone on home WiFi, and how does the system handle the different packet sizes?"
- "Why does my Zoom call sometimes drop completely but when I download a file, it always completes eventually? What's different about how these handle reliability?"
- "When I see the lock icon in my browser, what actually happened behind the scenes to make that appear? How does this relate to what Cerf-Kahn built?"
- "I can watch a 4K video on YouTube with millions of other people. How does this work when the 1974 paper's routing was designed for much smaller networks?"

#### 2. Conduct Your AI Investigation

Use an AI assistant (ChatGPT, Claude, Perplexity, etc.) to investigate your scenario. Document your conversation - you'll submit this.

What makes a good investigation:

- Start with your specific scenario
- Ask follow-up questions when the answer introduces new concepts
- Push for concrete examples ("Can you give me an example of when this would happen?")
- Connect back to the 1974 paper ("How does this relate to what Cerf-Kahn proposed?")
- Ask about edge cases ("What happens if..." or "Why doesn't it work when...")
- Question your assumptions ("I thought X was true, but you said Y - can you explain that?")

You should ask at least 5-6 questions in your investigation. Your first question won't give you everything you need - good learning requires iteration.

#### 3. Synthesize Your Findings

After your investigation, write up what you learned. This isn't just summarizing what the AI said - it's demonstrating what YOU now understand.

### Submission Format for Part 2

Submit a document in your course GitHub repository **(detailed information on filenames and format follows)** directory containing:

#### A. Investigation Overview

- **Problem chosen from table:** [Which row?]
- **Specific scenario:** What modern case are you investigating? (2-3 sentences explaining your concrete scenario)

#### B. Key Questions You Asked

List 4-6 questions showing how your investigation evolved. These should show progression - later questions should build on earlier answers. Format:

1. First question
2. Follow-up question
3. [etc.]

#### C. Connection to 1974 Paper (1-2 paragraphs)

How does the modern implementation relate to what Cerf-Kahn proposed? Consider:

- What parts of their solution are still used today?
- What changed and why?
- What did they not anticipate that had to be added later?

#### D. Surprise Finding (4-5 sentences)

What assumption of yours turned out wrong? What did you learn that contradicted what you thought you knew? Be specific about what surprised you and why.

#### E. AI Conversation Documentation

Include either:

- Full transcript of your conversation (can be copy-pasted), OR
- Condensed summary hitting the key exchanges (minimum 400 words showing the progression of your investigation)

You can format the transcript as:

```
ME: [your question]
AI: [relevant parts of response]
ME: [follow-up]
AI: [relevant parts of response]
```

### What We're Looking For

Good investigations show:

- Iterative questioning (you don't stop at the first answer)
- Critical thinking (you question assumptions and ask for clarification)
- Connection making (you relate modern solutions back to 1974 concepts)
- Personal engagement (you're genuinely curious, not just checking boxes)

Warning signs:

- Single generic question like "Explain how TCP works"
- No follow-up questions
- Just copying AI responses without synthesis
- No connection back to the Cerf-Kahn paper
- Generic "surprise" that could apply to anything

## Part 3: Reflection

Write a brief paragraph (4-5 sentences) reflecting on your overall learning from this assignment:

- Which problem from the paper did you find most surprising or interesting and why?
- What did your AI investigation teach you that you wouldn't have learned from just reading?
- How did this process change your understanding of how the Internet works?

## Submission Requirements

Submit a single markdown file to your course GitHub repository containing:

1. Problem-Solution Mapping Table with 6-8 rows
2. AI Investigation (sections A-E as specified above)
3. Reflection paragraph

Each table entry should be 1-3 sentences per column. Your AI investigation should include clear documentation of your question progression and synthesis of what you learned.

### Where to Submit

- **Repository:** Your course GitHub repository
- **Folder:** `01-History-of-Internet/`
- **Filename:** `history-of-internet-assignment.md` (or similar descriptive name)
- **Format:** Markdown (.md file)

### New to Markdown?

If you're not familiar with markdown formatting, GitHub has an excellent guide:

[GitHub Markdown Guide](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)

Key markdown syntax you'll need for this assignment:
- Headers: `#`, `##`, `###`
- Tables: Use pipes `|` and dashes `-` (see Part 1 table format)
- Bold: `**text**`
- Lists: `-` or `1.` for numbered lists
- Code blocks: Triple backticks ` ``` `

## Grading Rubric (40 points total)

### Problem-Solution Mapping Table (20 points)

**Excellent (18-20 points)**

- Identifies genuine problems from the paper in student's own words
- Shows clear understanding of basic solution approaches (not technical details)
- Makes thoughtful, personal connections to modern internet experience
- Demonstrates critical thinking about why some problems weren't addressed
- Uses original language and examples, not copied text

**Good (15-17 points)**

- Identifies most problems correctly but may miss some nuance
- Shows basic understanding of solutions proposed
- Makes reasonable connections to today's internet
- Some evidence of personal insight, though may rely heavily on provided examples
- Mostly original language with minimal copying

**Satisfactory (12-14 points)**

- Identifies problems but with limited understanding
- Basic grasp of solutions but may miss key concepts
- Superficial connections to modern experience
- Limited evidence of personal engagement with material
- Some copying from paper or generic responses

**Needs Improvement (0-11 points)**

- Fails to identify relevant problems or misunderstands paper's content
- Incorrect or missing solution descriptions
- No meaningful connections to modern internet experience
- Extensive copying from paper or other sources
- Little evidence of actual reading or engagement

### AI-Assisted Investigation (15 points)

**Excellent (14-15 points)**

- Specific, well-defined investigation scenario
- Clear evidence of iterative questioning (5+ quality questions)
- Strong synthesis connecting modern implementation to 1974 paper
- Genuine surprise finding showing changed understanding
- Complete conversation documentation showing learning progression
- Student's own understanding clearly demonstrated, not just AI parroting

**Good (11-13 points)**

- Reasonable investigation scenario
- Multiple questions asked (4-5) showing some iteration
- Basic connection to paper made
- Surprise finding present but may be somewhat generic
- Adequate conversation documentation
- Some evidence of student synthesis

**Satisfactory (8-10 points)**

- Generic or broad investigation scenario
- Limited questioning (2-3 questions) with little follow-up
- Weak connection to paper
- Superficial surprise finding
- Minimal conversation documentation
- Heavy reliance on AI responses without synthesis

**Needs Improvement (0-7 points)**

- No clear scenario or single generic question
- No evidence of iterative investigation
- Missing or incorrect connection to paper
- No genuine surprise finding
- Insufficient documentation
- Just copying AI responses without understanding

### Reflection Paragraph (5 points)

**Excellent (5 points)**

- Thoughtful personal reaction showing genuine engagement with both parts
- Demonstrates understanding of historical significance
- Specific reference to what AI investigation revealed
- Shows original thinking about learning process

**Good (3-4 points)**

- Shows engagement but may be more general
- Basic understanding of paper's importance
- Some reference to AI learning process
- Reasonable personal insight

**Needs Improvement (0-2 points)**

- Generic or copied response
- No evidence of personal engagement
- Fails to reference specific content or learning process

## Academic Integrity and AI Use

This assignment is explicitly designed to teach you how to use AI as a learning tool. You are required to use AI for Part 2 and encouraged to use it to help understand the paper for Part 1.

However:

- Your table entries must be in your own words and show YOUR understanding
- Your AI investigation must show YOUR question progression - the learning process is what we're evaluating
- Your synthesis and connections must be YOUR analysis, not just AI output
- Simply asking AI to "complete this assignment" and copying results will be obvious and will receive minimal credit

The goal is to see YOUR learning journey, even if AI helps guide it. We're evaluating your ability to ask good questions, make connections, and synthesize information - not your ability to copy-paste.

### Acceptable AI use:

- "Help me understand what Cerf-Kahn meant by 'gateway' in section 3"
- "What's a modern example of the fragmentation problem they describe?"
- [The entire Part 2 investigation]

### Unacceptable AI use:

- "Complete this assignment table for me"
- "Write a reflection paragraph about this paper"
- Copying AI responses verbatim without understanding or synthesis

If you have questions about appropriate AI use, ask before submitting.

---

## Submission Checklist

Before you submit, make sure your markdown file includes:

- [ ] **Part 1: Problem-Solution Mapping Table**
  - 6-8 rows completed
  - Three columns: Problem, Solution/Why Not Addressed, How We See This Today
  - Written in your own words (1-3 sentences per column)

- [ ] **Part 2: AI-Assisted Investigation**
  - **Section A:** Investigation overview (problem chosen + specific scenario)
  - **Section B:** 4-6 key questions you asked (showing progression)
  - **Section C:** Connection to 1974 paper (1-2 paragraphs)
  - **Section D:** Surprise finding (4-5 sentences)
  - **Section E:** AI conversation documentation (full transcript or 400+ word summary)

- [ ] **Part 3: Reflection**
  - One paragraph (4-5 sentences)
  - Addresses: most surprising problem, what AI investigation taught you, how understanding changed

- [ ] **File location:** `01-History-of-Internet/studentID-solution.md` (e.g., `bsm23-solution.md`) in your course GitHub repo.  This file must be in the `01-History-of-Intenet` folder in your course GitHub repository to be graded.
- [ ] **Format:** Properly formatted markdown (.md file).  If you are unfamiliar with GitHub's markdown syntax check out this [link](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)