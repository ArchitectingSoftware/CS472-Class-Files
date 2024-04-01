## Homework 2 - 
## Build a CS Class Lookup Client and Server

### Directions and info for this assignment
I stubbed out a lot of this code, so you can thank me later :-).

Basically the objective is to create a client and server that exchange information over a made-up protocol that is simple, but mimics a lot of what you would see working with a real network protocol.

The primary objective of this "protocol" is for the client to send a request to the server with a well-formed packet, requesting information about a class.  The server responds with this information.  If the client does not provide any command line args, the default is for the client to request information about CS472.  The client can also pick another class via the `-c` option. where an alternative class can be requested.  For example `-c cs577`.

The server also has another trick up its sleeve, it can act as a ping server.  This is very useful in real life to support something called health-checks.  We will discuss this later in class.  For now, the only other thing to understand is that the client can pass an argument `[-p "Any message you want"]` and the server will echo this message back to the client.

### (75% - Programming Component) What to do and hand in
Since students in this class seem to be in varying level of familiarity and comfort with systems level programming in C, I decided to provide a fairly robust scaffold with a bunch of comments and TODO instructions to get you started.  I extracted this code from my working implementation.  Please note that you do not have to use this scaffold, just implement the protocol as described below and in the source code documentation.  In fact, I would prefer you craft your own solution and only look to the code that I provided for help if you get stuck.  You will learn a lot more that way. I did some quick and dirty things but that does not mean that you have to do the same things that I did.  I also provided a simple makefile to compile and build the code, feel free to use that. 

What to hand in?  Of course your source code and any specific build instructions.  Also include some sample output in a readme file, like im doing here (see below)

### The protocol and header
Please understand the protocol defined in `cs472-proto.h`.  This file contains a number of constants and describes the protocol structure in detail. Note that the header has a placeholder for a `CMD` option. This will indicate if the client is requesting a course lookup or a response to a ping. 

### The Client
The client application opens up a socket that connects to the server.  I hard coded the server address at 127.0.0.1 or localhost.  You can change this if you want.  Make sure you understand the stubbed out client code well. The client accepts 2 command line parameters. Basically a `[-p "ANY MESSAGE YOU WANT"]` parameter to indicate that you want to ping the server and have it echo the request, or a `[-c COURSE_ID]` parameter to indicate you want the server to look up the course and provide information back.

### The Server
The server responds to requests from the client.  It binds on 0.0.0.0 - aka all local interfaces.  This should work well if you are running locally, you might have to adjust to run on tux.  The header `cs472-proto.h` defines a default port number - 1080.  This again might require modification on tux, but should work fine locally.  

The server runs in a loop processing client requests. If a request for a class lookup is sent, the server responds with a string about that class.  If a ping request is made, the server echos what was sent in the response.

### Sample Output
The following is some sample output from my implementation. You don't need to mirror it exactly, it just shows you what you should be displaying, and how things should be handled.

```
➜  hw2-echo ./client -p "Hello there server, how are you?"
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_PING_PONG
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        NONE
  Pkt Len:       29

RECV FROM SERVER -> PONG: Hello there serv
➜  hw2-echo ./client                                      
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        CS472
  Pkt Len:       12

RECV FROM SERVER -> CS472: Welcome to computer networks
➜  hw2-echo ./client -c cs577
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        cs577
  Pkt Len:       12

RECV FROM SERVER -> CS577: Software architecture is important
➜  hw2-echo ./client -c bad  
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        bad
  Pkt Len:       12

RECV FROM SERVER -> Requested Course Not Found
```

### (25% Question Component) Written Assignment Questions

The programming component of this assignment is worth 75% of your grade.  The written component is worth 25%.  When you hand in your source code, please provide a pdf or word document that addresses the following: 

I am not sure how many of you have even herd of Vint Cerf.  Vint is a touring award winner, and is the co-inventor of both TCP,TCP/IP, ARPANET, and is generally considered the father of the Internet.  Vint is a gift to our industry, still very active at almost 80 years old.  I had the opportunity to sit down with him in a 1:1 meeting to help give him background for a conference I organized where he was a featured keynote, and it was certainly a highlight of my career. 

For this assignment I want you to watch the YouTube video linked below.  This video is from a Keynote talk that Vint did at a conference in 2022 where he reviewed the history of the Internet, and some of his contributions and observations.
YouTube Link:  https://www.youtube.com/watch?v=lWyv4wU-WNA

After watching this video, please do a short writeup addressing the following prompts:

1. If you had the opportunity to ask Vint one thing what would that be?
2. What is one thing of interest you took away from this video?
