#include "sql_operation.h"

sqlOp::sqlOp(){
    conn = mysql_init(NULL);
    if (NULL == mysql_real_connect(conn, "127.0.0.1", "root", "abcd1234abcd","test",3306,0,0)){
        std::cout << "connect error" << std::endl;
    }
}

bool sqlOp::Insert(char* username, char* password){
    // char sql[255] = NULL;
    // sprintf(sql, "%s", "INSERT INTO user(uername, password) VALUES('%s')"
    char buf[255] = {0};
    string sql = "INSERT INTO user(username, password) VALUES('";
    sql += username;
    sql += "','";
    sql += password;
    sql+= "');";

    // cout << sql << endl;
    
    // if (mysql_query(conn, sql.c_str()) != 0){
    //      printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    // }

    return mysql_query(conn, sql.c_str()) == 0;
}

char* sqlOp::Select(char* username){
    string sql = "select password from user where username='";
    sql += username;
    sql += "';";

    int ret = mysql_query(conn, sql.c_str());
    if(ret){
        printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
        return NULL;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if(res != NULL){
        MYSQL_ROW row = mysql_fetch_row(res);
        if(row != NULL){
            return row[0];
        }
    }

    return NULL;
}

sqlOp::~sqlOp(){
    mysql_close(conn);
}

// int main(){
//     sqlOp obj;
//     bool tag = obj.Insert("11", "222");
//     std::cout << obj.Select("11") << endl;
//     return 0;
// }