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
- [✅ ] HTTP request parser (method, path, headers)
- [ ✅] Static file serving with MIME types
- [ ✅] Keep-alive connections
- [ ✅] Multithreading (thread pool)
- [✅ ] Benchmarks (wrk)
- [ ] Docker + live deployment

## Notes
See [NOTES.md](NOTES.md) for a raw log of things I learned and broke
along the way.

## Benchmarks

Load tested with [wrk](https://github.com/wg/wrk) — `wrk -t4 -c100 -d10s http://localhost:8081/`

| Configuration |  Requests/sec (median of 3 runs) | p50 latency | p99 latency |
|---|---|---|---|
| 1 worker | 14.8k | — | — |
| 8-worker thread pool | **69k** (peak: 106k) | 68µs | 775µs |

~5x throughput scaling from single worker to an 8-worker thread pool.

**Methodology notes:**
- Compiled with `-O2`, AddressSanitizer disabled, per-request logging disabled
- Tested on WSL2 (Ubuntu 24) — median of 3 runs reported due to VM
  scheduling variance; each run sanity-checked (total requests ÷ duration
  ≈ reported rate)
- Static file (~380 byte HTML) over keep-alive connections