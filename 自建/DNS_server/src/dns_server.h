#ifndef DNS_SERVER_H_INCLUDED
#define DNS_SERVER_H_INCLUDED

#include <time.h>
#include"public.h"
#include"DNS_table.h"

typedef struct
{
	unsigned short client_id;  // �ͻ��˷�������DNS���������������е�ID          
	struct sockaddr_in client_addr;  // �ͻ��˵�ַ�ṹ��
	int survival_time;  // id����ʱ��
	int finished;  // ���β�ѯ�����Ƿ����
}id_table;  // id��(�ṹ��)

// ��ʼ��id��
void initialize_id_table();
// ��ʼ���׽�����Ϣ
void initialize_socket();
// ��url-ipӳ������ڴ�
void insert_to_inter();
// ��url-ipӳ�����cache
void insert_to_cache(char* url, char* ip);
// չʾ����
void show_message(char* buf, int len);
// �����û�shell����
void handle_arguments(int argc, char* argv[]);
// ���ͻ���idת��Ϊ������id
unsigned short id_transform(unsigned short ID, struct sockaddr_in client_addr, int finished);
// ���ַ���������ʾ��urlת��Ϊ����url
void url_transform(char* buf, char* result);
// ���debugʱ��
void print_debug_time();

#endif 
