#include "DNS_table.h"

// ��ʼ��������-IP ��ַ�����ձ�table
void initTable() {
	// Ϊtable��ͷ����ռ�
	table = (Node*)malloc(sizeof(Node));
	memset(table, 0, sizeof(Node));
	INIT_LIST_HEAD(&table->list);

	// Ϊcache��ͷ����ռ�
	cache = (Node*)malloc(sizeof(Node));
	memset(cache, 0, sizeof(Node));
	INIT_LIST_HEAD(&cache->list);

	// Ϊ�ֵ�����ͷ����ռ�
	tableTrie = (TrieNode*)malloc(sizeof(TrieNode));
	memset(tableTrie, 0, sizeof(TrieNode));
	cacheTrie = (TrieNode*)malloc(sizeof(TrieNode));
	memset(cacheTrie, 0, sizeof(TrieNode));
	lastFlashTime = time(NULL);

	cache_size = 0;
	printf("��ʼ��������-IP ��ַ�����ձ���ɡ�\n");
}

// ���ֵ��������ӽ�㣬typeΪ0����table�ֵ�������ӣ�Ϊ1����cache�ֵ��������
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

// ���ֵ����в�ѯ typeΪ0����table�ֵ����в�ѯ��Ϊ1����cache�ֵ����в�ѯ
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

// ���ֵ�����ɾ����� typeΪ0����table�ֵ�����ɾ����Ϊ1����cache�ֵ�����ɾ��
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

// �ݹ��ɾ���ֵ���
void delTrie(TrieNode* p) {
	if (p->ptr[0])	free(p->ptr[0]);
	for (int i = 1; i < BRANCH; i++) {
		if (p->ptr[i]) {
			delTrie(p->ptr[i]);
			free(p->ptr[i]);
		}
	}
}

// ��cache�м�������
void addDataToCache(char* url, char* ip) {
	Node* p = (Node*)malloc(sizeof(Node));
	memset(p, 0, sizeof(Node));
	strcpy(p->url, url);
	strcpy(p->ip, ip);
	p->expireTime = time(NULL) + ttl;
	list_add(&p->list, &cache->list);
	addDataToTrie(url, 1, p);
	cache_size++;

	// ���cache�����������ɾ����ǰcache_list��ָ������ݽ��
	// cache_list��ָ���Ϊδ������ʱ����Ľ�㣬���getDataInCache()
	while (cache_size > CACHE_MAX_SIZE) {
		if (cache_LRU_data == NULL) {
			uint32_t minTime = 0;
			struct list_head* p = NULL, * n = NULL;
			list_for_each_safe(p, n, &cache->list) {
				Node* cacheNode = list_entry(p, Node, list);
				// ���cache�и����ݳ�ʱ������cache��ɾ��������
				if (cacheNode->expireTime < time(NULL)) {
					delNodeInTrie(cacheNode->url, 1);
					list_del(p);
					cache_size--;
					free(cacheNode);
				}
				// ʼ����cache_listָ���ʱ��δ���ʵ�����
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

// ��table�м������� typeΪ1ʱ�������������ͬ����cache�У�Ϊ2ʱ����Ϊ�ǳ�ʼ��
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
			p->expireTime = 0;  // ��̬���ݣ�����ɾ��
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

// ���table�г�ʱ��δ���ʵ��������ͷ��ڴ�
void flashTable() {
	if (lastFlashTime + flashTime < time(NULL)) {
		printf("��ʱˢ��table\n");
		struct list_head* p = NULL, * n = NULL;
		int flag = 0;
		list_for_each_safe(p, n, &table->list) {
			Node* tableNode = list_entry(p, Node, list);
			// ���table�и����ݳ�ʱ���Ҹ����ݲ��ǳ�ʼ��ʱ�ľ�̬���ݣ�����table��ɾ�����������ͷ��ڴ�
			if (tableNode->expireTime < time(NULL) && tableNode->expireTime != 0) {
				delNodeInTrie(tableNode->url, 0);
				list_del(p);
				free(tableNode);
			}
		}
	}
}

// ��cache�и���URL��ȡ���ݣ�������0��ʾcache������Ӧ���ݣ�Ӧ��ѯtable
int getDataInCache(char* url, char** ip) {
	Node* cacheNode = findNodeInTrie(url, 1);
	// cache������Ӧ���ݣ�����
	if (cacheNode == NULL) return 0;
	
	// �����ȡ����Ӧ�����ݣ������cache�����ݵ�expireTime��������ѯ����IP��ַ
	strcpy(*ip, cacheNode->ip);
	cacheNode->expireTime = time(NULL) + ttl;

	// ����table�е�expireTime
	Node* tableNode = findNodeInTrie(url, 0);
	if (tableNode != NULL && tableNode->expireTime != 0) {
		tableNode->expireTime = time(NULL) + ttl2;
	}
	return 1;
}

// ��DNS�м̷������л�ȡ���ݣ�������0��ʾDNS�м̷�����������Ӧ���ݣ�Ӧ��������DNS������������ѯ,����2��ʾ��cache�в鵽
int getData(char* url, char** ip) {
	flashTable();
	// ����cache�в�ѯ
	if (getDataInCache(url, ip)) {
		return 2;
	}

	// ����table�в���
	struct list_head* p = NULL, * n = NULL;
	Node* tableNode = findNodeInTrie(url, 0);
	if (tableNode == NULL) {
		// û�ҵ�������0
		strcpy(*ip, "NOTFOUND");
		return 0;
	}

	strcpy(*ip, tableNode->ip);
	if (tableNode->expireTime != 0)	tableNode->expireTime = time(NULL) + ttl;
	// ��cache�м��������
	addDataToCache(url, tableNode->ip);
	// ��DNS_table���ҵ�������1
	return 1;
}

// ɾ��cache
void deleteCache() {
	// ɾ��cache�ֵ���
	delTrie(cacheTrie);
	struct list_head* p = NULL, * n = NULL;
	list_for_each_safe(p, n, &cache->list) {
		Node* cacheNode = list_entry(p, Node, list);
		list_del(p);
		free(cacheNode);
	}
}

// ɾ��table
void deleteTable() {
	// ��ɾ��cache
	deleteCache();
	// ��ɾ��table
	// ɾ��table�ֵ���
	delTrie(tableTrie);
	struct list_head* p = NULL, * n = NULL;
	list_for_each_safe(p, n, &table->list) {
		Node* tableNode = list_entry(p, Node, list);
		list_del(p);
		free(tableNode);
	}
}

// ��ӡ��������debug��0 ��ӡtable��1 ��ӡcache
void printList(int type) {
	struct list_head* p = NULL;
	struct list_head* list_to_print = NULL;
	if (type) list_to_print = &cache->list;
	else list_to_print = &table->list;
	if (!list_to_print || list_to_print->next == list_to_print->prev) {
		printf("����Ϊ�գ�����\n");
		return;
	}
	printf("%s\n", list_entry(list_to_print, Node, list)->url);
	list_for_each(p, list_to_print) {

		Node* ln = list_entry(p, Node, list);
		printf("%s %s\n", ln->url, ln->ip);
	}
}