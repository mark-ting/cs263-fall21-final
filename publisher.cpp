#include "publisher.hpp"
#include <cassert>
#include <iostream>


/* Connects to broker server
 */
int connect_to_broker() {
    int broker_fd;
    size_t len;
    struct sockaddr_un remote;

    // Socket
    broker_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(broker_fd != -1);

    // Connect
    std::cout << "Attempting to connect to broker...\n";

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, PUB_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
    assert(connect(broker_fd, (struct sockaddr *)&remote, len) != -1);

    std::cout << "Connected to broker on fd " << broker_fd << std::endl;

    return broker_fd;
}

/* Sends messages to a broker via `broker_socket` fd
 */
void send_to_broker(int broker_fd) {
    Message send_message;
    char* message = "This is some content";
    strcpy(send_message.content, message);
    assert(send(broker_fd, send_message.content, MESSAGE_LEN, 0) != -1);
}

int main() { 
    int broker_fd;
    broker_fd = connect_to_broker();

    send_to_broker(broker_fd);

    return 0;
}