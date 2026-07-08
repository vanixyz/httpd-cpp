#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
//handle_client kahin aur defined hai - pool bas uskko bulayega
void handle_client(int client_fd);
class ThreadPool{
    public:
    //Constructor: n workers janm lo , sab worker_loop chalao
    ThreadPool(size_t n){
        for(size_t i=0;i<n;i++){
            workers.emplace_back(&ThreadPool::worker_loop, this);
        }
    }
    //Boss yahan kaam dalta hai 9hmare lie: ek client_fd ka kaam
     void submit(int client_fd){
        {
        std:: lock_guard<std::mutex> lock(mtx);
        clients.push(client_fd);
     }
     cv.notify_one();
    }

    private:
    void worker_loop(){
        while(true){
         int fd;
         {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock , [this]{ return !clients.empty();});
            fd=clients.front();
            clients.pop();
         }
         handle_client(fd); //kaam - taale ke bahar
        }
    }
    // file me order matters (ya forward-declare karo), 
    // class ke andar order sirf insaano ke liye hai, compiler ke liye nahi. 
    std::vector<std::thread> workers;
    std::queue<int> clients;
    std::mutex mtx;
    std::condition_variable cv;
};
