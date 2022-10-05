## Review of Socket Programming in C

### Overview

This repo includes a client program and 3 different server implementations.  The client sends a string to the server.  To run the client:

```
client "<PUT YOUR MESSAGE HERE - DONT FORGET THE QUOTES>"
```

If you don't provide an argument, the client defaults to sending the string `"Default Text"` to the server.

There are 3 different server implementations.  What they do is described below:

| Server Name | Description |
| :---        | :---        |
| `server`    | Basic echo server.  Echos what is sent from the client back to the server|
| `server2`   | Same as `server` but it shows chunking up receiving requests from client and keeps calling `recv` until all data is processed.  The client puts an EOF file marker at the end of the string.  Its ASCII character 5.
| `server3`   | Same as `server` but processes requests from clients in individual threads.  This increases the scale of the server. |

Note that this is very basic tutorial.  There is a lot of improvement that is still possible, especially with the multi-threaded server.  Specifically, using thread pools, and understanding how we can lock structures to coordinate across threads.  There remain a few possible (but rare) race conditions in my code, but my goal was to just demonstrate the bare minimum multi-threaded server.