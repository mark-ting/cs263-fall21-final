#ifndef PUBSUB_H
#define PUBSUB_H

#include <cstdint>
#include <functional>
#include <regex>
#include <errno.h>
// Sockets
#include <sys/types.h>          
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

// Maximum input string length from pub/sub client stdin
// Should equal `MESSAGE_LEN`
#define DEFAULT_STDIN_BUFFER_SIZE       512
#define MESSAGE_LEN                     512
#define PUB_PATH                        "./pub.path"

typedef enum ConnType {
    PUB,
    SUB,
    BROKER,
    FREE
} ConnType;

struct Message
{
    uint32_t topic;
    ConnType type;
    int length;
    char content[MESSAGE_LEN];
};

typedef std::function<bool(Message)> Filter;

int connect_to_broker();

#endif // PUBSUB_H
