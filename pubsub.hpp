#include <cstdint>
#include <functional>
#include <regex>
// Sockets
#include <sys/types.h>          
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define PUB_PATH                        "./pub.path"
#define MESSAGE_LEN                     512
// Maximum input string length from pub/sub client stdin
// Should equal `MESSAGE_LEN`
#define DEFAULT_STDIN_BUFFER_SIZE       512

struct Message
{
    uint32_t topic;
    char content[MESSAGE_LEN];
};

typedef std::function<bool(Message)> Filter;
