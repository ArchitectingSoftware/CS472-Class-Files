# HW4 - Building an HTTP Client

## Assignment Overview

**Total Points:** 70 points
- Coding Component: 50 points
- Independent Research Component: 20 points

**Submission:** All deliverables must be placed in your individual student GitHub repository under the `04-HTTP` directory. Submit a link to that directory in Canvas.

---

## Directions

**NOTE: There is a lot to read here, but don't freak out, there is not a lot to do**

For this assignment you will be building 2 versions of an operational HTTP client. The client should work with a real HTTP server. Testing for the purposes of grading will be using httpbin.org, although you can use whatever server you want for development, so long as it supports non-encrypted HTTP vs encrypted HTTPS.

**Important:** httpbin.org has been somewhat unstable recently. If you run into issues accessing it, you can use a self-hosted version of that service at `http://cci-p141.cci.drexel.edu/`

Note that the makefile includes test suites for the 2 versions of the program you will be implementing. More on this in a bit.

To make the programming assignment realistic, I am limiting the amount you can receive in any one call from the server to 1024 bytes. Thus, you need to loop around and keep receiving data until you get the entire payload from the server (except of course, if the response is less than 1024 bytes).

---

## Part 1: Simple HTTP Client (15 points)

The first program is `client-cc`, which sends a message to an HTTP server with a header `Connection: Close`. This simplifies things a good bit, making HTTP/1.1 work like HTTP/1.0. The implementation for this client is in `client-cc.c`. I provided a lot of the scaffolding, you just need to implement the parts of the code that I commented with `// TODO: what_you_need_to_do`.

When processing data returned from the server you can just keep looping `recv(...)` calls. When the server is done sending data it will close the socket. When this happens, `recv(...)` will return a negative number instead of the number of bytes read to indicate an error. In this case it is not really an error, but more of an indicator that the server closed the socket. All you need to do is to close the socket on the client end when this happens.

---

## Part 2: A Robust and Practical HTTP Client (25 points)

The second scaffold I provided is for the `client-ka` executable. The source code is in `client-ka.c`. This version must implement the `Connection: Keep-Alive` protocol **properly**. I discussed this in class, but as a refresher, this header basically requires the client to open a socket once, and then it can continue to issue multiple requests over the same socket. When the program terminates the socket should close. However, the client must also be sensitive to the server's behavior, as it can close the socket at any time. Thus care must be taken to properly handle the condition when the program is trying to send to the server and the server closed the socket. This situation requires the client to open a new socket when it detects this situation.

The other key challenge with implementing the `Keep-Alive` protocol is properly managing the `recv(...)` calls. You need to inspect the `Content-Length` header that the server sends back and make sure you properly terminate looping `recv(...)` calls when you get all of the data from the server. In other words, you need to track the amount of data you received, and ensure you don't loop and make an extra `recv(...)` call as this will block the client.

For this program, I wrote most of the code for you, again you need to flush out the sections annotated with `//TODO:`. I wrote several parsing helpers for you and placed them in a utility module called `http.c`. You will not need to modify this module.

### Understanding the Keep-Alive Advantage

The performance benefit of `Keep-Alive` becomes especially clear when the server has processing delays. In Part 4, you'll use httpbin's `/delay/:n` endpoint which waits `n` seconds before responding. When making multiple requests:

- `client-cc` must: open socket → TCP handshake → send request → wait for delay → receive → close → **repeat entire process**
- `client-ka` must: open socket → TCP handshake **once** → send request → wait → receive → send request → wait → receive (socket stays open)

The TCP connection overhead (handshake, slow start, teardown) is small but adds up. With delays, this overhead becomes measurable and demonstrates why HTTP/1.1 introduced persistent connections.

---

## Part 3: Code Documentation (10 points)

Next venture into `http.c`. There are 3 functions in there that I want you to carefully study and document. They are `socket_connect()`, `get_http_header_len()` and `get_http_content_len()`. You don't need to change these functions, however, you need to clearly document them in a way that demonstrates you understand what they do. There is some basic pointer arithmetic and possibly some runtime library functions that you might not be familiar with. Research the library functions online, and follow the code, either by hand-running, or better yet, by stepping through the functions in the debugger after you finish part 2.

You will be graded on the quality of your documentation in that it demonstrates your understanding of the code. What do the variables do? How are they updated? and so on. Don't just comment that the pointer is updated.

---

## Part 4: Performance Timing Analysis

Do some research online to figure out how you can collect a timestamp from the operating system via C. Hint: check out the functions in `<time.h>`. Update both programs to get the start time of executing your program (first line in main), and the end time of executing your program (last lines just before main ends) to get the total runtime of your program. In other words (endTime - startTime).

Run the following tests **at least 3 times each** and record the results:
- `make run-cc3` (or manually: `client-cc httpbin.org 80 /delay/2 /delay/2 /delay/2`)
- `make run-ka3` (or manually: `client-ka httpbin.org 80 /delay/2 /delay/2 /delay/2`)

Create a file called `timing.txt` that contains:
- Your timing results for each run of both programs
- Average runtime for each version
- Analysis: Did `Keep-Alive` show improved response time? By how much?
- Explanation: Why did you see the performance difference you observed? Consider what happens with TCP connection establishment/teardown when making multiple requests with delays. How does the `/delay/2` endpoint help demonstrate the protocol difference more clearly than regular endpoints would?

The `/delay/:n` endpoints are specifically useful for this assignment because they introduce controlled, measurable delays that make the connection overhead more apparent. With instant responses, the Keep-Alive benefit might be too small to measure reliably. The delays amplify the difference, making it clear why persistent connections matter in real-world scenarios where servers have processing time.

---

## Program Interfaces

The `client-cc` and `client-ka` programs have the same command line parameters. They are as follows:

```
client-cc host port ...resource
           -- and --
client-ka host port ...resource

where:
  host is a domain name, e.g., httpbin.org
  port is the port number, e.g., 80
  resource is what you want from the host, e.g., /json

Also note that the interface can support getting multiple resources from the server, for example:

client-xx httpbin.org 80 / /json /html

will make 3 requests to the server:
  - one to get the root "http://httpbin.org/"
  - one to get json "http://httpbin.org/json"
  - one to get html "http://httpbin.org/html"

For testing delay behavior:
client-xx httpbin.org 80 /delay/2 /delay/2 /delay/2

will make 3 requests, each with a 2-second server-side delay

Note that you can have as many resources as you want, the above examples show 3.

Also, if you leave the command line arguments blank, defaults will be provided, see http.h for the default values.
```

---

## What You Need to Submit

All files must be placed in your GitHub repository under the `04-HTTP` directory. Submit the link to this directory in Canvas.

### Required Deliverables:

1. **client-cc.c** - Your implementation of the Connection: Close client with all TODO sections completed
2. **client-ka.c** - Your implementation of the Connection: Keep-Alive client with all TODO sections completed
3. **http.c** - The helper functions file with detailed documentation added to the three specified functions (`socket_connect()`, `get_http_header_len()`, `get_http_content_len()`)
4. **timing.txt** - Your timing analysis as described in Part 4
5. **internet-scale.md** - Your independent research on CDNs and internet-scale infrastructure (see separate handout)

### Compilation and Testing

You can use the makefile I provided to compile and test your code.

To compile just `client-cc` use:
```bash
make client-cc
```

To compile just `client-ka` use:
```bash
make client-ka
```

To compile both:
```bash
make
```

To run single requests:
```bash
make run-cc   # single request with client-cc
make run-ka   # single request with client-ka
```

To run three requests (for testing Keep-Alive):
```bash
make run-cc3  # three requests with client-cc
make run-ka3  # three requests with client-ka
```

---

## Grading Breakdown (70 points total)

### Coding Component (50 points)

**Part 1: client-cc.c Implementation (15 points)**
- 15 pts: Correctly implements all TODO sections, properly handles Connection: Close protocol
- 10 pts: Mostly correct but with minor issues
- 5 pts: Significant implementation problems
- 0 pts: Does not compile or missing major functionality

**Part 2: client-ka.c Implementation (25 points)**
- 25 pts: Correctly implements all TODO sections, properly handles Connection: Keep-Alive protocol including Content-Length parsing and connection reuse
- 20 pts: Mostly correct but with minor issues in Keep-Alive implementation
- 15 pts: Basic functionality works but significant issues with Keep-Alive or Content-Length handling
- 10 pts: Compiles but major functionality missing
- 0 pts: Does not compile or completely non-functional

**Part 3: Code Documentation in http.c (10 points)**
- 10 pts: Excellent documentation demonstrating deep understanding of all three functions, explains variables, pointer arithmetic, and logic flow clearly
- 7 pts: Good documentation but missing some details or clarity
- 4 pts: Basic documentation that shows surface-level understanding
- 0 pts: Missing or insufficient documentation

**Part 4: timing.txt is included in submission (already graded as part of other sections)**

### Independent Research Component (20 points)
Directions are in [internet-scale-directions](./internet-scale-directions.md)

See separate grading rubric document.

---

## Additional Notes

Unlike other assignments, the written and coding parts are combined. I provided a lot of the code for you so much of the written component will be you documenting code that I developed to demonstrate that you understand the code. If you just blindly document the code in the sections where I ask for it you will not receive any credit - your documentation should convey that you understand the code. Ideally, you should run the code through the debugger to get the best understanding.

The scaffold I provided for `client-cc` makes one call to a helper function in `http.c`, but you don't need to worry about that in Part 1. There are several helpers in `http.c` for `client-ka` - just assume they work, you don't need to change them, and their function names should clearly identify what they do. You should not have to write very much code, but expect to spend a good bit of time studying my code and stepping through it to increase your understanding.