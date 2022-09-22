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

	
	cache_size = 0;
	printf("��ʼ��������-IP ��ַ�����ձ���ɡ�\n");
}

// ��cache�м�������
void addDataToCache(char* url, char* ip) {
	Node* p = (Node*)malloc(sizeof(Node));
	/*memset(&p, 0, sizeof(p));*/
	strcpy(p->url, url);
	strcpy(p->ip, ip);
	p->expireTime = time(NULL) + ttl;
	list_add(&p->list, &cache->list);
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
		struct list_head* t = cache_LRU_data;
		cache_LRU_data = NULL;
		list_del(t);
		cache_size--;
	}
}


// ��table�м������� typeΪ1ʱ�������������ͬ����cache��
void addDataToTable(char* url, char* ip, int type) {
	char tem[100];
	char* temip = tem;
	int resultType = getData(url, &temip);
	if (resultType == 0) {
		Node* p;
		p = (Node*)malloc(sizeof(Node));
		strcpy(p->url, url);
		strcpy(p->ip, ip);
		p->expireTime = time(NULL);  // ����table��˵��expireTimeΪ����table��ʱ��
		list_add(&p->list, &table->list);
	}
	if (resultType == 1 && type == 1)
		addDataToCache(url, ip);

}

// ��cache�и���URL��ȡ���ݣ�������0��ʾcache������Ӧ���ݣ�Ӧ��ѯtable
int getDataInCache(char* url, char** ip) {
	struct list_head* p = NULL, * n = NULL;
	uint32_t minTime = 0;
	int flag = 0;
	list_for_each_safe(p, n, &cache->list) {
		Node* cacheNode = list_entry(p, Node, list);

		// �����ȡ����Ӧ�����ݣ������cache�����ݵ�expireTime��������ѯ����IP��ַ
		if (strcmp(cacheNode->url, url) == 0) {
			strcpy(*ip, cacheNode->ip);
			cacheNode->expireTime = time(NULL) + ttl;
			flag = 1;
		}
		// ���cache�и����ݳ�ʱ������cache��ɾ��������
		else if (cacheNode->expireTime < time(NULL)) {
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
	if (flag == 1) return 1;
	// cache������Ӧ���ݣ�����
	return 0;
}

// ��DNS�м̷������л�ȡ���ݣ�������0��ʾDNS�м̷�����������Ӧ���ݣ�Ӧ��������DNS������������ѯ,����2��ʾ��cache�в鵽
int getData(char* url, char** ip) {
	// ����cache�в�ѯ
	if (getDataInCache(url, ip)) {
		return 2;
	}

	// ����table�в���
	struct list_head* p = NULL, * n = NULL;
	list_for_each_safe(p, n, &table->list) {
		Node* tableNode = list_entry(p, Node, list);

		if (strcmp(tableNode->url, url) == 0) {

			strcpy(*ip, tableNode->ip);
			printf("ip:%s	table�е�:%s\n", *ip, tableNode->ip);
			tableNode->expireTime = time(NULL);

			// ��cache�м��������
			addDataToCache(url, tableNode->ip);
			// ��DNS_table���ҵ�������1
			return 1;
		}
	}

	// û�ҵ�������0
	strcpy(*ip, "NOTFOUND");
	return 0;
}




// ɾ��cache
void deleteCache() {
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