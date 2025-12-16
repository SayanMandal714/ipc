#include <iostream>
#include <fstream>
#include <unordered_map>
#include <mqueue.h>
#include <unistd.h>
#include <cstdlib>
#include "smq.h"
#include "Taskq.h"
#include <vector>
#include <string>
#include <sstream>
#include "print.h"

using namespace std;
inline int safeStoi(const std::string& s, int def = -1) {
    try {
        if (s.empty() || s == " ")
            return def;
        return std::stoi(s);
    } catch (...) {
        return def;
    }
}

bool isInt(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-' || s[0] == '+') ? 1 : 0;
    for (; i < s.size(); i++)
        if (!isdigit(s[i])) return false;
    return true;
}

std::vector<std::string> split(const std::string& input) {
    std::vector<std::string> parts;
    std::istringstream iss(input);
    std::string word;

    while (iss >> word) {
        parts.push_back(word);
    }

    // default output
    std::vector<std::string> result = { " ", " ", " ", "false" };

    // index 0 & 1
    if (parts.size() > 0) result[0] = parts[0];
    if (parts.size() > 1) result[1] = parts[1];

    // index 2 → only if int
    if (parts.size() > 2 && isInt(parts[2])) {
        result[2] = parts[2];
    }

    // index 3 → true if 3rd token exists
    if (parts.size() > 2&&parts[parts.size()-1]=="true") {
        result[3] = "true";
    }

    return result;
}

struct TaskEntry {
    TaskQ task;
    int failCount = 0;
};

struct fail {
    int id;
    bool f;
    fail(int id = 0, bool f = false) : id(id), f(f) {}
};



int main() {
    // Remove old queues
    mq_unlink("/sendmsg");
    mq_unlink("/fail");
    mq_unlink("/res");

    // ---------------- FAIL QUEUE ----------------
   mq_attr fattr{};
fattr.mq_flags   = 0;
fattr.mq_maxmsg  = 10;
fattr.mq_msgsize = sizeof(fail);
fattr.mq_curmsgs = 0;

mqd_t failMq = smq_open(
    "/fail",
    O_CREAT | O_EXCL | O_RDWR,
    0666,
    &fattr
);

if (failMq == -1 && errno == EEXIST) {
    // Queue already exists → open normally (NO ATTR)
    failMq = smq_open("/fail", O_RDWR);
}

if (failMq == -1) {
    perror("mq_open /fail");
    exit(1);
}


    // ---------------- SEND QUEUE ----------------
    mq_attr tattr{};
    tattr.mq_flags = 0;
    tattr.mq_maxmsg = 10;
    tattr.mq_msgsize = sizeof(TaskQ);

    mqd_t sendMq = smq_open("/sendmsg", O_CREAT | O_WRONLY, 0666, &tattr);
    if (sendMq == -1) {
        perror("mq_open /sendmsg");
        exit(1);
    }

    // ---------------- RES QUEUE ----------------
    mq_attr rattr{};
    rattr.mq_maxmsg = 8;
    rattr.mq_msgsize = 64;
    rattr.mq_flags = 0;

    mqd_t endMq = smq_open("/res", O_CREAT | O_WRONLY, 0666, &rattr);
    if (endMq == -1) {
        perror("mq_open /res");
        exit(1);
    }

    cout << "All queues created successfully!\n";
    
    ifstream fin("input.txt");
    if (!fin) {
        perror("input.txt");
        return 1;
    }
    cout << "Sender PID: " << getpid() << endl;

    int globalTaskId = 1;
    bool userEnded = false;
    unordered_map<int, TaskEntry> taskMap;

    while (true) {


        for (int i = 0; i < 6; i++) {

            string input;
            if (!getline(fin, input) || input == "end") {
                userEnded = true;
                break;
            }
            string topic;
            string payload;
            bool cs;
           vector<string>in=split(input);
           topic=in[0];


           if(topic=="end"){
            userEnded = true;
                break;}

           payload=in[1];
           cs=(in[3]=="true")?1:0;
           int pr = safeStoi(in[2]);
              TaskQ tq(getpid(), topic, payload, globalTaskId, cs, pr);


           TaskEntry te;
           te.task = tq;
          te.failCount = 0;
        taskMap[globalTaskId] = te;

            smq_send(sendMq,(char*)&tq,sizeof(TaskQ),0);
            globalTaskId++;
        }

        while (!taskMap.empty()){

            fail fm{};
            ssize_t r = smq_receive(failMq,(char*)&fm,sizeof(fm));

          if (r == -1) {
          usleep(100000);
              continue;
                 }

            auto it = taskMap.find(fm.id);
            if (it == taskMap.end()) continue;

            if (fm.f) {
                it->second.failCount++;
                if (it->second.failCount > 2){
                 cout << "❌ Task " << fm.id << " DROPPED\n";
                    taskMap.erase(it);}
                else
                    smq_send(sendMq,(char*)&it->second.task,sizeof(TaskQ));
            } else {
                cout << "✅ Task " << fm.id << " completed\n";
                taskMap.erase(it);
            }
        }

        if (userEnded) {
            const char* endMsg = "__end__";
            smq_send(endMq, endMsg, strlen(endMsg) + 1, 0);
            break;
        }
    }
    mq_close(failMq);
   mq_close(endMq);
    mq_close(sendMq);
        cout << "Sender exited cleanly\n";
        return 0;
}
