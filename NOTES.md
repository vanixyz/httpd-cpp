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
- " \t" missing space in find_first_not_of
- #include = copy-paste, inline/ODR, hpp vs cpp
- fd = kernel table ka index/ID, 0/1/2 reserved
- multi-line string me beech ka semicolon = statement cut, lines discard
kachre pe 200 OK ja raha tha → req.valid check + 400 handler
netcat blocked: kernel ka connect (backlog) vs process ka accept — idle browser preconnect ne single-threaded server ko block kiya
- Content-Length = body ka byte-count ka vaada; galat ho to client atakta hai
- server terminal = mere cout, netcat = raw bytes, browser = rendered
- "accept() < 0 = us ek client ke liye fail (fd khatam / client bhaag gaya / signal). Response: continue — per-client error server ko nahi girata. Fatal (socket/bind) pe exit, transient (accept/read) pe skip — error severity ka fark."
- fd limit: soft ~1024 default (ulimit -n), hard limit lakhs me. Khatam hone pe EMFILE → accept fail. fd leak + limit = server ghanton baad naye connections refuse karta hai. select() ka 1024 cap → C10K problem → epoll ka janm."
- : "Single-threaded me fd limit concurrency se nahi, LEAK se hit hoti hai — leak cumulative hai (har missed close = ek fd hamesha ke liye gaya), 1024 requests kaafi hain marne ke liye. Backlog kernel queue hai, process fd nahi khata. Keep-alive = duration ka game, threads = volume ka game."
-  "read() = 'is call me jo mila' — poori request ki guarantee NAHI (TCP stream hai, message boundaries nahi hote). Localhost pe hamesha poora milta dikhta hai = jhootha comfort. Fix: delimiter (\r\n\r\n) tak read-loop; body ke liye Content-Length tak. Buffer-size limit bhi isi se khatam hoti hai."
- #pragma once lagate hain taaki ek single file me code repeat na ho (Compile-time protection).inline lagate hain taaki alag-alag files ke beech me code takraye nahi (Link-time protection).
- "CSS download hui render ki jagah = server octet-stream bhej raha tha = MIME detection fail. Chrome galat-MIME stylesheet ko silently reject karta hai — koi error nahi, bas rang nahi. Debug ka rasta: file ko seedha URL se kholo, browser ka behavior hi MIME bata deta hai." — ye debugging story interview-grade hai, "how did you test MIME handling" ka jeeta-jaagta jawab 😄
-- MIME bug: map me ".css" ki jagah ",css" likh diya (dot→comma typo).
  Koi compile error nahi, koi crash nahi — bas lookup miss → default
  octet-stream → Chrome ne CSS silently reject ki / download karayi.
  Lesson: logic bugs chupke se galat hote hain; data (keys, strings,
  configs) ko bhi code jitni dhyan se proofread karo.
  Debug chain: rang nahi → quote-test se HTML fresh confirm → direct
  URL → download hua → octet-stream → MIME detection → hpp me typo.
  - ## Day 3 section banao — aaj ki entries (short me likh do, apne words me):

sizeof(string) ≠ .size() — sizeof = object ka size (~32 bytes), .size() = content ki lambai
curl paths normalize karta hai — .. server tak pahuncha hi nahi; asli security test raw bytes se (printf + nc). Client-side safai pe bharosa nahi.
MIME comma-bug ki poori detective story (pichhle message me draft de diya tha, wahi use kar lo)
Chrome galat-MIME CSS ko silently reject karta hai; direct URL kholna = MIME check ka shortcut (download hua = octet-stream)
Commented code ne brackets kha liye the — purana code delete karo, git yaad rakhta hai
Content badla to sirf refresh, code badla to build + restart
- ## Day 4 — keep-alive, read-loop, timeout
- read() poori request ki guarantee nahi deta (TCP stream) → read_request()
  loop: \r\n\r\n milne tak append; 64KB guard (endless-bytes attack se).
- Keep-alive: close ab REQUEST ke end pe nahi, CONNECTION ke end pe —
  inner loop = ek connection ki saari requests, outer = naye clients.
  HTTP/1.1 default keep-alive, "Connection: close" pe kaato.
- Status line HAMESHA response ki pehli line — Connection header neeche.
- fd-leak bug pakda: close(client_fd) outer loop ke BAHAR chala gaya tha
  (kabhi chalta hi nahi). Nested loops me close ki jagah = audit point.
- SO_RCVTIMEO: 5 sec idle → read -1 → break → close. Idle client ab
  poore server ko sirf 5 sec bandhak bana sakta hai (pehle hamesha).
- Timeout "fail" ka drama: server ne sahi close kiya tha (FIN-WAIT-2),
  nc apne khule stdin ki wajah se CLOSE-WAIT me latka tha. ss -tnp ne
  sach dikhaya. Lesson: test fail ho to pehle TEST ko check karo.
  nc -q 1 = EOF ke baad exit. Naye TCP states seekhe: FIN-WAIT-2, CLOSE-WAIT.
- curl "Re-using existing connection" = keep-alive ka litmus test.
- Debug print (read returned: n) daala tha — use HATANA hai (Step 5 me).
- aaj ka epilogue: "nc WSL pe server-close ke baad bhi exit nahi karta — debug prints ne saboot diya: read -1 exactly 5 sec pe. Do independent evidence (ss states + debug print) se server begunah sabit. Lesson: tool pe shak karne se pehle evidence lo, evidence mile to tool badlo — python socket one-liner cleaner test nikla."