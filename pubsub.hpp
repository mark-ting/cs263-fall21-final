#include <cstdint>
#include <functional>
#include <regex>
// Sockets
#include <sys/types.h>          
#include <sys/socket.h>
#include <sys/un.h>


#define PUB_PATH "./pub.path"

struct Message
{
    uint32_t topic;
    char content[512];
};

typedef std::function<bool(Message)> Filter;
