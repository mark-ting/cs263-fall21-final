#include "broker.hpp"
#include <iostream>
#include <cassert>

void Broker::receive(uint32_t pub_id, Message message)
{
    message_queue.push(message);
}

void Broker::notify(uint32_t sub_id, Message message)
{
    // TODO: send queue()
    std::cout << "Queue is empty!" << std::endl;
    return;
}

void Broker::process_queue()
{
    if (!message_queue.empty())
    {
        const auto message = message_queue.front();

        // iterate through filters
        for (auto const &sub_pair : filter_list)
        {
            const auto sub = sub_pair.first;
            const auto filter = sub_pair.second;

            if (filter(message))
            {
                notify(sub, message);
            }
        }

        // remove message from queue
        message_queue.pop();
    }
    else
    {
        std::cout << "Queue is empty!" << std::endl;
    }
}

/* Returns a socket address for the publisher that listens for broker connections
 */
int setup_server() {
    int pub_socket;
    size_t len;
    struct sockaddr_un pub;

    // Socket
    pub_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(pub_socket != -1);

    pub.sun_family = AF_UNIX;
    strncpy(pub.sun_path, PUB_PATH, strlen(PUB_PATH) + 1);
    unlink(pub.sun_path);

    // Bind
    len = strlen(pub.sun_path) + sizeof(pub.sun_family) + 1;
    assert(bind(pub_socket, (struct sockaddr *)&pub, len) != -1);

    // Listen
    assert(listen(pub_socket, 5) != -1);

    return pub_socket;
}

/* Handles a connection with a publisher on fd `pub_fd
 */
void handle_publisher_connection(int pub_fd) {
    int done = 0;
    int length;

    // Create two messages, one from which to read and one from which to receive
    Message send_message;
    Message recv_message;

    // Read and print messages from publisher
    do {
        length = recv(pub_fd, recv_message.content, MESSAGE_LEN, 0);
        std::cout << "Publisher Message: " << recv_message.content << std::endl;
    } while (!done);
}

int main() {
    int broker_fd = -1;
    broker_fd = setup_server();

    struct sockaddr_un remote;
    socklen_t t = sizeof(remote);
    int pub_fd = 0;

    pub_fd = accept(broker_fd, (struct sockaddr *)&remote, &t);

    std::cout << "Connected Pub Socket: " << pub_fd << std::endl;

    handle_publisher_connection(pub_fd);
    close(pub_fd);
    return 0;
}