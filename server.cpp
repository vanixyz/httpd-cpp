#include <iostream>
#include <string>
#include <cstring> //memset k lie
#include <unistd.h> //close() k lie
#include <sys/socket.h> //sokcet fucntions
#include <netinet/in.h> //sockaddr_in struct

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
    addr.sin_port=htons(8081); //htons = port ko network fromat me

    //4. bind = phonei ko number do
    if(bind(server_fd , (sockaddr*)&addr,sizeof(addr))<0){
        std::cerr<<"bind()) fail-port busy hai kya?\n";
        return 1;
    }

    //5 listen=ringer on karo , calls aa sakti hai ab
    listen(server_fd,10); //10=waiting line ki length
    std::cout<<"server chal raha hai: http://localhost:8081\n";

    //6 hamehsa k lie loop har call utha jawab de , kaat de
    while(true){
        //accept-incoming call uthao ; ye naya fd deta hai
        //sirf IS client se baat krne k lie
        int client_fd = accept(server_fd , nullptr , nullptr);
        if(client_fd<0) continue;

        //clinet ne kya bola(browser ki req) padho aur print karo
        char buffer[4096];
        std::memset(buffer , 0 , sizeof(buffer));
        read(client_fd, buffer , sizeof(buffer)-1);
        std::cout<<"-----Requesr aayi------\n"<<buffer<<"\n";

        //HTTP jawab - format fixed hai : status line , headers,
        //Khali line (\r\n\r\n), phir body
        std::string body = "<h1>Hello from my server!</h1>";
        std::string response=
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size())+"\r\n"
        "\r\n" + body;
        
        write(client_fd , response.c_str(), response.size());
        close(client_fd); //is client se batt khtm
    }
  close(server_fd);
  return 0;
}