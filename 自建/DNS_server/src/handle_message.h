#ifndef HANDLE_MESSAGE_H_INCLUDED
#define HANDLE_MESSAGE_H_INCLUDED

#include"dns_server.h"

void handle_server_message(char* buf, int length, struct sockaddr_in server_addr);  // 处理来自服务器的报文
void handle_client_message(char* buf, int length, struct sockaddr_in client_addr);  // 处理来自客户端的报文

#endif // HANDLE_MESSAGE_H_INCLUDED
