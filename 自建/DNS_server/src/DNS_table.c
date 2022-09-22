#include "DNS_table.h"

// 初始化“域名-IP 地址”对照表table
void initTable() {
	// 为table表头分配空间
	table = (Node*)malloc(sizeof(Node));
	memset(table, 0, sizeof(Node));
	INIT_LIST_HEAD(&table->list);

	// 为cache表头分配空间
	cache = (Node*)malloc(sizeof(Node));
	memset(cache, 0, sizeof(Node));
	INIT_LIST_HEAD(&cache->list);

	// 为字典树表头分配空间
	tableTrie = (TrieNode*)malloc(sizeof(TrieNode));
	memset(tableTrie, 0, sizeof(TrieNode));
	cacheTrie = (TrieNode*)malloc(sizeof(TrieNode));
	memset(cacheTrie, 0, sizeof(TrieNode));
	lastFlashTime = time(NULL);

	cache_size = 0;
	printf("初始化“域名-IP 地址”对照表完成。\n");
}

// 向字典树中增加结点，type为0是往table字典树中添加；为1是往cache字典树中添加
void addDataToTrie(char* url, int type, struct Node* lf) {
	TrieNode* p;
	if (type == 1)	p = cacheTrie;
	else p = tableTrie;
	while (0 != *url)
	{
		if (!p->ptr[*url])
		{
			p->ptr[*url] = (TrieNode*)malloc(sizeof(TrieNode));
			memset(p->ptr[*url], 0, sizeof(TrieNode));
		}
		p = p->ptr[*url];
		url++;
	}
	if (!p->ptr[0])
	{
		p->ptr[0] = (TrieNode*)malloc(sizeof(TrieNode));
		memset(p->ptr[0], 0, sizeof(TrieNode));
	}
	p = p->ptr[0];
	if (!p->lf)
	{
		p->lf = (struct Node*)malloc(sizeof(struct Node));
	}
	p->lf = lf;
}

// 在字典树中查询 type为0是在table字典树中查询；为1是在cache字典树中查询
struct Node* findNodeInTrie(char* url, int type) {
	TrieNode* p;
	if (type == 1)	p = cacheTrie;
	else p = tableTrie;
	while (0 != *url)
	{
		p = p->ptr[*url];
		url++;
		if (!p)
		{
			return NULL;
		}
	}
	p = p->ptr[0];
	if (!p)	return NULL;	
	return p->lf;
}

// 在字典树中删除结点 type为0是在table字典树中删除；为1是在cache字典树中删除
void delNodeInTrie(char* url, int type) {
	TrieNode* p;
	if (type == 1)	p = cacheTrie;
	else p = tableTrie;
	while (0 != *url)
	{
		if (!p)
		{
			return;
		}
		p = p->ptr[*url];
		url++;
	}
	if(p->ptr[0])	free(p->ptr[0]);
}

// 递归的删除字典树
void delTrie(TrieNode* p) {
	if (p->ptr[0])	free(p->ptr[0]);
	for (int i = 1; i < BRANCH; i++) {
		if (p->ptr[i]) {
			delTrie(p->ptr[i]);
			free(p->ptr[i]);
		}
	}
}

// 向cache中加入数据
void addDataToCache(char* url, char* ip) {
	Node* p = (Node*)malloc(sizeof(Node));
	memset(p, 0, sizeof(Node));
	strcpy(p->url, url);
	strcpy(p->ip, ip);
	p->expireTime = time(NULL) + ttl;
	list_add(&p->list, &cache->list);
	addDataToTrie(url, 1, p);
	cache_size++;

	// 如果cache容量溢出，则删除当前cache_list所指向的数据结点
	// cache_list所指向的为未被访问时间最长的结点，详见getDataInCache()
	while (cache_size > CACHE_MAX_SIZE) {
		if (cache_LRU_data == NULL) {
			uint32_t minTime = 0;
			struct list_head* p = NULL, * n = NULL;
			list_for_each_safe(p, n, &cache->list) {
				Node* cacheNode = list_entry(p, Node, list);
				// 如果cache中该数据超时，则在cache中删除该数据
				if (cacheNode->expireTime < time(NULL)) {
					delNodeInTrie(cacheNode->url, 1);
					list_del(p);
					cache_size--;
					free(cacheNode);
				}
				// 始终让cache_list指向最长时间未访问的数据
				else if (minTime > cacheNode->expireTime || minTime == 0) {
					minTime = cacheNode->expireTime;
					cache_LRU_data = &cacheNode->list;
				}
			}
		}
		if (cache_size > CACHE_MAX_SIZE) {
			Node* cacheNode = list_entry(cache_LRU_data, Node, list);
			struct list_head* t = cache_LRU_data;
			delNodeInTrie(cacheNode->url, 1);
			cache_LRU_data = NULL;
			list_del(t);
			cache_size--;
			free(cacheNode);
		}
		
	}
}

// 向table中加入数据 type为1时，将加入的数据同步到cache中，为2时，意为是初始化
void addDataToTable(char* url, char* ip, int type) {
	char tem[100];
	char* temip = tem;
	int resultType = getData(url, &temip);
	if (resultType == 0) {
		Node* p;
		p = (Node*)malloc(sizeof(Node));
		strcpy(p->url, url);
		strcpy(p->ip, ip);
		if (type == 2) {
			p->expireTime = 0;  // 静态数据，永不删除
		}
		else {
			p->expireTime = time(NULL) + ttl2;
		}
		list_add(&p->list, &table->list);
		addDataToTrie(url, 0, p);
	}
	if (resultType == 1 && type == 1)
		addDataToCache(url, ip);

}

// 清除table中长时间未访问的数据以释放内存
void flashTable() {
	if (lastFlashTime + flashTime < time(NULL)) {
		printf("定时刷新table\n");
		struct list_head* p = NULL, * n = NULL;
		int flag = 0;
		list_for_each_safe(p, n, &table->list) {
			Node* tableNode = list_entry(p, Node, list);
			// 如果table中该数据超时，且该数据不是初始化时的静态数据，则在table中删除该数据以释放内存
			if (tableNode->expireTime < time(NULL) && tableNode->expireTime != 0) {
				delNodeInTrie(tableNode->url, 0);
				list_del(p);
				free(tableNode);
			}
		}
	}
}

// 在cache中根据URL获取数据，若返回0表示cache中无相应数据，应查询table
int getDataInCache(char* url, char** ip) {
	Node* cacheNode = findNodeInTrie(url, 1);
	// cache中无相应数据，返回
	if (cacheNode == NULL) return 0;
	
	// 如果获取到相应的数据，则更新cache中数据的expireTime并给出查询到的IP地址
	strcpy(*ip, cacheNode->ip);
	cacheNode->expireTime = time(NULL) + ttl;

	// 更新table中的expireTime
	Node* tableNode = findNodeInTrie(url, 0);
	if (tableNode != NULL && tableNode->expireTime != 0) {
		tableNode->expireTime = time(NULL) + ttl2;
	}
	return 1;
}

// 在DNS中继服务器中获取数据，若返回0表示DNS中继服务器中无相应数据，应向因特网DNS服务器发出查询,返回2表示在cache中查到
int getData(char* url, char** ip) {
	flashTable();
	// 先在cache中查询
	if (getDataInCache(url, ip)) {
		return 2;
	}

	// 再在table中查找
	struct list_head* p = NULL, * n = NULL;
	Node* tableNode = findNodeInTrie(url, 0);
	if (tableNode == NULL) {
		// 没找到，返回0
		strcpy(*ip, "NOTFOUND");
		return 0;
	}

	strcpy(*ip, tableNode->ip);
	if (tableNode->expireTime != 0)	tableNode->expireTime = time(NULL) + ttl;
	// 向cache中加入该数据
	addDataToCache(url, tableNode->ip);
	// 在DNS_table中找到，返回1
	return 1;
}

// 删除cache
void deleteCache() {
	// 删除cache字典树
	delTrie(cacheTrie);
	struct list_head* p = NULL, * n = NULL;
	list_for_each_safe(p, n, &cache->list) {
		Node* cacheNode = list_entry(p, Node, list);
		list_del(p);
		free(cacheNode);
	}
}

// 删除table
void deleteTable() {
	// 先删除cache
	deleteCache();
	// 再删除table
	// 删除table字典树
	delTrie(tableTrie);
	struct list_head* p = NULL, * n = NULL;
	list_for_each_safe(p, n, &table->list) {
		Node* tableNode = list_entry(p, Node, list);
		list_del(p);
		free(tableNode);
	}
}

// 打印链表（用于debug）0 打印table，1 打印cache
void printList(int type) {
	struct list_head* p = NULL;
	struct list_head* list_to_print = NULL;
	if (type) list_to_print = &cache->list;
	else list_to_print = &table->list;
	if (!list_to_print || list_to_print->next == list_to_print->prev) {
		printf("链表为空！！！\n");
		return;
	}
	printf("%s\n", list_entry(list_to_print, Node, list)->url);
	list_for_each(p, list_to_print) {

		Node* ln = list_entry(p, Node, list);
		printf("%s %s\n", ln->url, ln->ip);
	}
}