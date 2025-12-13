#include<iostream>
#include<string>
#include<fstream>
#include "PThreadPool.h"
#include <thread> 

#include <mqueue.h>
#include <fcntl.h>     
#include <sys/stat.h>
#include <string>
#include"Taskq.h"
#include<unordered_map>

using namespace std;
void computeTask(string&s){
    cout<<"task is computing.....  "<<s<<endl;
}
unordered_map<string, function<void(string&)>> taskMap = {
    {"compute", computeTask},
};

function<void(string&)> gett(string &topic) 
{
    if (taskMap.find(topic) != taskMap.end()) {
        return taskMap[topic];   // return function
    }
    return [](string &data){
        std::cout << "Unknown topic: " << data << "\n";
    };
}


int main() {
    string getm ="/sendmsg";
    
    PThreadPool pool(5);
    mqd_t mq = mq_open(getm.c_str(), O_RDONLY | O_CREAT, 0666, NULL);
    mqd_t res = mq_open("/res", O_RDONLY | O_CREAT, 0666, NULL);
    struct mq_attr attr;
    struct mq_attr at;
    while(true){
        
       char b[256];
     ssize_t ret = mq_receive(res, b, sizeof(b), NULL);
     if(ret > 0){
    string r(b);
    if(r == "__end__"){
        mq_unlink(getm.c_str());
        mq_unlink("/res");
        break;
    }
 }


        mq_getattr(mq, &attr);
        while (attr.mq_curmsgs==0)
        {
            std::this_thread::sleep_for(std::chrono::seconds(7));
            continue; 
        }
        
   TaskQ msg;
        ssize_t mret = mq_receive(mq, (char*)&msg, sizeof(msg), NULL);
        if(mret == -1){
            if(errno == EAGAIN){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            } else {
                perror("mq_receive /sendmsg error");
                continue;
            }
        }
     Task t(msg);

    auto func = gett(t.topic); 
    pool.add(t,func,ref(t.payload));
    
    }
    mq_close(mq);
    mq_close(res);

   return 0;

}
 
