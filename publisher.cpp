#include "broker.hpp"
#include "publisher.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <iostream>

/* Reads messages from stdin and sends to a broker via `broker_socket` fd
 */
void process_broker_messages(int broker_fd) {
    // Always output an interactive marker at the start of each command if the
    // input is from stdin. Do not output if piped in from file or from other fd
    char prefix[20]; 
    if (isatty(fileno(stdin))) {
        strncpy(prefix, "publisher > ", 13);
    } else {
        strncpy(prefix, "", 2);
    }

    Message send_message;
    Message recv_message;
    char* output_str = NULL;
    int msg_length = 0;

    // Continuously loop and wait for input. At each iteration:
    // 1. output interactive marker
    // 2. read from stdin until eof.
    char read_buffer[DEFAULT_STDIN_BUFFER_SIZE];

    // State client is a publisher
    send_message.type = PUB;
    strncpy(send_message.content, "Connecting PUB", MESSAGE_LEN);
    send_message.length = strlen(send_message.content) + 1;
    assert(send(broker_fd, &send_message, sizeof(Message), 0) != -1);

    while (printf("%s", prefix), output_str = fgets(read_buffer,
           DEFAULT_STDIN_BUFFER_SIZE, stdin), !feof(stdin)) {
        if (!output_str) {
            std::cerr << "fgets() failed\n" << std::endl;
            break;
        }
        msg_length = strlen(read_buffer);
        if (msg_length > 1) {
            strncpy(send_message.content, read_buffer, MESSAGE_LEN);
            assert(send(broker_fd, &send_message, sizeof(Message), 0) != -1);
        }
    }
}

int main() { 
    int broker_fd = connect_to_broker();

    process_broker_messages(broker_fd);
    close(broker_fd);
    return 0;
}