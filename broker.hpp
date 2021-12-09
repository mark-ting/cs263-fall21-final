#include "pubsub.hpp"

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