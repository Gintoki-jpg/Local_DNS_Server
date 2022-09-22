#ifndef DNS_SERVER_H_INCLUDED
#define DNS_SERVER_H_INCLUDED

#include <stdbool.h>
#include <time.h>
#include"public.h"
#include"list.h"

typedef struct
{
    unsigned short client_id;          //客户端发给本地DNS服务器的请求报文中的ID
    SOCKADDR_IN client_addr;            //客户端地址结构体
    int survival_time;                  //id表存活时间
    BOOL finished;                      //本次查询请求是否完成
}id_table;                              //id表(结构体)

void initialize_id_table();                                                                //初始化id表
void initialize_socket();                                                                  //初始化套接字信息
void insert_to_inter();                                                                    //将url-ip映射存入内存
void insert_to_cache(char* url, char *ip);                                                //将url-ip映射存入cache
void show_message(char* buf, int len);                                                    //展示报文
void show_cache_url_ip();                                                                  //展示cache中的url_ip映射
void handle_server_message(char *buf,int length,SOCKADDR_IN server_addr);                 //处理来自服务器的报文
void handle_client_message(char *buf,int length,SOCKADDR_IN client_addr);                 //处理来自客户端的报文
void handle_arguments(int argc, char* argv[]);                                            //处理用户shell输入
unsigned short id_transform(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished);  //将客户端id转换为服务器id
void url_transform(char* buf, char* result);                                              //将字符计数法表示的url转换为正常url

#endif // DNS_SERVER_H_INCLUDED
