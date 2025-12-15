#include <iostream>
#include <thread>
#include <unistd.h>
#include "smq.h"
#include "PThreadPool.h"
#include "Taskq.h"
#include "print.h"
#include "taskf.h"
#include "info.h"

using namespace std;



int wait_open(const char* name, int flags) {
    mqd_t mq;
    while (true) {
        mq = smq_open(name, flags);

        if (mq==-1||errno == ENOENT) {
            cout << "⏳ Waiting for queue " << name << "...\n";
            sleep(1);
            continue;
        }else 
               break;
     }
 return mq;

}




int main() {

    /* ---------- TASK QUEUE ---------- */

    mqd_t taskMq = wait_open("/sendmsg",O_RDONLY | O_NONBLOCK);

    /* ---------- END QUEUE ---------- */
  mqd_t resMq = smq_open("/res",O_CREAT | O_RDONLY | O_NONBLOCK);

    if (resMq == -1) {
        cout<<"82 resmq is -1 in rec.cpp"<<endl;
        return 1;
    }

    printHeader("RECEIVER SERVICE STARTED");
    slowRefresh(150);
    cout << " Thread Pool Size : 5\n";

    PThreadPool pool(5);

    while (true) {

        /* 🔹 END SIGNAL */
        char endbuf[64]{};
        ssize_t er = mq_receive(resMq, endbuf, sizeof(endbuf), nullptr);
        if (er > 0 && string(endbuf) == "__end__") {
            slowRefresh(150);
            cout << "\n🛑 Receiver shutting down...\n";
            break;
        }

        /* 🔹 RECEIVE TASK */
        TaskQ msg;
       ssize_t r = smq_receive(taskMq,(char*)&msg,sizeof(msg),nullptr);
        if (r == -1) continue;

        if (msg.topic == "end") {
            std::cout << "🛑 End task received\n";
            break;
}

Task t(msg);

       { lock_guard<mutex> lk(printMutex());
        printSection("DISPATCH");
        slowRefresh(150);
        printKV("PID", t.senderPid);
        slowRefresh(150);
        printKV("Topic", t.topic);
        slowRefresh(150);
        printKV("Payload", t.payload);
        slowRefresh(150);
        printKV("Priority", t.priority);
        slowRefresh(150);
        printFooter();
       }
        try {
            auto func = gett(t.topic);
            pool.add(t, func, t.payload);

        } catch (...) {
         perror("in rec.cpp 108  auto func = gett(t.topic);  \n pool.add(t, func, ref(t.payload));");
         return 1;
        }
    }

    mq_close(taskMq);
    mq_close(resMq);;

    return 0;
}
