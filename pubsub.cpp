#include "pubsub.hpp"
#include <iostream>

void QueueServer::receive(uint32_t pub_id, Message message)
{
    message_queue.push(message);
}

void QueueServer::notify(uint32_t sub_id, Message message)
{
    // TODO: send queue()
    std::cout << "Queue is empty!" << std::endl;
    return;
}

void QueueServer::process_queue()
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