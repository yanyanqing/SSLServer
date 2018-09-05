#include "threadpool.h"
#include "conn.h"




int setnonblocking(int sock){
    int op;
    if((op = fcntl(sock, F_GETFL)) < 0){
        perror("fcntl get error");
        exit(-1) ;
    }

    int newop = op | O_NONBLOCK;
    if(fcntl(sock, F_SETFL, newop) < 0){
        perror("fcntl set error");
        exit(-1) ;
    }

    return op;
}

void addfd(int epollfd, int fd, bool one_shot){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;

    if(one_shot){
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
 }

int StartUp(const char *ip, int port){
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr=inet_addr(ip);
    int sock = socket(PF_INET, SOCK_STREAM,0);
    assert(sock != -1);
    
    int reuse=1; 
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    
    socklen_t len  = sizeof(server_addr);
    if(bind(sock, (struct sockaddr*)&server_addr, len) < 0){
        printf("bind error");
        return -1;
    }

    if(listen(sock, 5) < 0){
        printf("listen error");
        return -1;
    }

    return sock;
}



int main(int argc, char *argv[]){
  // int socket = StartUp(argv[1], atoi(argv[2]));
    HttpsConn::init_cert();
    pid_t pid = fork();
    if(pid < 0){
        perror("fork error");
        exit(1);
    }
    if(pid == 0){
        ThreadPool<HttpConn>* pool = new ThreadPool<HttpConn>(8, 1024);    
        //HttpConn *conn = new HttpConn[1024];
        int listenfd = StartUp("127.0.0.1",8888);
        assert(listenfd != -1);
        
        struct epoll_event ev, events[MAX_EVENT_NUMBER];
        ev.data.fd = listenfd;
        ev.events = EPOLLIN | EPOLLET;
        int epollfd = epoll_create(5);
        assert(epollfd != -1);

        HttpConn::m_epollfd = epollfd;
        addfd(epollfd, listenfd, false);
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        while(1){
            int numbers = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

            if(numbers < 0 && errno != EAGAIN){
                perror("epoll_wait error");
                break;
            }

            for(int i = 0; i < numbers; i++){
                int fd = events[i].data.fd;
                if(fd == listenfd){
                    int sock = accept(listenfd, (struct sockaddr*)&client_addr, &len);
                    if(sock < 0){
                        perror("accept error");
                        continue;
                    }
                    printf("http client connects successful %s\n", inet_ntoa(client_addr.sin_addr));
                    addfd(epollfd, sock, true);
                    pool->append(new HttpConn(sock));
                }
            }
        }

        close(epollfd);
        close(listenfd);
    }else{
        ThreadPool<HttpsConn>* pool = new ThreadPool<HttpsConn>(8, 1024);    
        //HttpConn *conn = new HttpConn[1024];
        int listenfd = StartUp("127.0.0.1",4433);
        assert(listenfd != -1);
        
        struct epoll_event ev, events[MAX_EVENT_NUMBER];
        ev.data.fd = listenfd;
        ev.events = EPOLLIN | EPOLLET;
        int epollfd = epoll_create(5);
        assert(epollfd != -1);

        HttpsConn::m_epollfd = epollfd;
        addfd(epollfd, listenfd, false);
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        while(1){
            int numbers = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);

            if(numbers < 0 && errno != EAGAIN){
                perror("epoll_wait error");
                break;
            }

            for(int i = 0; i < numbers; i++){
                int fd = events[i].data.fd;
                if(fd == listenfd){
                    int sock = accept(listenfd, (struct sockaddr*)&client_addr, &len);
                    if(sock < 0){
                        perror("accept error");
                        continue;
                    }
                    
                    SSL_set_fd(HttpsConn::ssl, sock);
                    if (SSL_accept(HttpsConn::ssl) <= 0) {
                        perror("SSL_accept error");
                        ERR_print_errors_fp(stderr);
                    }
                    addfd(epollfd, sock, true);
                    pool->append(new HttpsConn(sock));
                }
            }
        }

        close(epollfd);
        close(listenfd);
    }

    return 0;
}