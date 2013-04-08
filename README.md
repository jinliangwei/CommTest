CommTest
========

Communication Module Test

MPI based communication module is implemented in Scheduler.cpp,
Scheduler.hpp. The main purpose of test is to see how many MPI
messages scheduler-server and client can exchange with varying size of
message. By default, it use a test message with 8K size. 
To change the size of message, change MSGSIZE macro in scheduler.hpp.  

Relevant functions: sched_server(), sched_client() and other low level
functions that encapsulate and decapsulate packets.