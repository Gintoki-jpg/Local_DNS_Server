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

	
	cache_size = 0;
	printf("初始化“域名-IP 地址”对照表完成。\n");
}

// 向cache中加入数据
void addDataToCache(char* url, char* ip) {
	Node* p = (Node*)malloc(sizeof(Node));
	/*memset(&p, 0, sizeof(p));*/
	strcpy(p->url, url);
	strcpy(p->ip, ip);
	p->expireTime = time(NULL) + ttl;
	list_add(&p->list, &cache->list);
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
		struct list_head* t = cache_LRU_data;
		cache_LRU_data = NULL;
		list_del(t);
		cache_size--;
	}
}


// 向table中加入数据 type为1时，将加入的数据同步到cache中
void addDataToTable(char* url, char* ip, int type) {
	char tem[100];
	char* temip = tem;
	int resultType = getData(url, &temip);
	if (resultType == 0) {
		Node* p;
		p = (Node*)malloc(sizeof(Node));
		strcpy(p->url, url);
		strcpy(p->ip, ip);
		p->expireTime = time(NULL);  // 对于table来说，expireTime为加入table的时间
		list_add(&p->list, &table->list);
	}
	if (resultType == 1 && type == 1)
		addDataToCache(url, ip);

}

// 在cache中根据URL获取数据，若返回0表示cache中无相应数据，应查询table
int getDataInCache(char* url, char** ip) {
	struct list_head* p = NULL, * n = NULL;
	uint32_t minTime = 0;
	int flag = 0;
	list_for_each_safe(p, n, &cache->list) {
		Node* cacheNode = list_entry(p, Node, list);

		// 如果获取到相应的数据，则更新cache中数据的expireTime并给出查询到的IP地址
		if (strcmp(cacheNode->url, url) == 0) {
			strcpy(*ip, cacheNode->ip);
			cacheNode->expireTime = time(NULL) + ttl;
			flag = 1;
		}
		// 如果cache中该数据超时，则在cache中删除该数据
		else if (cacheNode->expireTime < time(NULL)) {
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
	if (flag == 1) return 1;
	// cache中无相应数据，返回
	return 0;
}

// 在DNS中继服务器中获取数据，若返回0表示DNS中继服务器中无相应数据，应向因特网DNS服务器发出查询,返回2表示在cache中查到
int getData(char* url, char** ip) {
	// 先在cache中查询
	if (getDataInCache(url, ip)) {
		return 2;
	}

	// 再在table中查找
	struct list_head* p = NULL, * n = NULL;
	list_for_each_safe(p, n, &table->list) {
		Node* tableNode = list_entry(p, Node, list);

		if (strcmp(tableNode->url, url) == 0) {

			strcpy(*ip, tableNode->ip);
			printf("ip:%s	table中的:%s\n", *ip, tableNode->ip);
			tableNode->expireTime = time(NULL);

			// 向cache中加入该数据
			addDataToCache(url, tableNode->ip);
			// 在DNS_table中找到，返回1
			return 1;
		}
	}

	// 没找到，返回0
	strcpy(*ip, "NOTFOUND");
	return 0;
}




// 删除cache
void deleteCache() {
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