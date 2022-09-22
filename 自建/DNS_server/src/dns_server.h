#ifndef DNS_SERVER_H_INCLUDED
#define DNS_SERVER_H_INCLUDED

#include <time.h>
#include"public.h"
#include"DNS_table.h"

typedef struct
{
	unsigned short client_id;  // 客户端发给本地DNS服务器的请求报文中的ID          
	struct sockaddr_in client_addr;  // 客户端地址结构体
	int survival_time;  // id表存活时间
	int finished;  // 本次查询请求是否完成
}id_table;  // id表(结构体)

// 初始化id表
void initialize_id_table();
// 初始化套接字信息
void initialize_socket();
// 将url-ip映射存入内存
void insert_to_inter();
// 将url-ip映射存入cache
void insert_to_cache(char* url, char* ip);
// 展示报文
void show_message(char* buf, int len);
// 处理用户shell输入
void handle_arguments(int argc, char* argv[]);
// 将客户端id转换为服务器id
unsigned short id_transform(unsigned short ID, struct sockaddr_in client_addr, int finished);
// 将字符计数法表示的url转换为正常url
void url_transform(char* buf, char* result);
// 输出debug时间
void print_debug_time();

#endif 
