#ifndef HANDLE_MESSAGE_H_INCLUDED
#define HANDLE_MESSAGE_H_INCLUDED

#include"dns_server.h"

void handle_server_message(char* buf, int length, struct sockaddr_in server_addr);  // �������Է������ı���
void handle_client_message(char* buf, int length, struct sockaddr_in client_addr);  // �������Կͻ��˵ı���

#endif // HANDLE_MESSAGE_H_INCLUDED
