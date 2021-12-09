#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <regex>
#include <unordered_map>
#include <vector>

typedef std::function<bool(Message)> Filter;

struct Message
{
    uint32_t topic;
    char content[512];
};

class QueueServer
{
public:
    void process_queue();
    void notify(uint32_t sub_id, Message message);
    void receive(uint32_t pub_id, Message message);

private:
    std::queue<Message> message_queue;
    std::unordered_map<uint32_t, Filter> filter_list;
};

class Subscriber
{
public:
    uint32_t id;

private:
    void consume(Message message);
};

class Publisher
{
public:
    uint32_t id;

private:
    void publish(Message message);
};
