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

int main() {
    int sockfd;
    size_t len;
    struct sockaddr_un remote;

    // Socket
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(sockfd != -1);

    // Connect
    std::cout << "Attempting to connect to publisher...\n";

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, PUB_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family) + 1;
    assert(connect(sockfd, (struct sockaddr *)&remote, len) != -1);

    std::cout << "Connected to publisher\n";

    return 0;
}