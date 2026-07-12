#include <iostream>
#include <string>
#include <cstring> //memset k lie
#include <unistd.h> //close() k lie
#include <sys/socket.h> //sokcet fucntions
#include <netinet/in.h> //sockaddr_in struct
#include "http_parser.hpp"
#include "file_server.hpp"
#include <thread>
#include "thread_pool.hpp"
#include <cstdlib> //get env
//Poori request aane tak padho (headers ka end =\r\n\r\n)
//true = request mili , false=client gaya/timeout/ bahut badi request
bool read_request(int fd , std::string& raw){
    char buf[4096];
    raw.clear();
    while(raw.find("\r\n\r\n")==std::string::npos){   
        int n = read(fd,buf,sizeof(buf));
        if(n<=0) return false; //client gaya ya timeout
        raw.append(buf , n);
        if(raw.size() > 65536) return false; //64kb+ headers? kachra/attack
    }
    return true;
}
// Poora data likhne ki guarantee — partial writes handle karta hai
// (write bhi read ki tarah "jitna abhi ho saka" wala hai)
bool write_all(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = write(fd, data.data() + sent, data.size() - sent);
        if (n <= 0) return false;    // client gaya / error
        sent += n;                    // jitna gaya, aage se continue
    }
    return true;
}
void handle_client(int client_fd){
    timeval tv;
    tv.tv_sec=5;
    tv.tv_usec=0;
    setsockopt(client_fd,SOL_SOCKET,SO_RCVTIMEO , &tv , sizeof(tv));

    bool keep_alive=true;

    while(keep_alive){
        std::string raw ;
            if(!read_request(client_fd , raw)) break; //gaya/timeout ->bas

            HttpRequest req = parse_request(raw);
             if(!req.valid){
        std:: string body =  "<h1>400 BAD REQUEST</h1>";
        std:: string response=
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std:: to_string(body.size()) +"\r\n"
        "\r\n" + body;
        write_all(client_fd, response);
        break;
  
     }
         std::cout<<"Method"<< req.method 
        << "| Path: "<<req.path 
        <<" | Headers: " << req.headers.size() << "\n";

        //Client kya chahta hai rakhein ya kaatien ?
        //HTTP/1.1 ka default: keep-alive , jab tak "close" na bole
        std:: string conn;
        if(req.headers.count("Connection")) conn = req.headers["Connection"];
        keep_alive = (req.version == "HTTP/1.1")&& (conn != "close");

        std::string conn_header = keep_alive?"keep-alive": "close";
        //path se file tak
       std::string path = req.path;
       if(path=="/") path = "/index.html"; //ghar ka default page

//SECURITY: ".." wale raste block - warna koi
// /../../etc/passwd mang ke system files le jayega
if(path.find("..")!= std::string::npos){
    std::string body = "<h1>403 Forbidden </h1>";
    std::string response=
    "HTTP/1.1 403 Forbidden\r\n"
     "Connection: " + conn_header + "\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: " + std::to_string(body.size()) +"\r\n"
    "\r\n"+body;
    write_all(client_fd, response);
    break;
}
std::string content;
if(!read_file("www" + path, content)){
    std::string body ="<h1>404 Not Found</h1>";
    std::string response=
    "HTTP/1.1 404 Not Found\r\n"
     "Connection: " + conn_header + "\r\n"
    "Content-Type: text/html\r\n"
    "Content-Length: "+ std::to_string(body.size())+"\r\n"
    "\r\n"+body;
    write_all(client_fd, response);
    break;
}

//file mili : shi type ke saath bhejo

std::string response=
"HTTP/1.1 200 OK\r\n"
 "Connection: " + conn_header + "\r\n"
"Content-Type: " + get_mime_type(path) +"\r\n"
"Content-Length: "+ std::to_string(content.size()) + "\r\n"
"\r\n"+ content;
 write_all(client_fd, response);
    }
    close(client_fd);
}
int main(){
    //1. sokcet banao ye ek "phone" hai jo abhi kisi no. se juda nhi
    // AF-INET= IPv4 , SOCK_STREAM = TCP
    int server_fd= socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        std::cerr<<"socket() fail ho gaya\n";
        return 1;
    }
    //2. ye line address already in use vale error se bachayegi
    //server dubara chalo to purana vla port turant mil jaye
    int opt=1;
    setsockopt(server_fd , SOL_SOCKET , SO_REUSEADDR , &opt , sizeof(opt));

    //3 address setup "main port 8080 pe rahunga"
    sockaddr_in addr;
    std::memset(&addr , 0 , sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=INADDR_ANY; //ismachine k ekisi bhi IP epe
    //addr.sin_port=htons(8081); //htons = port ko network fromat me
    int port = 8081;
    if(const char* p= std::getenv("PORT"))
        port = std::atoi(p);
       addr.sin_port= htons(port);
    //4. bind = phonei ko number do
    if(bind(server_fd , (sockaddr*)&addr,sizeof(addr))<0){
        std::cerr<<"bind()) fail-port busy hai kya?\n";
        return 1;
    }

    //5 listen=ringer on karo , calls aa sakti hai ab
    listen(server_fd,10); //10=waiting line ki length
    std::cout<<"server chal raha hai: http://localhost:"<<port<<std::endl;;
    
     ThreadPool pool(8); //8 permanent waiters
    //6 hamehsa k lie loop har call utha jawab de , kaat de
    while(true){
      int client_fd = accept(server_fd, nullptr , nullptr);
      if(client_fd<0) continue;
      pool.submit(client_fd);
     
}
  close(server_fd);
  return 0;
}   //main ka end