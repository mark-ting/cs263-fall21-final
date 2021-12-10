#ifndef BROKER_H
#define BROKER_H

#include <queue>
#include <unordered_map>

#include "pubsub.hpp"

#define MAX_CLIENTS     100
// Used connection constants
#define UNUSED          -1

class Broker
{
public:
    void process_queue();
    void notify(uint32_t sub_id, Message message);
    void receive(uint32_t pub_id, Message message);

private:
    std::queue<Message> message_queue;
    std::unordered_map<uint32_t, Filter> filter_list;
};

typedef struct Connection {
    int fd;
    ConnType type;
} Connection;

#endif // BROKER_H