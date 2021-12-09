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

/* Reads messages from stdin and sends to a broker via `broker_socket` fd
 */
void process_broker_messages(int broker_fd) {
    // Always output an interactive marker at the start of each command if the
    // input is from stdin. Do not output if piped in from file or from other fd
    char* prefix = "";
    if (isatty(fileno(stdin))) {
        prefix = "publisher > ";
    }

    Message send_message;
    char* output_str = NULL;
    int msg_length = 0;

    // Continuously loop and wait for input. At each iteration:
    // 1. output interactive marker
    // 2. read from stdin until eof.
    char read_buffer[DEFAULT_STDIN_BUFFER_SIZE];

    while (printf("%s", prefix), output_str = fgets(read_buffer,
           DEFAULT_STDIN_BUFFER_SIZE, stdin), !feof(stdin)) {
        if (!output_str) {
            std::cerr << "fgets() failed\n" << std::endl;
            break;
        }
        msg_length = strlen(read_buffer);
        if (msg_length > 1) {
            strncpy(send_message.content, read_buffer, MESSAGE_LEN);
            assert(send(broker_fd, send_message.content, MESSAGE_LEN, 0) != -1);
        }
    }
}

int main() { 
    int broker_fd;
    broker_fd = connect_to_broker();

    process_broker_messages(broker_fd);
    close(broker_fd);
    return 0;
}