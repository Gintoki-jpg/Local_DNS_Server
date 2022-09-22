#ifndef DNS_SERVER_H_INCLUDED
#define DNS_SERVER_H_INCLUDED

#include <stdbool.h>
#include <time.h>
#include"public.h"
#include"list.h"

typedef struct
{
    unsigned short client_id;          //�ͻ��˷�������DNS���������������е�ID
    SOCKADDR_IN client_addr;            //�ͻ��˵�ַ�ṹ��
    int survival_time;                  //id����ʱ��
    BOOL finished;                      //���β�ѯ�����Ƿ����
}id_table;                              //id��(�ṹ��)

void initialize_id_table();                                                                //��ʼ��id��
void initialize_socket();                                                                  //��ʼ���׽�����Ϣ
void insert_to_inter();                                                                    //��url-ipӳ������ڴ�
void insert_to_cache(char* url, char *ip);                                                //��url-ipӳ�����cache
void show_message(char* buf, int len);                                                    //չʾ����
void show_cache_url_ip();                                                                  //չʾcache�е�url_ipӳ��
void handle_server_message(char *buf,int length,SOCKADDR_IN server_addr);                 //�������Է������ı���
void handle_client_message(char *buf,int length,SOCKADDR_IN client_addr);                 //�������Կͻ��˵ı���
void handle_arguments(int argc, char* argv[]);                                            //�����û�shell����
unsigned short id_transform(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished);  //���ͻ���idת��Ϊ������id
void url_transform(char* buf, char* result);                                              //���ַ���������ʾ��urlת��Ϊ����url

#endif // DNS_SERVER_H_INCLUDED
