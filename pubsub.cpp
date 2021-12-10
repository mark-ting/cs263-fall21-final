#include "broker.hpp"
#include "pubsub.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <iostream>

/* Connects to broker server
 */
int connect_to_broker() {
    int broker_fd;
    size_t len;
    struct sockaddr_un remote;

    // Socket
    struct sockaddr_in broker;

    broker_fd = socket(AF_INET, SOCK_STREAM, 0);
    assert(broker_fd != -1);

    // Connect
    std::cout << "Attempting to connect to broker...\n";

    broker.sin_family = AF_INET;
    broker.sin_addr.s_addr = inet_addr("127.0.0.1");
    broker.sin_port = htons(PORT);
    assert(connect(broker_fd, (struct sockaddr *)&broker, sizeof(broker)) != -1);

    std::cout << "Connected to broker on fd " << broker_fd << std::endl;

    return broker_fd;
}