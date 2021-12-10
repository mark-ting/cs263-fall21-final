#include "broker.hpp"
#include <iostream>
#include <cassert>
#include <sys/select.h>

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
int setup_broker() {
    int master_socket;
    size_t len;
    struct sockaddr_un pub;

    // Socket
    master_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(master_socket != -1);

    pub.sun_family = AF_UNIX;
    strncpy(pub.sun_path, PUB_PATH, strlen(PUB_PATH) + 1);
    unlink(pub.sun_path);

    // Bind
    len = strlen(pub.sun_path) + sizeof(pub.sun_family) + 1;
    assert(bind(master_socket, (struct sockaddr *)&pub, len) != -1);

    // Listen
    assert(listen(master_socket, 5) != -1);

    return master_socket;
}

/* Handles receives a message from a client on fd `pub_fd
 * 
 * Returns: length of message received; 0 on orderly shutdown, -1 on error, o/w > 0
 */
Message receive_client_msg(int client_fd) {
    int length = 0;

    // Create two messages, one from which to read and one from which to receive
    Message recv_message;

    // Read and print message from publisher
    length = recv(client_fd, recv_message.content, MESSAGE_LEN, 0);
    printf("Client Message (socket fd %d): %s\n", client_fd, recv_message.content);

    recv_message.length = length;
    return recv_message;
}

/* Accepts a new socket connection via the master socket
 */
Connection accept_connection(int master_socket) {
    Connection c;

    struct sockaddr_un remote;
    socklen_t t = sizeof(remote);
    int fd = -1;

    // Establish connection with client
    fd = accept(master_socket, (struct sockaddr*)&remote, &t);
    if (fd == -1) {
        printf("Error accepting connections. Errno %d\n", errno);
    }
    assert(fd != -1);

    // Categorize client as pub or sub
    Message msg = receive_client_msg(fd);
    int msg_length = msg.length;
    assert(msg.type == SUB || msg.type == PUB);
    c.type = msg.type;

    std::cout << "Newly Connected Client fd: " << fd << std::endl;

    c.fd = fd;
    return c;
}

/*  Handles connections from broker to multiple publishers
 */
void handle_multiple_publishers(int master_socket) {
    // Bitmap of fdset
    fd_set readfds;

    // Used_sockets is an array containing the socket numbers in use
    Connection used_sockets[MAX_CLIENTS];
    int max_sd = master_socket;
    int client_socket, activity;

    // Initialize all connections and set the first one to server fd
    for (int i = 1; i < MAX_CLIENTS; i++) {
        used_sockets[i].fd = -1;
        used_sockets[i].type = FREE;
    }
    used_sockets[0].fd = master_socket;
    used_sockets[0].type = MASTER;

    // Process ready fds with blocking `select`
    while (1) {
        // Clear the read socket set 
        FD_ZERO(&readfds);  

        // Add master read socket to set 
        FD_SET(master_socket, &readfds);

        // Add child read sockets to set 
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (used_sockets[i].fd >= 0) {
                max_sd = max_sd < used_sockets[i].fd ? used_sockets[i].fd : max_sd;
                FD_SET(used_sockets[i].fd, &readfds);
            }
        }

        // Wait indefinitely for an activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL); 

        // If master socket, then incoming connection 
        if (FD_ISSET(master_socket, &readfds)) {
            Connection c = accept_connection(master_socket);
            // add new socket to array of sockets 
            for (int i = 0; i < MAX_CLIENTS; i++) {  
                if (used_sockets[i].fd == UNUSED) {
                    used_sockets[i] = c;  
                    printf("Adding fd %d to list of sockets as index %d\n" , c.fd, i);  
                    break;
                }
            }
        } else {
            // else its some IO operation on some other socket
            for (int i = 0; i < MAX_CLIENTS; i++) {
                int sd = used_sockets[i].fd;
                if (sd != UNUSED && FD_ISSET(sd, &readfds)) {
                    Message msg = receive_client_msg(sd);
                    int msg_length = msg.length;
                    if (msg_length <= 0) {
                        close(used_sockets[i].fd);
                        used_sockets[i].fd = -1; /* Connection is now closed */
                        used_sockets[i].type = FREE;
                    }
                    break; 
                }
            }
        }
    }
}

int main() {
    int master_socket = -1;
    master_socket = setup_broker();
    handle_multiple_publishers(master_socket);
    close(master_socket);
    return 0;
}