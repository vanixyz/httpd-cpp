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
## Day 4 — keep-alive, read-loop, timeout

### Read-loop
- read() poori request ki guarantee nahi deta — TCP byte-stream hai,
  message boundaries nahi hote. Localhost pe poora milta dikhta hai =
  jhootha comfort; asli network pe partial reads normal.
- Fix: read_request() — \r\n\r\n milne tak loop me append. memset ki
  zaroorat khatam (append exact n bytes leta hai).
- 64KB guard: koi endless bytes bheje bina \r\n\r\n ke to memory na
  khaye — resource-exhaustion defense (security feature #2).

### Keep-alive
- Pehle: har request ke baad close = har file ka apna TCP handshake
  (ek page load = 3 connections). Ab: EK connection, kai requests.
- Structure: inner while(keep_alive) = ek connection ki baat-cheet,
  outer while(true) = naye clients. close() ab sirf connection ke
  end pe — request ke end pe NAHI.
- HTTP/1.1 default = keep-alive; client "Connection: close" bole to
  us request ke baad kaato. (Day 2 ka " \t" space-trim fix yahan
  kaam aaya — warna " close" != "close" hamesha true rehta.)
- Har response me "Connection: keep-alive/close" header batate hain.
- Status line HAMESHA pehli line — Connection header uske neeche.
- Proof: curl do URLs pe "Re-using existing connection" bolta hai.

### fd-leak bug (khud ka)
- close(client_fd) galti se outer loop ke BAHAR chala gaya tha =
  kabhi chalta hi nahi = har client ek fd leak = ~1024 clients me
  server naye connections lena band kar deta. Nested loops me
  close ki jagah = audit point. break inner loop todta hai, close
  uske turant baad (outer ke andar).

### Timeout (SO_RCVTIMEO)
- accept ke baad socket pe 5-sec timer: read me 5 sec kuch nahi
  aaya to -1 → break → close. Idle client (Chrome preconnect) ab
  server ko max 5 sec bandhak bana sakta hai — pehle hamesha ke liye.
- But single thread me problem chhoti hui, khatam nahi: har idle
  connection = 5-sec ki tax line me lagne walon ke liye (browser
  refresh 5-sec lag raha tha isi wajah se). Asli ilaaj = threads.

### Timeout-testing ka drama (best debugging story so far)
- nc test "fail" dikha — prompt kabhi wapas nahi aaya. Shak server pe.
- ss -tnp ne sach khola: server FIN-WAIT-2 me ("maine FIN bhej diya,
  tumhare FIN ka wait hai") = server ne 5 sec pe SAHI close kiya tha.
  nc CLOSE-WAIT me ("FIN mil gaya, mujhe close karna chahiye... but
  nahi kiya") = doshi nc tha.
- TCP close DO-TARFA hota hai — dono sides apna FIN bhejti hain.
  Ek side band = aadha-band connection (half-close), states me dikhta hai.
- nc kyun atka: uska stdin (keyboard) khula tha, WSL ke nc ka -q flag
  ne bhi wada nahi nibhaya. Tool ka quirk, code ki galti nahi.
- Debug prints ka pattern: ek "yahan pahuncha?", ek "kya mila?" —
  read -1 exactly 5 sec pe = timeout ka ECG.
- Final certificate: python socket one-liner + time = real 5.171s.
- LESSON: test fail ho to 3 suspects — code, test, tool. Faisla
  independent evidence se (ss + debug prints, dono ek hi baat bole).

  ### Kya se kya hua
Pehle: har request pe naya connection, ek read call, idle client
hamesha ka bandhak. Ab: ek connection pe kai requests (keep-alive),
poori request aane tak read-loop, 5-sec timeout ka bouncer. Bacha:
ek waqt me ek hi client (single thread) — Day 5 ka kaam.

## Day 5 — threads (thread-per-connection)
- Problem (4 avatars me jheli): single thread = ek idle client sabko
  rokta tha. Ilaaj: har client apna thread.
- Refactor: accept ke baad ka SAB (timeout→loop→close) handle_client()
  me; main sirf accept + thread bana ke aage.
- std::thread t(handle_client, client_fd); t.detach() = fire-and-forget:
  thread apna kaam karke khud khatam. join() = wait karna (wo yahan
  wapas single-threaded bana deta).
- close(client_fd) ab thread ke andar — har thread apna phone khud rakhta hai.
- -pthread compile flag chahiye threads ke liye (build alias updated).
- C++ top-to-bottom padhta hai: function use se pehle declared ho
  (order theek kiya; forward declaration = headers ka core idea).
- Test: nc atka + browser INSTANT (pehle 5-sec tax). 5 parallel curls,
  5 threads, sab served.
- Race condition abhi nahi dikhi prints me — but shared cout pe threads
  bina lock ke likh rahe hain = wo hai, bas sharmila hai. "Kabhi dikhe
  kabhi nahi" = race ki definition. Mutex Day 6 me.
- Agla sawal khud se: 10K clients = 10K threads? Nahi — thread creation
  mehenga, unlimited threads = memory blast. Ilaaj: THREAD POOL.

### Kya se kya hua
Pehle: ek waiter, ek table, baaki line me. Ab: har table ka apna waiter,
koi kisi ko nahi rokta. Problem: waiter unlimited nahi ho sakte —
Day 6 me fixed staff + order queue (thread pool).


-  file me order matters (ya forward-declare karo), class ke andar order sirf insaano ke liye hai, compiler ke liye nahi. NOTES.md entry ban gayi free me 
-## Day 6 — thread pool
- Thread-per-connection ki dikkat: har client pe naya thread (~8MB
  stack, creation cost) — 10K clients = maut. Ilaaj: 8 fixed workers
  + ek shared queue = producer-consumer pattern.
- Mutex = taala: queue chhoone se pehle lo, kaam karo, chhodo.
  lock_guard/unique_lock = RAII — scope khatam, taala khud chhoota.
- Critical section CHHOTA rakho: lock sirf push/pop jitna; asli kaam
  (handle_client) lock ke BAHAR — warna 8 workers ek taale pe atke =
  wapas serial.
- condition_variable = ghanti: cv.wait = "kaam aaye to jagana" (CPU
  free, busy-wait nahi); wait sote waqt lock chhodta hai, uthte waqt
  wapas leta hai. notify_one = ek worker jagao.
- wait me condition/while kyun: (1) doosra worker kaam utha chuka ho
  sakta hai, (2) spurious wakeups — OS bina ghanti ke bhi jaga deta hai.
- Queue<int> rakhi (sirf fd) — jobs homogeneous hain; generic banana ho
  to std::function<void()> + lambda-capture (overhead ke saath).
  Deliberate simplicity bhi design decision hai.
- Class ke andar member order compiler ke liye matter nahi karta
  (complete-class context) — convention: public interface upar,
  private plumbing neeche. File-level pe order matters (ya forward
  declaration — jo headers ka core idea hai).
- Shutdown nahi banaya (server hamesha chalta hai) — clean shutdown =
  stop flag + notify_all + join. Interview future-work answer.
- Proof: /proc/<pid>/task = thread count — 9 fixed, load pe bhi.

### Kya se kya hua
Kal: har customer pe naya waiter hire (unbounded). Aaj: 8 permanent
waiters + order counter. Memory bounded, threads reuse. Bacha:
benchmarks — kitna tez hai ye sab? (Day 7)

-"Do threads EXACT same moment pe lock maang sakte hain (multi-core = true parallel). Tie hota hi nahi: mutex ki neev hardware atomic instruction (CAS/test-and-set) hai — cache-coherence ek ko jitata hai, doosra kernel ki wait-list me. Saara concurrency stack is ek hardware vaade pe khada hai."
-(Interview word: minimize the critical section / reduce lock contention.)
 (real systems kya karte hain — interview me bolne layak): teen hathiyar: (1) bounded queue — queue ki max size fix karo (jaise 1000); bhar gayi to naye client ko turant 503 Service Unavailable bol do — "der se haan" se "turant na" behtar hai (fail fast); (2) workers scale karo — 8 ki jagah CPU cores ke hisaab se (hardware_concurrency), ya load pe badhao; (3) timeouts + monitoring — queue depth naapte raho, alarm bajao. Hamare server me abhi unbounded queue hai — ye jaan-boojh ke simplification hai, aur "future work" ka perfect answer: "I'd add a bounded queue with 503 rejection to apply backpressure."
 -Ye discipline khud interview-material hai: "I ran multiple trials and took the median because single benchmark runs on a VM are noisy" — ye ek line tumhe "number ratta candidate" se "measurement samajhne wala engineer" bana deti hai. 📊

 -Day 7: buildfast (-O2, no ASan) vs build; logging ki keemat; write_all (partial writes — read ka judwa); wrk ka WSL clock bug (307 arab minute 😄) + sanity-check habit; multiple runs + median (variance ka sach); 1→8 workers = ~5x

 