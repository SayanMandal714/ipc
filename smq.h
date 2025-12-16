
#ifndef SMQ_H
#define SMQ_H
#include <limits>
#include <cerrno>
#include <iostream>
#include <unistd.h>
#include <mqueue.h>
#include <fcntl.h>
#include <cstring>

mqd_t smq_open(const char* name,int flags,mode_t mode = 0666,mq_attr* attr = nullptr)
{
    mqd_t mq = mq_open(name, flags, mode, attr);
 
 if (mq == -1) {
        switch (errno) {
            case EACCES:
                std::cerr << "\nPermission denied\n"; break;
            case EEXIST:
                std::cerr << "\nQueue already exists\n"; break;
            case EINVAL:
                std::cerr << "\nInvalid name or attributes\n"; break;
            case ENOENT:
                std::cerr << "\nQueue does not exist\n"; break;
            case EMFILE:
                std::cerr << "\nProcess file descriptor limit reached\n"; break;
            case ENFILE:
                std::cerr << "\nSystem-wide FD limit reached\n"; break;
            case ENOSYS:
                std::cerr << "\nPOSIX MQ not supported\n"; break;
            default:
                std::cerr << strerror(errno) << "\n";
        }
    }
    return mq;
}


mqd_t smq_send(mqd_t mq, const char* data, size_t size, unsigned prio=3) {
    mqd_t x=mq_send(mq, data, size, prio);
    if ( x== -1) {

        switch (errno) {
            case EAGAIN: break;
            case EBADF:
                std::cerr << "\nInvalid MQ descriptor\n"; break;
            case EMSGSIZE:
                std::cerr << "\nMessage too large\n"; break;
            case EINTR:
                std::cerr << "\nInterrupted by signal\n"; break;
            default:
                std::cerr << strerror(errno) << "\n";
        }
       
    }
    return x;
}

 ssize_t smq_receive(mqd_t mq, char* buffer, size_t size, unsigned* prio=nullptr) {
    ssize_t ret = mq_receive(mq, buffer, size, prio);

    if (ret == -1) {

        switch (errno) {
            case EAGAIN:
                 break;
            case EBADF:
                std::cerr << "\nInvalid MQ descriptor\n"; break;
            case EMSGSIZE:
                std::cerr << "\nBuffer too small\n"; break;
            case EINTR:
                std::cerr << "\nInterrupted by signal\n"; break;
            default:
                std::cerr << strerror(errno) << "\n";
        }
    }
    return ret;
}



#endif
