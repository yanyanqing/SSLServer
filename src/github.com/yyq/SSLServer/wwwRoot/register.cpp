/*************************************************************************
> File Name: hello.c
> Author: FENGXUANMO
> Mail: 763608087@qq.com
> Created Time: Mon 10 Apr 2017 03:56:56 AM PDT
************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include "lib/sql_operation.h"

void Register(char* query_string){
    char *argv[2];
    int i = 0;

    while(*query_string){
        if(*query_string == '='){
            argv[i++] = ++query_string;
        }
        if(*query_string == '&'){
            *query_string = 0;
        }
        ++query_string;
    }

    sqlOp sqlConn;
    if(sqlConn.Insert(argv[0], argv[1])){
        printf("HTTP/1.1 200 OK\r\n\r\n"); 
        printf("<HTML><HEAD> Register successful!\r\n");

        printf("</HEAD>\r\n");
        
        printf("<BODY>Hello Welcome\r\n");

        printf("</BODY></HTML>\r\n");
    }else{
        printf("HTTP/1.1 500 Server Interval Error\r\n\r\n"); 
        printf("<HTML><HEAD> Register Failed!");

        printf("</HEAD>\r\n");
        
        printf("<BODY>Register Failed! Please check you argument\r\n");

        printf("</BODY></HTML>\r\n");
    }
}

int main()
{
    char *method;
    char *p_string;
    int Content_len;
    method=getenv("REQUEST_METHOD");
    if(strcasecmp("GET",method)==0 || strcasecmp("DELETE",method)==0)
    {
        p_string=getenv("QUERY_STRING");   
    }
    else
    {
        char c;
        Content_len=atoi(getenv("CONTENT_LENGTH"));
        p_string=(char*)malloc(Content_len);
        int i=0;
        for(;i<Content_len;i++)
        {
            read(0,&c,1);
            p_string[i]=c;
        }
    }

    Register(p_string);
//  printf("HTTP/1.0 200 OK\r\n");
//  //send(client, buf, strlen(buf), 0);
//  printf("Server: yyq\r\n");

//  printf("Content-Type: text/html\r\n");
 
//  printf("\r\n");
 
//  printf("<HTML><HEAD><TITLE>Hello Here\r\n");

//  printf("</TITLE></HEAD>\r\n");
 
//  printf("<BODY>Hello Welcome\r\n");

//  printf("</BODY></HTML>\r\n");

//    close(0);
    free(p_string);
    return 0;
}
