#include "pubsub.hpp"

class Publisher
{
public:
    uint32_t id;

private:
    void publish(Message message);
};