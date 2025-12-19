#ifndef PTHREADPOOL_H
#define PTHREADPOOL_H
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>
#include <queue>
#include <functional>
#include <iostream>
#include <mqueue.h>
#include <cstring>
#include "Taskq.h"
#include "info.h"
#include "print.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <atomic>
#include  "cput.h"


struct TaskQ;

struct Task {
private:
    inline static int i = 4;
    inline static int up = 0;
    inline static std::mutex mtx;
    static constexpr uint8_t ENC_KEY = 0x5A;


public:

    std::chrono::steady_clock::time_point enqueueTime;
    bool cs=0;
    int priority;
    pid_t senderPid;
    int id;
    std::string topic = "";
    std::string payload = "";
    uint8_t checksum = 0;
    std::function<void()> func;
    bool valid;

    Task() : priority(0), senderPid(0) {}

    Task(pid_t sid, std::string t, std::string pay, int ch, int p = -1)
        : senderPid(sid), topic(t), payload(pay), checksum(ch)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (p == -1) {
            i++;
            priority = i;
        } else {
            priority = p;
        }
    }

     std::string decrypt(const char* data) {
        std::string result(data);
        for (char &c : result) {
            c ^= ENC_KEY;
        }
        return result;
    }


      Task(const TaskQ& t) {
          senderPid = t.senderPid;
          topic = decrypt(t.topic);       // decrypt topic
          payload = decrypt(t.payload);   // decrypt payload
          id=t.ind;
          cs=t.cs;
          checksum =xorChecksum(topic) ^ xorChecksum(payload);
          valid = (checksum==t.checksum);
          if (t.priority == -1) {
              std::lock_guard<std::mutex> lock(mtx);
              i++;
              priority = i;
            } else {
                priority = t.priority;
            }
           enqueueTime = std::chrono::steady_clock::now();
    }



    static void done() {
        std::lock_guard<std::mutex> lock(mtx);
        up++;
    }

    uint8_t xorChecksum(std::string data) {
        uint8_t cs = 0;
        for (int i = 0; data[i]; i++)
            cs ^= (uint8_t)data[i];
        return cs;
    }

    bool check(std::string s = "", std::string t = "") {
        uint8_t x = xorChecksum(s) + xorChecksum(t);
        return x == checksum;
    }

    void addf(std::function<void()> f) {
        func = f;
    }

    int effectivePriority() const {
        using namespace std::chrono;
    auto waited = duration_cast<milliseconds>(
        steady_clock::now() - enqueueTime
    ).count();

    return priority + (waited / 100); // +1 priority per 100ms
    }
};

struct  fail
{
    int id;
    bool f;
    fail(int id,bool f):id(id),f(f){};
};


class PThreadPool {
    struct cmp {
        bool operator()(const Task& a, const Task& b) const {
            return a.effectivePriority() > b.effectivePriority();
        }
    };
private:
std::atomic<int> workingThreads{0}; 
   
public:
   struct PoolStats {
    int total;
    int working;
    int waiting;
    int queued;
};
PoolStats stats() {
    std::lock_guard<std::mutex> lock(mtx);
    return {
        size,
        workingThreads.load(),
        size-workingThreads.load(),
        (int)pq.size()
    };
}
private:
    

    bool stop = false;
    int size;
    std::vector<std::future<void>> workers;
    std::priority_queue<Task, std::vector<Task>, cmp> pq;
    std::mutex mtx;
    std::condition_variable cv;

    void workerThread() {
        while (true) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&] { return stop || !pq.empty(); });

                if (stop && pq.empty())
                    return;
               
                task = pq.top();
                pq.pop();
            }

            bool status = true;

/* CPU snapshot BEFORE execution */
CpuStat cpuStart = readCPU();

if(!task.valid)status=false;

else{

ThreadCpuTime tStart = threadCpuNow();

try {
    workingThreads++;    
 int working = workingThreads.load();
    int waiting = size - working;
    int queued  = pq.size();
 {   
lock_guard<mutex> lk(printMutex());
printSection("Thread at the exicuting time");
    cout<<"Working"<< working<< "/"<< size<<endl;
    cout<<"Waiting"<<waiting<<"/"<<size<<endl;
    cout<<"Queued"<<queued<<"/"<<10<<endl;
    printFooter();
 }

task.func();
workingThreads--;
} catch (...) {
    status = false;
}

ThreadCpuTime tEnd = threadCpuNow();
double threadCpu = threadCpuMs(tStart, tEnd);


/* CPU snapshot AFTER execution */
CpuStat cpuEnd = readCPU();
double cpu = cpuUsage(cpuStart, cpuEnd);

/* THREAD EXECUTION PRINT */
if(task.cs){
  {  
 lock_guard<mutex> lk(printMutex());
printSection("THREAD EXECUTION");
slowRefresh(150);
printKV("Thread CPU", threadCpu, " ms");
slowRefresh(150);
printKV("Thread ID", (int)syscall(SYS_gettid));
slowRefresh(150);
printKV("TaskID", task.id);
slowRefresh(150);
printKV("Task", task.topic);
slowRefresh(150);
printKV("Payload", task.payload);
slowRefresh(150);
printKV("Status", status ? "SUCCESS" : "FAILED");
slowRefresh(150);

printKV("CPU Used", cpu, "%");
slowRefresh(200);
printCPULoad(cpu);
slowRefresh(200);
printFooter();

slowRefresh(600);

auto s = stats();

printSection("THREAD POOL STATUS (BAR)");
slowRefresh(150);
printBar("Working", s.working, s.total);
slowRefresh(150);
printBar("Waiting", s.waiting, s.total);
slowRefresh(150);
int qMax = std::max(s.total * 2, s.queued);
printBar("Queued", s.queued, qMax);
slowRefresh(250);
printFooter();
    slowRefresh(1000); 
  }
}

}

/* response + bookkeeping */

sucess(task.id, status);
Task::done();
 

}
    }

public:
    PThreadPool(int n) : size(n) { 
        for (int i = 0; i < n; i++) {
            workers.emplace_back(
                std::async(std::launch::async, [this] { workerThread(); })
            );
        }
    }

    ~PThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
    }

    template<class F, class... Args>
    auto add(Task t, F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using Ret = typename std::invoke_result<F, Args...>::type;

        auto taskPack = std::make_shared<std::packaged_task<Ret()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<Ret> res = taskPack->get_future();

        t.addf([taskPack]() { (*taskPack)(); });

        {
            std::unique_lock<std::mutex> lock(mtx);
            pq.push(t);
        }
        cv.notify_one();

        return res;
    }


void sucess(int id, bool ok)
{
    static mqd_t mq = -1;
    static std::once_flag once;

    std::call_once(once, [](){
        mq = smq_open("/fail", O_WRONLY);
        if (mq == -1) exit(1);
    });

    fail msg{id, !ok};
    smq_send(mq, (char*)&msg, sizeof(msg));
}

};

#endif



