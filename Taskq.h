#ifndef taskq
#define taskq
#include <cstring>
#include <iostream>
#include <string>
#include <cstdint>   
#include <unistd.h>
using namespace std;

struct TaskQ { 
    int priority;
    int ind;
    bool cs;
    pid_t senderPid;
    char topic[300];
    char payload[300];
    uint8_t checksum;

    TaskQ() {}

    TaskQ(pid_t sid, const std::string &t, const std::string &pay, int id,bool b,int p=-1) {
        priority = p;
        senderPid = sid;
        ind = id;
        cs=b;

        // Encrypt and store topic
        strncpy(topic, t.c_str(), sizeof(topic));
        xorEncrypt(topic, sizeof(topic));

        // Encrypt and store payload
        strncpy(payload, pay.c_str(), sizeof(payload));
        xorEncrypt(payload, sizeof(payload));

        // Compute checksum of encrypted data
        checksum = xorChecksum(topic) ^ xorChecksum(payload);
    }

    // XOR encryption/decryption
    void xorEncrypt(char* data, size_t size) {
       uint8_t ENC_KEY = 0x5A; 
        for (size_t i = 0; i < size && data[i] != '\0'; i++) {
            data[i] ^= ENC_KEY;
        }
    }

    uint8_t xorChecksum(const char* data) const {
        uint8_t cs = 0;
        for (int i = 0; data[i]; i++) {
            cs ^= (uint8_t)data[i];
        }
        return cs;
    }
};

#endif