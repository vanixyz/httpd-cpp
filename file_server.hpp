#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>

//path ke extension se Content-type btao
inline std::string get_mime_type(const std::string& path){

    //extension nikallo : aakhri '.' ke baad ka hissa
    size_t dot = path.rfind('.');
    if(dot== std::string::npos) return "application/octet-stream";
    std::string ext = path.substr(dot);

    static const std::unordered_map<std::string , std::string> types={
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".png" , "image/png"},
        {".jpg" , "image/jpeg"},
        {".gif" , "image/gif"},
        {".txt" , "text/plain"},
        {".ico" , "image/x-icon"},

    };
    auto it = types.find(ext);
    if(it!=types.end()) return it->second;
    return "application/octet-stream"; //unknown =generic types 
}

//File padho ; mili to content 'out' me daalo true return karo
inline bool read_file(const std::string& fs_path, std::string& out){
    std::ifstream file(fs_path,std::ios::binary); //binary: images ke lie zaroori
    if(!file) return false; //file nahi/  khul nahi saki

    std::ostringstream ss;
    ss<< file.rdbuf(); //poori file stream se string me
    out = ss.str();
    return true;

}