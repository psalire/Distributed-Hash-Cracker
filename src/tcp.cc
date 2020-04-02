
#include "tcp.h"

#define RUNTIME_STRERR std::runtime_error(std::string(strerror(errno))+" at "+std::string(__FILE__)+":"+std::to_string(__LINE__))

/* Constructors */
TCPComm::TCPComm() {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw RUNTIME_STRERR;
    }
    being_copied = false;
}
// TCPComm::TCPComm(TCPComm &&other) {
    // other.being_copied = true;
    // sockfd = other.sockfd;
    // being_copied = false;
// }
// /* Destructor */
// TCPComm::~TCPComm() {
    // /* Don't close() if sockfd still being used */
    // if (!being_copied) {
        // close(sockfd);
    // }
// }
// /* Assignment Operator */
// TCPComm &TCPComm::operator=(TCPComm &&other) {
    // other.being_copied = true;
    // sockfd = other.sockfd;
    // being_copied = false;
    // return *this;
// }

/* Methods */
void TCPComm::verify_sockfd() {
    if (sockfd < 0) {
        throw std::runtime_error("sockfd < 0");
    }
}
void TCPComm::verify_sockfd(int fd) {
    if (fd < 0) {
        throw std::runtime_error("sockfd < 0");
    }
}
void TCPComm::bind_socket(int port) {
    verify_sockfd();
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        throw RUNTIME_STRERR;
    }
}
void TCPComm::listen_socket(int num) {
    try {
        verify_sockfd();
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    if (listen(sockfd, num) < 0) {
        throw RUNTIME_STRERR;
    }
}
int TCPComm::accept_connection() {
    try {
        verify_sockfd();
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    memset(&client_addr, 0, sizeof(client_addr));
    memset(&client_addr_size, 0, sizeof(client_addr_size));
    int ret = accept(sockfd, (struct sockaddr *) &client_addr, &client_addr_size);
    if (ret < 0) {
        throw RUNTIME_STRERR;
    }
    return ret;
}
void TCPComm::connect_socket(const char *ip_addr, int port) {
    try {
        verify_sockfd();
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip_addr);
    addr.sin_port = htons(port);
    /* connect(), return error if fail */
    if (connect(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        throw RUNTIME_STRERR;
    }
}
void TCPComm::send_message(int fd, void *msg, int msg_len) {
    try {
        verify_sockfd();
        verify_sockfd(fd);
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    int tot_bytes_sent = 0;
    /* Send all bytes */
    while (tot_bytes_sent < msg_len) {
        int bytes_sent = send(fd, &((char *) msg)[tot_bytes_sent], msg_len - tot_bytes_sent, 0);
        if (bytes_sent < 0) {
            throw RUNTIME_STRERR;
        }
        tot_bytes_sent += bytes_sent;
    }
}
void TCPComm::send_message(void *msg, int msg_len) {
    try {
        verify_sockfd();
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    int tot_bytes_sent = 0;
    /* Send all bytes */
    while (tot_bytes_sent < msg_len) {
        int bytes_sent = send(sockfd, &((char *) msg)[tot_bytes_sent], msg_len - tot_bytes_sent, 0);
        if (bytes_sent < 0) {
            throw RUNTIME_STRERR;
        }
        tot_bytes_sent += bytes_sent;
    }
}
bool TCPComm::recv_message(int fd, void *buf, int msg_len) {
    try {
        verify_sockfd();
        verify_sockfd(fd);
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    memset(buf, 0, msg_len);
    int tot_bytes_recvd = 0;
    /* Recv all bytes */
    while (tot_bytes_recvd < msg_len) {
        int bytes_recvd = recv(fd, &((char *) buf)[tot_bytes_recvd], msg_len - tot_bytes_recvd, 0);
        if (bytes_recvd < 0) {
            throw RUNTIME_STRERR;
        }
        else if (bytes_recvd == 0) {
            /* If bytes_recvd==0, connection ended */
            return false;
        }
        tot_bytes_recvd += bytes_recvd;
    }
    return true;
}
bool TCPComm::recv_message(void *buf, int msg_len) {
    try {
        verify_sockfd();
    }
    catch (std::exception &e) {
        throw RUNTIME_STRERR;
    }
    memset(buf, 0, msg_len);
    int tot_bytes_recvd = 0;
    /* Recv all bytes */
    while (tot_bytes_recvd < msg_len) {
        int bytes_recvd = recv(sockfd, &((char *) buf)[tot_bytes_recvd], msg_len - tot_bytes_recvd, 0);
        if (bytes_recvd < 0) {
            throw RUNTIME_STRERR;
        }
        else if (bytes_recvd == 0) {
            /* If bytes_recvd==0, connection ended */
            return false;
        }
        tot_bytes_recvd += bytes_recvd;
    }
    return true;
}
int TCPComm::get_sockfd() {
    return sockfd;
}
void TCPComm::close_socket() {
    close(sockfd);
    sockfd = -1;
}
