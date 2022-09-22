#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"

// ����Ľ��
typedef struct Node {
    struct list_head list;
    char url[200];
    char ip[50];  //  ip��ַ
    time_t expireTime;
} Node;

// �궨��cache����Ĵ�С
#define CACHE_MAX_SIZE 256

// cache������������ttlʱ����δ�����ʣ����Ƴ�������
static uint32_t ttl = 6000;

// cache����������DNS��url��IP�Ķ��ձ�
struct Node* cache, * table;
struct list_head* cache_LRU_data;  // cache��Ӧ�ñ��Ƴ�������

// cache���ڵĴ�С
static int cache_size;

// ��ʼ��������-IP ��ַ�����ձ�table
void initTable();

// ��table�м������� typeΪ1ʱ�������������ͬ����cache��
void addDataToTable(char* url, char* ip, int type);


// ��cache�м�������
void addDataToCache(char* url, char* ip);

// ��DNS�м̷������л�ȡ���ݣ�������0��ʾDNS�м̷�����������Ӧ���ݣ�Ӧ��������DNS������������ѯ,����2��ʾ��cache�в鵽
int getData(char* url, char** ip);

// ��cache�и���URL��ȡ���ݣ�������0��ʾcache������Ӧ���ݣ�Ӧ��ѯtable
int getDataInCache(char* url, char** ip);

// ɾ��table
void deleteTable();

// ɾ��cache
void deleteCache();

// ��ӡ��������debug��0 ��ӡtable��1 ��ӡcache
void printList(int type);

