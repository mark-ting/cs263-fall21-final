#ifndef SUB_H
#define SUB_H

#include "pubsub.hpp"

class Subscriber
{
public:
    uint32_t id;

private:
    void consume(Message message);
};

#endif // SUB_H