#ifndef DNS_SERVER_H_INCLUDED
#define DNS_SERVER_H_INCLUDED

#include <time.h>
#include"public.h"
#include"DNS_table.h"

typedef struct
{
    unsigned short client_id;          //�ͻ��˷�������DNS���������������е�ID
    struct sockaddr_in client_addr;            //�ͻ��˵�ַ�ṹ��
    int survival_time;                  //id����ʱ��
    int finished;                      //���β�ѯ�����Ƿ����
}id_table;                              //id��(�ṹ��)

void initialize_id_table();                                                                //��ʼ��id��
void initialize_socket();                                                                  //��ʼ���׽�����Ϣ
void insert_to_inter();                                                                    //��url-ipӳ������ڴ�
void insert_to_cache(char* url, char *ip);                                                //��url-ipӳ�����cache
void show_message(char* buf, int len);                                                    //չʾ����
void handle_arguments(int argc, char* argv[]);                                            //�����û�shell����
unsigned short id_transform(unsigned short ID, struct sockaddr_in client_addr, int finished);  //���ͻ���idת��Ϊ������id
void url_transform(char* buf, char* result);                                              //���ַ���������ʾ��urlת��Ϊ����url
void print_debug_time();                                                                   //���debugʱ��

#endif // DNS_SERVER_H_INCLUDED
