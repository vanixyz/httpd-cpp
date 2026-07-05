# Learning Notes — httpd-cpp

Raw log of what I learned and broke while building this. For future me
(and interview prep).

## Day 1 — first working server

- socket() returns -1 on failure, positive fd on success. Maine condition
  ulti likh di thi (`> 0` pe error return), to server successfully socket
  bana ke khud hi exit ho raha tha. Lesson: return value ka direction
  docs se confirm karo, guess mat karo.
- `return 1` (program ka exit code, non-zero = error) aur socket() ka -1
  (function ka failure signal) do alag cheezein hain — pehle dono ko mix
  kar rahi thi.
- Ek port pe ek hi process listen kar sakta hai. Mera port 8080 pe Jenkins
  already baitha tha, isliye 8081 use kiya. bind() fail hone ka common
  matlab yahi hota hai.
- SO_REUSEADDR: server restart karne pe "Address already in use" se
  bachata hai (purana socket TIME_WAIT me hota hai).
- HTTP response ka format strict hai: status line, headers, blank line
  (\r\n\r\n), phir body. Status me "OK" letter O hai, zero nahi — aur
  header names ki spelling exact honi chahiye (Content-Length), warna
  browser
 - "using namespace std headers me kabhi nahi — namespace pollution + C headers ke saath name collisions. CP habit hai, production habit nahi."
 - "fd = kernel ki per-process table ka index (ID). Isi liye read/write/close files aur sockets dono pe same chalte hain. 0/1/2 = stdin/stdout/stderr, isliye mera server_fd 3 hota hai. close() na karo to fd leak."
 -  "TIME_WAIT per-connection hota hai (4-tuple ka), per-port nahi — chalte server pe naye clients unaffected rehte hain."
 - entry (ye wali badi kaam ki hai): "Browser tab band karne se server band nahi hota — server independent process hai, accept() pe wait karna hi uska normal state hai. Aur: single-threaded server ko ek idle/slow client block kar sakta hai (Chrome preconnect se live dekha) — yahi multithreading + timeouts ka motivation hai."
 -bytes>=0 ulta condition + "didn't send any data" symptom
" \t" missing space in find_first_not_of
#include = copy-paste, inline/ODR, hpp vs cpp
fd = kernel table ka index/ID, 0/1/2 reserved
multi-line string me beech ka semicolon = statement cut, lines discard
kachre pe 200 OK ja raha tha → req.valid check + 400 handler
netcat blocked: kernel ka connect (backlog) vs process ka accept — idle browser preconnect ne single-threaded server ko block kiya
Content-Length = body ka byte-count ka vaada; galat ho to client atakta hai
server terminal = mere cout, netcat = raw bytes, browser = rendered