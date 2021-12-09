#include <cstdint>
#include <functional>
#include <map>
#include <queue>
#include <regex>
#include <unordered_map>
#include <vector>

struct Message
{
    uint32_t topic;
    char content[512];
};

typedef std::function<bool(Message)> Filter;
