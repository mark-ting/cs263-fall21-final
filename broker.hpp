#ifndef BROKER_H
#define BROKER_H

#include <queue>
#include <unordered_map>

#include "pubsub.hpp"

#define MAX_CLIENTS     100
// Used connection constants
#define UNUSED          -1
#define PORT            8080

typedef struct Filter {
    Filter* next;
    char regex[MESSAGE_LEN];
} Filter;

typedef struct Connection {
    int fd;
    ConnType type;
    Filter* filter;
} Connection;

#endif // BROKER_H