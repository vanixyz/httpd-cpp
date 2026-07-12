#----- Stage1: build (poora GCC toolchain sirf yahan)
FROM gcc:13 AS build
WORKDIR /app
COPY server.cpp http_parser.hpp file_server.hpp thread_pool.hpp ./
RUN g++ -std=c++17 -O2 -pthread -static-libstdc++ -static-libgcc server.cpp -o server

#---- Stage 2: run (slim image - sirf binary + www) ---
#nanha sa OS
FROM debian:bookworm-slim 
WORKDIR /app
COPY --from=build /app/server .
COPY www ./www
EXPOSE 8081
CMD ["./server"] 