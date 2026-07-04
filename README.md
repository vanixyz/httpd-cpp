# httpd-cpp

An HTTP/1.1 server written from scratch in C++17 — no frameworks, no
libraries, just POSIX sockets.

**Status:** Work in progress (Day 1)

## Current features
- TCP server using POSIX sockets (socket/bind/listen/accept)
- Serves a basic HTTP/1.1 response with correct headers
- SO_REUSEADDR for clean restarts

## Build & run
```bash
g++ -std=c++17 -Wall -Wextra -fsanitize=address -g server.cpp -o server
./server
# open http://localhost:8081
```

## Roadmap
- [ ] HTTP request parser (method, path, headers)
- [ ] Static file serving with MIME types
- [ ] Keep-alive connections
- [ ] Multithreading (thread pool)
- [ ] Benchmarks (wrk)
- [ ] Docker + live deployment

## Notes
See [NOTES.md](NOTES.md) for a raw log of things I learned and broke
along the way.