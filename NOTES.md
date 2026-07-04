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