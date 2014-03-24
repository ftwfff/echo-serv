echo server
===========

Simple echo server and client module for the linux kernel. Part of my master's
thesis, to better understand the working of the kernel and explore the ways of
how to develop a kernel module.


Current functionality
---------------------

- TCP connections
- Server listens on port 2325, accepts one connection at a time
- Server will return all data it receives
- Client connects to server and periodically sends message and waits for reply
- Uses blocking sockets, MUST change asap
