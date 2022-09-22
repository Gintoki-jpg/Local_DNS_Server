#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "list.h"

// 链表的结点
typedef struct Node {
	struct list_head list;
	char url[200];
	char ip[50];  //  ip地址
	time_t expireTime;
} Node;

#define BRANCH 192

// 字典树结点
typedef struct TrieNode
{
	union
	{
		struct Node* lf;
		struct TrieNode* ptr[BRANCH];
	};
} TrieNode;


// 宏定义cache链表的大小
#define CACHE_MAX_SIZE 256

// cache中数据若超过ttl时间仍未被访问，则移除该数据
static uint32_t ttl = 500;
// table中数据若超过ttl2时间仍未被访问，且不是静态表中数据，则移除该数据以释放内存
static uint32_t ttl2 = 20000;
static uint32_t flashTime = 10000;  // 每隔flashTime，扫描一次table，找到超时数据并删除
static uint32_t lastFlashTime;  // 上次刷新的时间
// cache缓存链表与DNS的url与IP的对照表
struct Node* cache, * table;
// cache与table对应的字典查询树
struct TrieNode* tableTrie, *cacheTrie;
struct list_head* cache_LRU_data;  // cache最应该被移除的数据

// cache现在的大小
static int cache_size;

// 初始化“域名-IP 地址”对照表table
void initTable();

// 向table中加入数据 type为1时，将加入的数据同步到cache中，为2时，意为是初始化
void addDataToTable(char* url, char* ip, int type);

// 向字典树中增加结点，type为0是往table字典树中添加；为1是往cache字典树中添加
void addDataToTrie(char* url, int type, struct Node* lf);

// 在字典树中查询 type为0是在table字典树中查询；为1是在cache字典树中查询
struct Node* findNodeInTrie(char* url, int type);

// 递归的删除字典树
void delTrie(TrieNode* p);

// 在字典树中删除结点 type为0是在table字典树中删除；为1是在cache字典树中删除
void delNodeInTrie(char* url, int type);

// 向cache中加入数据
void addDataToCache(char* url, char* ip);

// 在DNS中继服务器中获取数据，若返回0表示DNS中继服务器中无相应数据，应向因特网DNS服务器发出查询,返回2表示在cache中查到
int getData(char* url, char** ip);

// 在cache中根据URL获取数据，若返回0表示cache中无相应数据，应查询table
int getDataInCache(char* url, char** ip);

// 清除table中长时间未访问的数据以释放内存
void flashTable();

// 删除table
void deleteTable();

// 删除cache
void deleteCache();

// 打印链表（用于debug）0 打印table，1 打印cache
void printList(int type);

