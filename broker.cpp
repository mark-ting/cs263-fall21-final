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

int connect_to_publisher() {
    int pub_broker_fd;
    size_t len;
    struct sockaddr_un remote;

    // Socket
    pub_broker_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(pub_broker_fd != -1);

    // Connect
    std::cout << "Attempting to connect to publisher...\n";

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, PUB_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
    assert(connect(pub_broker_fd, (struct sockaddr *)&remote, len) != -1);

    std::cout << "Connected to publisher on fd " << pub_broker_fd << std::endl;

    return pub_broker_fd;
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
        break;
    } while (!done);
}

int main() {
    int pub_fd = -1;
    pub_fd = connect_to_publisher();

    handle_publisher_connection(pub_fd);
    close(pub_fd);
    return 0;
}