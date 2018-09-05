#include "conn.h"

int HttpConn::m_epollfd = -1;
int HttpsConn::m_epollfd = -1;
HttpConn::HttpConn(int sock){
    m_sock = sock;
}

int get_line(int m_sock, char* buf, int size, SSL* ssl, bool ishttps){
    char c;
    int i = 0;
    int n = 0;
    do{
        if(!ishttps){
            n = recv(m_sock, &c, 1, 0);
        }else{
            n = SSL_read(ssl, &c, 1); 
            // if(n < 0){
            //    printf("n=%d, c=%c\n", n, c);
            //     sleep(1);
            // }

            if(n < 0){
                perror("SSL_read error");
                printf("error=%d\n", SSL_get_error(ssl, n));
                sleep(1); 
            }
        }

        if(n < 0)continue;
        if(n == 0){
            //close_conn();
            break;
        }

        if (c == '\r'){
             if(!ishttps){
                n = recv(m_sock, &c, 1, MSG_PEEK);
            }else{
                n= SSL_peek(ssl, &c, 1); 
            }
            if(n > 0 && c == '\n'){
                if(!ishttps){
                    n = recv(m_sock, &c, 1, 0);
                }else{
                    n= SSL_read(ssl, &c, 1); 
                }
            }else{
                c = '\n';
            }
        }
        buf[i++] = c;
    }while(i < size - 1 && c != '\n');

    buf[i] = '\0';
    return i;
}

void clearHead(int m_sock, SSL* ssl, bool ishttps){
    char buf[SIZE];
    int n = 0;
   
    do{
         n = get_line(m_sock, buf, SIZE, ssl, ishttps);
//         printf("buf:%s", buf);
    }while(n > 0 && strcmp(buf, "\n") != 0);
}

void serve_static(int sock, SSL* ssl, char* method, char* path, int size, bool ishttps){
    printf("showPage path=%s\n", path);
    char buff[SIZE] = {0};
    sprintf(buff, "%s", "HTTP/1.0 200 OK\r\n\r\n");
    if(strcasecmp(method, "delete") == 0){
        char command[255];
        char cwd[255]={0};
        getcwd(cwd, 255);
        strcat(cwd, "/");
        strcat(cwd, path);
        sprintf(command, "rm -rf %s", cwd);
        printf("c=%s\n", command);
        pid_t pid = fork();
        if(pid == 0){
            //execl(command, command, NULL);
            system(command);
            if(ishttps){
                SSL_write(ssl, buff, strlen(buff));
            }else{
                send(sock, buff, strlen(buff),0);
            }
            close(sock);
        }        
        return ;
    }

    int fd = open(path, O_RDONLY);
    if(ishttps){
        SSL_write(ssl, buff, strlen(buff));
        void* srcp = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        SSL_write(ssl, srcp, size);
        munmap(srcp, size);
    }else{
        send(sock, buff, strlen(buff),0);
        if(sendfile(sock, fd, 0, size) < 0){
            perror("sendfile error");
        }
    }

    close(fd);
    sleep(1);
}

void cannot_execute(int client, SSL* ssl, bool ishttps)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 500 Internal Server Error\r\n");
 //   send(client, buf, strlen(buf), 0);
    strcat(buf, "Content-type: text/html\r\n");
 //   send(client, buf, strlen(buf), 0);
    strcat(buf, "\r\n");
 //   send(client, buf, strlen(buf), 0);
    strcat(buf, "<P>Error prohibited CGI execution.\r\n");
    if(ishttps){
        SSL_write(ssl, buf, strlen(buf));
    }else{
        send(client, buf, strlen(buf), 0);
    }
}

void bad_request(int client, SSL* ssl, bool ishttps)
{
    char buf[1024];

    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
  //  send(client, buf, sizeof(buf), 0);
    strcat(buf, "Content-type: text/html\r\n");
  //  send(client, buf, sizeof(buf), 0);
    strcat(buf, "\r\n");
  //  send(client, buf, sizeof(buf), 0);
    strcat(buf, "<P>Your browser sent a bad request, ");
  //  send(client, buf, sizeof(buf), 0);
    strcat(buf, "such as a POST without a Content-Length.\r\n");
    if(ishttps){
        SSL_write(ssl, buf, strlen(buf));
    }else{
        send(client, buf, strlen(buf), 0);
    }

}

void not_found(int client, SSL* ssl, bool ishttps){
    char buf[1024];

    sprintf(buf, "HTTP/1.0 404 Page Not Found\r\n");
   // send(client, buf, strlen(buf), 0);
    strcat(buf, "Content-Type: text/html\r\n");
    //send(client, buf, strlen(buf), 0);
    strcat(buf, "\r\n");
   // send(client, buf, strlen(buf), 0);
    strcat(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
   // send(client, buf, strlen(buf), 0);
    strcat(buf, "<BODY><P>The server could not fulfill\r\n");
  //  send(client, buf, strlen(buf), 0);
    strcat(buf, "your request because the resource specified\r\n");
  //  send(client, buf, strlen(buf), 0);
    strcat(buf, "is unavailable or nonexistent.\r\n");
  //  send(client, buf, strlen(buf), 0);
    strcat(buf, "</BODY></HTML>\r\n");
   // send(client, buf, strlen(buf), 0);
    if(ishttps){
        SSL_write(ssl, buf, strlen(buf));
    }else{
        send(client, buf, strlen(buf), 0);
    }  
}

void unimplemented(int client, SSL* ssl, bool ishttps)
{
    char buf[1024] = {0};

    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n");
    
    strcat(buf, "Content-Type: text/html\r\n");
   
    strcat(buf, "\r\n");
    
    strcat(buf, "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
    
    strcat(buf, "</TITLE></HEAD>\r\n");
    
    strcat(buf, "<BODY><P>HTTP request method not supported.\r\n");
    
    strcat(buf, "</BODY></HTML>\r\n");

    if(ishttps){
        SSL_write(ssl, buf, strlen(buf));
    }else{
        send(client, buf, strlen(buf), 0);
    }   
}

void excute_cgi(int m_sock, SSL* ssl, char* method, char* path, char *query_string, bool ishttps){
    char buf[SIZE];
    char c;
    int content_length = -1;
    char query_env[255];
    char method_env[255];
    char length_env[255];
    int n = 0;

    if(strcasecmp("get", method) == 0 || strcasecmp("delete", method) == 0){
         clearHead(m_sock, ssl, ishttps);
    }else if(strcasecmp("post", method)==0 || strcasecmp("put", method) == 0){
        do{
            n = get_line(m_sock, buf, SIZE,ssl, ishttps);
            buf[15] = 0;
            if(strcmp("Content-Length:", buf) == 0){
                content_length = atoi(&buf[16]);
            }
        }while(n > 0 && strcmp("\n", buf) != 0);
    }
    if((strcasecmp("post", method)==0 || strcasecmp("put", method)==0)&& content_length == -1){
        bad_request(m_sock, ssl, ishttps);
        return ;
    }

   // printf("query_string=%s\n", query_string);
    int cgi_input[2];
    int cgi_output[2];

    if (pipe(cgi_output) < 0) {
        cannot_execute(m_sock, ssl, ishttps);
        return;
    }
    if (pipe(cgi_input) < 0) {
        cannot_execute(m_sock, ssl, ishttps);
        return;
    }

    int pid = -1;
    //创建一个子进程
    if ( (pid = fork()) < 0 ) {
        cannot_execute(m_sock, ssl, ishttps);
        return;
    }
 
    //子进程用来执行 cgi 脚本
    if (pid == 0)  /* child: CGI script */
    {
    char meth_env[255];
    char query_env[255];
    char length_env[255];

    //dup2()包含<unistd.h>中，参读《TLPI》P97
    //将子进程的输出由标准输出重定向到 cgi_ouput 的管道写端上
    dup2(cgi_output[1], 1);
    //将子进程的输出由标准输入重定向到 cgi_ouput 的管道读端上
    dup2(cgi_input[0], 0);
    //关闭 cgi_ouput 管道的读端与cgi_input 管道的写端
    close(cgi_output[0]);
    close(cgi_input[1]);
    
    //构造一个环境变量
    sprintf(meth_env, "REQUEST_METHOD=%s", method);
    //putenv()包含于<stdlib.h>中，参读《TLPI》P128
    //将这个环境变量加进子进程的运行环境中
    putenv(meth_env);
    
    //根据http 请求的不同方法，构造并存储不同的环境变量
    if (strcasecmp(method, "GET") == 0 || strcasecmp(method, "delete") == 0) {
        sprintf(query_env, "QUERY_STRING=%s", query_string);
        putenv(query_env);
    }
    else {   /* POST */
        sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
        putenv(length_env);
    }
    
    //execl()包含于<unistd.h>中，参读《TLPI》P567
    //最后将子进程替换成另一个进程并执行 cgi 脚本
    execl(path, path, NULL);
    exit(0);
    
    } else {    /* parent */
    //父进程则关闭了 cgi_output管道的写端和 cgi_input 管道的读端
    close(cgi_output[1]);
    close(cgi_input[0]);
    
    //如果是 POST 方法的话就继续读 body 的内容，并写到 cgi_input 管道里让子进程去读
    if (strcasecmp(method, "POST") == 0 || strcasecmp(method, "PUT") == 0)
    for (int i = 0; i < content_length; i++) {
        if(!ishttps){
            recv(m_sock, &c, 1, 0);
        }else{
            SSL_read(ssl, &c, 1); 
        }
        write(cgi_input[1], &c, 1);
    }
    
    //然后从 cgi_output 管道中读子进程的输出，并发送到客户端去
    while (read(cgi_output[0], &c, 1) > 0){
        if(!ishttps){
            send(m_sock, &c, 1, 0);
        }else{
            SSL_write(ssl, &c, 1);
        }
          
    }
  

    //关闭管道
    close(cgi_output[0]);
    close(cgi_input[1]);
    //等待子进程的退出
    int status = 0;
    waitpid(pid, &status, 0);
    }
}

void parse_request_line(int sock, char* buf, int size, SSL* ssl, bool ishttps){
    char method[255];
    char url[255];
    char path[255];
    char version[255];
    char* query_string = NULL;
    int content_length = -1;
    bool cgi = false;
    int i = 0;
    int j = 0;

    while(buf[i] && !isspace(buf[i])){
        method[j++] = buf[i++];
    }
    method[j] = 0;
    if(strcasecmp(method, "post") != 0 && strcasecmp(method, "get") != 0 \
     && strcasecmp(method, "delete") != 0 && strcasecmp(method, "put") != 0){
        unimplemented(sock, ssl, ishttps);
        close(sock);
        return;
    }
    if(strcasecmp("post", method) == 0 || strcasecmp(method, "put") == 0){
        cgi = true;
    }

    ++i;
    j = 0;
    while(buf[i] && !isspace(buf[i])){
        url[j++] = buf[i++];
    }
    url[j] = 0;

    if(strcasecmp("get", method) == 0 || strcasecmp(method, "delete") == 0){
        query_string = url;
        while(*query_string){
            if(*query_string == '?'){
                break;
            }
            ++query_string;
        }
    }
    if(query_string != NULL && *query_string == '?'){
        cgi = 1;
        *query_string = 0;
        ++query_string;
    }

    ++i;
    j = 0;
    while(buf[i] != '\n' && !isspace(buf[i])){
        version[j++] = buf[i++];
    }

     sprintf(path, "wwwRoot%s", url);
     if (path[strlen(path) - 1] == '/')
        strcat(path, "index.html");

    struct stat st;
    if(stat(path, &st) < 0){
        clearHead(sock, ssl, ishttps);
        not_found(sock, ssl, ishttps);
        close(sock);
        return;
    }


    if(st.st_mode & S_IFMT == S_IFDIR)  strcat(path, "/index.html");
    if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) ||(st.st_mode & S_IXOTH)){
        cgi = true;
    }

    if(cgi){
        excute_cgi(sock, ssl, method, path, query_string, ishttps);
    }else{
        clearHead(sock, ssl, ishttps);
        serve_static(sock, ssl, method, path, st.st_size, ishttps);
    }
    close(sock);   
}

void HttpConn::process(){
    char buf[SIZE] = {0};
    get_line(m_sock,  buf, sizeof(buf), NULL,false);
    parse_request_line(m_sock, buf, sizeof(buf), NULL, false);
}


SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
	    exit(EXIT_FAILURE);
    }
}

SSL* HttpsConn::ssl = NULL;

void HttpsConn::init_cert(){
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
    SSL_CTX* ctx = create_context();
    configure_context(ctx);
    ssl = SSL_new(ctx);
}

HttpsConn::HttpsConn(int sock){
    m_sock = sock;
    // SSL_set_fd(ssl, m_sock);
    // if (SSL_accept(ssl) <= 0) {
    //     perror("SSL_accept error");
    //     ERR_print_errors_fp(stderr);
    // }
}

void HttpsConn::process(){
    char buf[SIZE] = {0};
    get_line(m_sock, buf, sizeof(buf), ssl, true);
    parse_request_line(m_sock, buf, sizeof(buf), ssl, true);
}