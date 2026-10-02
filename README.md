# Netchat

A multi-client terminal chat server and client written in C++23 on top of raw POSIX sockets.

Clients connect over TCP, pick a name, and every message is broadcast to all connected users with the sender's name and a timestamp.

## Highlights
- The server is fast and multithreaded. Each client gets its own thread, and a separate broadcaster thread drains a thread-safe message queue.
- The code uses modern C++ with RAII and type safety. Sockets and `addrinfo` are wrapped in RAII types, and errors are returned with `std::expected` and strong typedefs.
- The program is compiled with strict warnings and warnings as errors. Flags include `-Wall -Wextra -Wpedantic -Werror -Wconversion` plus Address Sanitizer and Undefined Behavior Sanitizer.

## Build & Run
```
cmake -B build
cmake --build build

./build/netchat-server [port]       # default port 4286
./build/netchat [host] [port]       # default 127.0.0.1 4286
```
