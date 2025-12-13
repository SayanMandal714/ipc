#ifndef PTHREADPOOL_H
#define PTHREADPOOL_H

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

struct TaskQ;

struct Task {
private:
    inline static int i = 4;
    inline static int up = 0;
    inline static std::mutex mtx;

public:
    int priority;
    pid_t senderPid;
    std::string topic = "";
    std::string payload = "";
    uint8_t checksum = 0;
    std::function<void()> func;

    Task() : priority(0), senderPid(0) {}

    Task(pid_t sid, std::string t, std::string pay, int cs, int p = -1)
        : senderPid(sid), topic(t), payload(pay), checksum(cs)
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (p == -1) {
            i++;
            priority = i;
        } else {
            priority = p;
        }
    }

    Task(const TaskQ& t) {
        senderPid = t.senderPid;
        topic = t.topic;
        payload = t.payload;
        checksum = t.checksum;

        if (t.priority == -1) {
            std::lock_guard<std::mutex> lock(mtx);
            i++;
            priority = i;
        } else {
            priority = t.priority;
        }
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
        return priority - up;
    }
};

class PThreadPool {
    struct cmp {
        bool operator()(const Task& a, const Task& b) const {
            return a.effectivePriority() < b.effectivePriority();
        }
    };

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
            try {
                task.func();
            } catch (...) {
                status = false;
            }

            sucess(task.senderPid, status);
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

    void sucess(int id, bool t) {
        std::string fail = "/fail";
        mqd_t mq = mq_open(fail.c_str(), O_CREAT | O_WRONLY, 0666, NULL);
        mq_send(mq, (char*)&id, sizeof(int), 0);
    }
};

#endif
