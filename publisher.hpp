#ifndef PUB_H
#define PUB_H

#include "pubsub.hpp"

class Publisher
{
public:
    uint32_t id;

private:
    void publish(Message message);
};

#endif // PUB_H