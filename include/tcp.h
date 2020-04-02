
#ifndef _TCP_H_
#define _TCP_H_

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>

class TCPComm {
    private:
        int sockfd;
        void verify_sockfd();
        void verify_sockfd(int);
        bool being_copied;
    
    public:
        /* Constructors */
        TCPComm();
        // TCPComm(TCPComm &&);
        /* Destructor */
        // ~TCPComm();
        
        /* Assignment operator */
        // TCPComm &operator=(TCPComm &&);
        
        /* Methods */
        int get_sockfd();
        void bind_socket(int);
        void listen_socket(int);
        int accept_connection();
        void connect_socket(const char *, int);
        void send_message(void *, int);
        void send_message(int, void *, int);
        bool recv_message(void *, int);
        bool recv_message(int, void *, int);
        void close_socket();
};

#endif
