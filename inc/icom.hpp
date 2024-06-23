#ifndef ICOM_H
#define ICOM_H

#define VERSION "2.0.0dev1"
#define DURATION_MARKER 5
#define SLEEP_AFTER_SEQUENCE 1000
#define DEFAULT_PORT 49152
#define DEFAULT_LENGTH_HEADER 64
#define DEFAULT_LENGTH_CHUNK 4096

#include <netinet/in.h>
#include <string>
//#include <sys/socket.h>
//#include <sys/types.h>

namespace icom {
    class server {
        public:
            server(int port, unsigned int length_header, unsigned int length_chunk);
            void start();
            void send(std::string s);
            char** recv();
            void close();
            int get_n_conns();
        private:
            unsigned int length_header;
            unsigned int length_chunk;
            int port;
            int sockfd;
            int* conns;
            int n_conns = 0;
            //socklen_t clilen;
            //struct sockaddr_in serv_addr, cli_addr;
            void server_thread();

    };
    
    class client {
        public:
            client(std::string ip, int port, unsigned int length_header, unsigned int length_chunk);
            int connect();
            void send(std::string s);
            char* recv();
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