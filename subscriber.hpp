#include "pubsub.hpp"

class Subscriber
{
public:
    uint32_t id;

private:
    void consume(Message message);
};