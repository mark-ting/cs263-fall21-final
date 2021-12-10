#include "broker.hpp"
#include <iostream>
#include <cassert>
#include <sys/select.h>

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

    Message recv_message;

    // Read and print message from publisher
    length = recv(client_fd, &recv_message, sizeof(Message), 0);
    recv_message.length = length;
    assert(recv_message.type == SUB || recv_message.type == PUB);
    if (length == 0) {
        // Msg 0 length means closed connection
        return recv_message;
    }

    if (recv_message.type == PUB) {
        printf("Client (PUB) Message (socket fd %d): %s\n", client_fd, recv_message.content);
    } else {
        printf("Client (SUB) Message (socket fd %d): %s\n", client_fd, recv_message.content);

    }

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

    if (c.type == PUB) {
        std::cout << "Newly Connected Client (Type: PUB) fd: " << fd << std::endl;
    } else {
        std::cout << "Newly Connected Client (Type: SUB) fd: " << fd << std::endl;
    }

    c.fd = fd;
    c.filter = NULL;
    return c;
}

/*  Filter helper: Frees a linked list of Filters starting from the root
 */
void free_filter_list(Filter* root) {
    Filter* curr = root;
    Filter* next;
    while (curr) {
        next = curr->next;
        free(curr);
        curr = next;
    }
}

/*  Filter helper: Appends `filter` to the end of linked list of Connection c
 */
void add_filter_list(Connection* c, Filter* filter) {
    filter->next = NULL;
    if (!c->filter) {
        c->filter = filter;
        return;
    }
    Filter* last_node = c->filter;
    while (last_node) {
        if (!last_node->next) {
            break;
        }
        last_node = last_node->next;
    }
    last_node->next = filter;
}

/*  Returns if all filters starting from `root` are sufficed by `content`
 */
bool filters_met(Filter* root, char* content) {
    Filter* curr = root;
    while (curr) {
        std::regex re(curr->regex);
        bool match = std::regex_search(content, re);
        if (!match) {
            return false;
        }
        curr = curr->next;
    }
    return true;
}

/*  Processes a client message. If from pub, publishes. If from sub, adds regex filter.
 *  Connection array `arr` of size `sz`. `fd` is at index `idx` in `arr`
 *
 *  Returns the message
 */
Message process_client_message(int fd, int idx, Connection* arr, int sz) {
    // idx cannot be more than max size of arr
    assert(idx < sz && idx >= 0);
    Message recv_msg = receive_client_msg(fd);

    if (recv_msg.length == 0) {
        // Length 0 means end connection
        return recv_msg;
    }

    // Sending client type should equal stored type in connection arr
    assert(recv_msg.type == arr[idx].type);

    
    // Publish messages from publishers to subscribers
    if (recv_msg.type == PUB) {
        Message send_msg;
        send_msg.type = BROKER;
        strncpy(send_msg.content, recv_msg.content, MESSAGE_LEN);
        send_msg.length = strlen(send_msg.content) + 1;
        for (int i = 0; i < sz; i++) {
            if (arr[i].fd >= 0 && arr[i].type == SUB) {
                // TODO: ADD IF FILTER CONDITIONS ARE MET
                char content[MESSAGE_LEN];
                strncpy(content, recv_msg.content, MESSAGE_LEN);
                content[strcspn(content, "\n")] = 0;
                if (filters_met(arr[i].filter, content)) {
                    assert(send(arr[i].fd, &send_msg, sizeof(send_msg), 0) != -1);
                }
            }
        }
    } else {
        // Add filter to filter list
        Filter* f = (Filter*) malloc(sizeof(Filter));
        strncpy(f->regex, recv_msg.content, MESSAGE_LEN);
        f->regex[strcspn(f->regex, "\n")] = 0;
        add_filter_list(&arr[idx], f);
    }

    return recv_msg;
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
        used_sockets[i].filter = NULL;
    }
    used_sockets[0].fd = master_socket;
    used_sockets[0].type = BROKER;
    used_sockets[0].filter = NULL;

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
                    Message msg = process_client_message(sd, i, used_sockets, MAX_CLIENTS);
                    int msg_length = msg.length;
                    if (msg_length <= 0) {
                        close(used_sockets[i].fd);
                        used_sockets[i].fd = -1; /* Connection is now closed */
                        used_sockets[i].type = FREE;
                        free_filter_list(used_sockets[i].filter);
                        used_sockets[i].filter = NULL;
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