#pragma once  //ye file do baar include na ho iska guard
#include <string>
#include <unordered_map>
#include <sstream> //istringstream ke lie 

//ek req ki saari structured info ek jgh
struct HttpRequest{
    std::string method; //"GET"
    std::string path;   //"/about.html"
    std::string version; //HTTP/.1.1
    std::unordered_map<std::string , std::string> headers;
    bool valid=false; //parsing shi hui ya nahi 
};

//Raw string-> structured HttpRequest
inline HttpRequest parse_request(const std::string& raw){
    HttpRequest  req;

    //-------step1 request line nikalo (pehli \r\n tak)---
    size_t line_end = raw.find("\r\n");
    if(line_end==std::string::npos) return req; //\r\n hi nahi==kachra
    std::string request_line = raw.substr(0,line_end);

    //---Step2: request line ko 3 hisso me todo ----
    // istringstream spaces pe khud tod deta hai - manual find/substr
    //se jyada saaf tarika 3 tokens k lie 
    std::istringstream rl(request_line);
    rl>> req.method >> req.path >> req.version;
    if(req.method.empty() || req.path.empty()|| req.version.empty())
    return req; //teeno nahi mile = malformed request

    //---Step3 : header parse karo , line by line
    size_t pos=line_end+2; //+2 = \r\n ke paar \r\n ki total length 2 bytes hoti h
    while(true){
        size_t next = raw.find("\r\n",pos); //dhundhra hai ki\r\n kis index pr h strting from pos
        if(next==std::string::npos) break; //age \r\n nahi
        if(next==pos) break; //khali line mili headrers khatam 

        std::string line = raw.substr(pos,next-pos);

        //"Host: localhost:8081-> naam ":" se pehle , value baad me"
        size_t colon = line.find(':');
        if(colon!=std::string::npos){
            std::string name = line.substr(0,colon);
            std::string value = line.substr(colon+1);

            //value ke aage ki space htao ("_localhost" -> "localhost")
            size_t start = value.find_first_not_of(" \t");
            if(start!=std::string::npos) value = value.substr(start);
            else value="";
            req.headers[name]=value;
        }
        pos=next+2; //agli line pe
    }
    req.valid=true;
    return req;
}