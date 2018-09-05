#ifndef _CONN_H
#define _CONN_H

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


const int MAX_EVENT_NUMBER = 10000;
const int SIZE = 1024;

// struct Argument{
//     char* method;
//     char* url;
//     char* require_string;
//     int content_length;
//     char* version;
//     bool cgi;

//     Argument(char* method, char* url, char* require_string, int content_length, char* version, bool cgi){
//         this->method = method;
//         this->url = url;
//         this->require_string = require_string;
//         this->content_length = content_length;
//         this->version = version;
//         this->cgi = cgi;
//     }
// }


class HttpConn{
public:
    HttpConn(int sock);
    void process();
//    int  http_get_line(char* buf, int size);
//    void http_clear_head();
//    void http_exec_cgi(char* method, char* query_string, char* path);
//    void http_serve_static(char* path, int size);
    static int m_epollfd;
private:
    int m_sock;
};

class HttpsConn{
public:
    HttpsConn(int sock);
    void process();
    static void init_cert();
//    int  https_get_line(char* buf, int size);
//    void https_clear_head();
//    void http_exec_cgi(char* method, char* query_string, char* path);
//    void http_serve_static(const char* path, int size);
    static int m_epollfd;
    static SSL *ssl;
private:
    int m_sock;
};

#endif