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

#define BRANCH 192

// �ֵ������
typedef struct TrieNode
{
	union
	{
		struct Node* lf;
		struct TrieNode* ptr[BRANCH];
	};
} TrieNode;


// �궨��cache����Ĵ�С
#define CACHE_MAX_SIZE 256

// cache������������ttlʱ����δ�����ʣ����Ƴ�������
static uint32_t ttl = 500;
// table������������ttl2ʱ����δ�����ʣ��Ҳ��Ǿ�̬�������ݣ����Ƴ����������ͷ��ڴ�
static uint32_t ttl2 = 20000;
static uint32_t flashTime = 10000;  // ÿ��flashTime��ɨ��һ��table���ҵ���ʱ���ݲ�ɾ��
static uint32_t lastFlashTime;  // �ϴ�ˢ�µ�ʱ��
// cache����������DNS��url��IP�Ķ��ձ�
struct Node* cache, * table;
// cache��table��Ӧ���ֵ��ѯ��
struct TrieNode* tableTrie, *cacheTrie;
struct list_head* cache_LRU_data;  // cache��Ӧ�ñ��Ƴ�������

// cache���ڵĴ�С
static int cache_size;

// ��ʼ��������-IP ��ַ�����ձ�table
void initTable();

// ��table�м������� typeΪ1ʱ�������������ͬ����cache�У�Ϊ2ʱ����Ϊ�ǳ�ʼ��
void addDataToTable(char* url, char* ip, int type);

// ���ֵ��������ӽ�㣬typeΪ0����table�ֵ�������ӣ�Ϊ1����cache�ֵ��������
void addDataToTrie(char* url, int type, struct Node* lf);

// ���ֵ����в�ѯ typeΪ0����table�ֵ����в�ѯ��Ϊ1����cache�ֵ����в�ѯ
struct Node* findNodeInTrie(char* url, int type);

// �ݹ��ɾ���ֵ���
void delTrie(TrieNode* p);

// ���ֵ�����ɾ����� typeΪ0����table�ֵ�����ɾ����Ϊ1����cache�ֵ�����ɾ��
void delNodeInTrie(char* url, int type);

// ��cache�м�������
void addDataToCache(char* url, char* ip);

// ��DNS�м̷������л�ȡ���ݣ�������0��ʾDNS�м̷�����������Ӧ���ݣ�Ӧ��������DNS������������ѯ,����2��ʾ��cache�в鵽
int getData(char* url, char** ip);

// ��cache�и���URL��ȡ���ݣ�������0��ʾcache������Ӧ���ݣ�Ӧ��ѯtable
int getDataInCache(char* url, char** ip);

// ���table�г�ʱ��δ���ʵ��������ͷ��ڴ�
void flashTable();

// ɾ��table
void deleteTable();

// ɾ��cache
void deleteCache();

// ��ӡ��������debug��0 ��ӡtable��1 ��ӡcache
void printList(int type);

