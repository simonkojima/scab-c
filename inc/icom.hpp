#ifndef ICOM_H
#define ICOM_H

#include <netinet/in.h>
#include <string>
//#include <sys/socket.h>
//#include <sys/types.h>

#define ICOM_BLOCK 0
#define ICOM_NONBLOCK 1

namespace icom {
    class server {
        public:
            server(int port, unsigned int length_header, unsigned int length_chunk);
            void start();
            int send(std::string s);
            int recv(char* data, unsigned int length_data);
            int recv_nonblock(char* data, unsigned int length_data);
            void wait_for_connection();
            int close();
        private:
            unsigned int length_header;
            unsigned int length_chunk;
            int port;
            int sockfd;
            int conn;

    };
    
    class client {
        public:
            client(std::string ip, int port, unsigned int length_header, unsigned int length_chunk);
            int connect();
            int send(std::string s);
            int recv(char* data, unsigned int length_data);
            int recv_nonblock(char* data, unsigned int length_data);
            void close();
        private:
            std::string ip;
            unsigned int length_header;
            unsigned int length_chunk;
            int port;
            //int sock;
            int conn;
    };
}

#endif