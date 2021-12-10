#include "subscriber.hpp"
#include <cassert>
#include <iostream>
#include <stdio.h>

/* Reads messages from stdin and sends to a broker via `broker_socket` fd
 * Returns 0 on success, -1 on close or error
 */
int send_broker_message(int broker_fd) {
    Message send_message;
    Message recv_message;
    char* output_str = NULL;
    int msg_length = 0;

    // Continuously loop and wait for input. At each iteration:
    // 1. output interactive marker
    // 2. read from stdin until eof.
    char read_buffer[DEFAULT_STDIN_BUFFER_SIZE];

    output_str = fgets(read_buffer, DEFAULT_STDIN_BUFFER_SIZE, stdin);
    if (!output_str) {
        std::cerr << "fgets() failed\n" << std::endl;
        return -1;
    }
    if (feof(stdin)) {
        return -1;
    }
    msg_length = strlen(read_buffer);
    if (msg_length > 1) {
        send_message.type = SUB;
        strncpy(send_message.content, read_buffer, MESSAGE_LEN);
        assert(send(broker_fd, &send_message, sizeof(Message), 0) != -1);
    }
    return 0;
}

/*  Prints the `subscriber >` terminal prefix
 */ 
void print_prefix() {
    // Always output an interactive marker at the start of each command if the
    // input is from stdin. Do not output if piped in from file or from other fd
    char prefix[20]; 
    if (isatty(fileno(stdin))) {
        strncpy(prefix, "subscriber > ", 14);
    } else {
        strncpy(prefix, "", 2);
    }
    printf("%s", prefix);
}

/*  Declares to server connection is subscriber
 */
void declare_subscriber(int broker_fd) {
    Message send_message;
    
    // State client is a subscriber
    send_message.type = SUB;
    strncpy(send_message.content, "Connecting SUB", MESSAGE_LEN);
    assert(send(broker_fd, &send_message, sizeof(Message), 0) != -1);
}

/* Blocks until either `stdin` or `broker_fd` is ready to be read from
 */
void process_messages(int broker_fd) {
    // Bitmap of fdset
    fd_set readfds;
    
    Message recv_message;
    // Process ready fds with blocking `select`
    while (1) {
        // Clear the read set 
        FD_ZERO(&readfds); 

        // Add broker & stdin read socket to set 
        FD_SET(broker_fd, &readfds);
        FD_SET(fileno(stdin), &readfds);

        // Wait indefinitely for an activity on one of the sockets
        int activity = select(broker_fd + 1, &readfds, NULL, NULL, NULL); 

        // If broker socket, then incoming message
        if (FD_ISSET(broker_fd, &readfds)) {
            int length = recv(broker_fd, &recv_message, sizeof(Message), 0);
            if (length == 0) {
                // Length 0 means broker end of socket closed -> close subscriber
                break;
            }
            printf("\nSubscriber message received: %s", recv_message.content);
            print_prefix();
        } else if (FD_ISSET(fileno(stdin), &readfds)) {
            // else, stdin
            if (send_broker_message(broker_fd) == -1) {
                break;
            }
            print_prefix();
        }
    } 
}

int main() { 
    int broker_fd;
    broker_fd = connect_to_broker();
    // Sends initial read receipt to broker
    declare_subscriber(broker_fd);
    // Prints `subscriber >` terminal prefix without buffer
    setvbuf(stdout, NULL, _IONBF, 0);
    print_prefix();
    // Reads user input or broker messages
    process_messages(broker_fd);
    close(broker_fd);
    return 0;
}