## Programming Assignment 1

#### Directions
The purpose of this assignment is to dust off some of your C skills and
get exposure to the types of things you do in network programming working
with raw buffers and manipulating them using PDUs and Packet structures.
You will also get the chance to play with wireshark to create your own 
test cases, but before you do this, you should implement this code with
the samples that I already provided.

I have provided a makefile for you.  `make build` compiles your program and
creates an executable called `decoder`.  Running `make run` will run your
program.

See the homework supplement I provided on blackboard for some more information
and background.

#### What to Hand In
This assignment only requires you to modify code in `decoder.c`.  All of the
network structures are in `packet.h` make sure you understand them.  To make
things easier for you to convert buffered data to formatted strings for things
like IP and MAC addresses, I have provided helpers in `nethelper.c`.  Again, 
please only modify `decoder.c`.

When you are ready to submit, you can create a zip file of your entire
directory and submit on blackboard.  AKA in other words everything required so
that the grader only has to execute `make build && make run`. to compile, link, 
and execute your program.  Alternatively, you can create a git repo on GitHub
with a public repository and provide a link to this repo as your blackboard
submission.  If you dont want this code public, but want to use a git environment
you can also push to a private Drexel CCI GitLab repo and provide that link
as your blackboard submission.

#### Warnings, dont let this be you...
This is a **REALY EASY** assignment if you are comfortable with C.  My solution is 
about 40 lines of code for **ALL** of the things you need to accomplish. If you need
to refresh your C knowledge, PLEASE DONT WAIT UNTIL THE LAST MINUTE. This assignment
is setting the foundation for typical network programming tasks which involve
creating and manipulating structures, working with buffers, pointers, etc.  

If you find yourself struggling, you are not stupid.  You just need a refresher on systems programming in C.  I will be doing some office hours to help refresh your memory on C programming during the first two weeks - no need to attend if you are comfortable with these things, but please make it a priority to attend if you dont
understand what is going on. 

I also put a **TON** of documentation in the code to help you.  To make things
easier on the grader, please thin out the documentation in your submission, removing
mine and putting in documentation relevant to your specific implementation.