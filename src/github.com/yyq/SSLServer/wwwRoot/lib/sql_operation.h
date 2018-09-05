#ifndef _SQL_OPERATION_H
#define _SQL_OPERATION_H

#include <mysql.h>
#include <string>
#include <string.h>
#include <iostream>
using namespace std;

struct sqlOp{
    sqlOp();
    bool Insert(char* username, char* password);
    char* Select(char* username);
    ~sqlOp();

    MYSQL* conn;
};

#endif