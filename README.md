echo server
===========

Simple echo server module for the linux kernel. Part of my master's thesis, to
better understand the working of the kernel and explore the ways of how to
develop a kernel module.

Current functionality
---------------------

- TCP listen socket created on module load
- Clients can connect, connection will auto-close
- Kernel logs will show if connect was succesfull
- Uses blocking sockets, MUST change asap
