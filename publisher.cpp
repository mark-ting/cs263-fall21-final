#include "publisher.hpp"
#include <cassert>
#include <iostream>

/* Returns a socket address for the publisher that listens for broker connections
 */
int setup_publisher() {
    int pub_socket;
    size_t len;
    struct sockaddr_un pub;

    // Socket
    pub_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    assert(pub_socket != -1);

    pub.sun_family = AF_UNIX;
    strncpy(pub.sun_path, PUB_PATH, strlen(PUB_PATH) + 1);

    // Bind
    len = strlen(pub.sun_path) + sizeof(pub.sun_family) + 1;
    assert(bind(pub_socket, (struct sockaddr *)&pub, len) != -1);

    // Listen
    assert(listen(pub_socket, 5) != -1);

    return pub_socket;
}

int main() { 
    int pub_socket = setup_publisher();

    struct sockaddr_un remote;
    socklen_t t = sizeof(remote);
    int client_socket = 0;

    client_socket = accept(pub_socket, (struct sockaddr *)&remote, &t);

    std::cout << "Client Socket: " << client_socket << std::endl;

    return 0;
}