
#ifndef taskq
#define taskq
#include <cstring>
#include <iostream>

using namespace std;
struct TaskQ { //for mque
    int priority;
    int ind;
    pid_t senderPid;
    char topic[32];
    char payload[256];
    uint8_t checksum;
    TaskQ(){}
    TaskQ(pid_t sid, const std::string &t, const std::string &pay, int id,int p=-1) {
        priority = p;
        senderPid = sid;
        ind = id;
        strncpy(topic, t.c_str(), sizeof(topic));
        strncpy(payload, pay.c_str(), sizeof(payload));
        checksum = xorChecksum(topic) ^ xorChecksum(payload);
    }

    uint8_t xorChecksum(const char* data) {
        uint8_t cs = 0;
        for (int i = 0; data[i]; i++) {
            cs ^= (uint8_t)data[i];
        }
        return cs;
    }
};

#endif