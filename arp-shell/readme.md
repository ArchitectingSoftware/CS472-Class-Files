# Assignment 1 Shell for CS472

See directions from the class blackboard site.

1. This is a shell, you can modify it how you see fit, including a total refactor
2. There are 3 example binary arrays, each one is given in both byte array and word array format. 
3. The ARP header structure is provided for you in the `arp-header.h` header file.  You should not need to modify this file.

### What you need to do

For each of the 3 examples:

1. Read in the byte array
2. Load it into the `arp_ether_ipv4` type that is described in the `arp-header.h` file.
3. Print out a formatted string showing the contents.
4. Repeat steps 1,2,3 with the word array format provided.

While it should be obvious. The first example are the arrays `ex1b` which is the byte version of example 1, and `ex1w` is the word example of example 1.  The same conventions are followed for example 2 and example 3.

In order to help you out, the proper value for Example 1 is:

```
ARP PACKET DETAILS 
     htype:     0x0001 
     ptype:     0x0800 
     hlen:      6  
     plen:      4 
     op:        1 
     spa:       192.168.1.51 
     sha:       01:02:03:04:05:06 
     tpa:       192.168.1.1 
     tha:       aa:bb:cc:dd:ee:ff 
```

Your objective is to figure out the ARP values for Example 2 and Example 3, and to submit your code and these answers. 

This is a simple program, no make file is required, you need to submit compile instructions if you go beyond needing to do `gcc decoder.c -o decoder`
