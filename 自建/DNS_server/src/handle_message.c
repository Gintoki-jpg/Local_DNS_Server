#include"handle_message.h"

int count_c = 0;  // 客户端debug消息序号（避免并发后台消息紊乱）
int count_s = 0;  // 服务器debug消息序号（避免并发后台消息紊乱）
extern int debug_level;

#ifdef _WIN32
extern SOCKET my_socket;  // 套接字
#elif __linux__
extern int my_socket;
#endif
extern struct sockaddr_in client_addr;
extern struct sockaddr_in server_addr;
extern id_table ID_table[ID_TABLE_SIZE];

void handle_client_message(char* buf, int length, struct sockaddr_in client_addr)
{

	char old_url[200];  // 转换前的url
	char new_url[200];  // 转换后的url
	char ip[100];
	char inter_result[200];
	char cache_result[200];
	char sendbuf[BUFFER_SIZE];
	int pos = 0;
	char res_rec[16];
	int point = 0;
	count_c++;

	if (debug_level)
	{
		print_debug_time();
		printf("\n#%d:收到来自本地客户端的请求", count_c);
		if (debug_level == 2) {
			show_message(buf, length);  // 显示完整报文
		}
	}

	// 从请求报文的QNAME中获得url,HEADER长度为12字节，故直接访问buf数组的第12个元素即QNAME字段
	memcpy(old_url, &(buf[12]), length);
	// 将请求报文中的字符串格式的url转换成普通模式的url
	url_transform(old_url, new_url);


	// 这一步是判断客户端发来的是IPV6报文还是IPV4报文，请求IPV6的报文只能中继给外部的服务器处理
	while (*(buf + 12 + point))  // QNAME以0X00为结束符
	{
		point++;
	}
	point++;  // 跳过长度为i的QNAME，现在buf+12+i代表的就是QTYPE
	int getResult;
	char* temip = ip;
	getResult = getData(new_url, &temip);
	// 若QTYPE字段非空
	if (buf + 12 + point != NULL)
	{
		unsigned short type = ntohs(*(unsigned short*)(buf + 12 + point));  // 获取TYPE字段

		if (type == 28 && strcmp(ip, "0.0.0.0") != 0)
		{
			if (debug_level == 2) {
				printf("\n#%d:客户端发出的IPV6请求，将向外部服务器寻求帮助", count_c, new_url);
			}
			// 声明ID标识符(用于处理和id有关的信息)
			unsigned short ID;
			// 获取客户端的id(因为是第一个信息直接获取buf即可)
			memcpy(&ID, buf, sizeof(unsigned short));
			// 将原id存入id映射表并获得新的id(为了后续服务器代理功能准备-这个顺序就该放在后面)
			unsigned short new_id = id_transform(ID, client_addr, 0);

			if (new_id == 0)
			{
				if (debug_level == 2)
				{
					printf("\n#%d:向外部服务器发送请求失败，本地ID转换表已满！", count_c);
				}
			}
			// 假设请求到了新的id，注意这个id是位置+1得到的
			else
			{
				memcpy(buf, &new_id, sizeof(unsigned short));  // 将新id加入缓冲区
				length = sendto(my_socket, buf, length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
				// 将域名查询请求发送至外部dns服务器
				if (debug_level == 2)
				{
					printf("\n#%d:成功向外部服务器发送请求", count_c);

				}
			}
			return;
		}

	}

	// 从本地(dns_table或者chache)中查询是否有该记录
	// 若域名在dns_table、cache中均无法找到，需要向外部dns服务器请求帮助
	if (getResult == 0)
	{
		if (debug_level == 2) {
			printf("\n#%d:本地服务器查询<URL:%s>失败，即将向外部服务器发送请求", count_c, new_url);
		}
		unsigned short ID;
		memcpy(&ID, buf, sizeof(unsigned short));
		// 将原客户端id进行转换得到新本地服务器id
		unsigned short new_id = id_transform(ID, client_addr, 0);
		// 假如id转换失败则报错并退出
		if (new_id == 0)
		{
			if (debug_level == 2)
			{
				printf("\n#%d:向外部服务器发送请求失败，本地ID转换表已满！", count_c);
			}
		}
		// 将域名查询请求发送至外部dns服务器
		else
		{
			memcpy(buf, &new_id, sizeof(unsigned short));
			length = sendto(my_socket, buf, length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
			if (debug_level == 2)
			{
				printf("\n#%d:成功向外部服务器发送请求,需要查询的域名为<URL:%s>", count_c, new_url);
			}
		}
	}
	// 若在本地查询到了域名和ip的映射
	else
	{
		// 如果是从dns_table中查到
		if (getResult == 1)
		{
			count_s++;
			if (debug_level == 2) {
				printf("\n#%d:从内存中查到<URL:%s>对应的IP，结果如下:\n", count_c, new_url);
				printf("<URL: %s , IP: %s>", new_url, ip);

			}
		}
		// 如果是从cache中查到
		else
		{
			count_s++;  // 为了保持请求和响应同步

			if (debug_level == 2)
			{
				printf("\n#%d:从cache中查到<URL:%s>对应的IP，结果如下:\n", count_c, new_url);
				printf("<URL: %s , IP: %s>", new_url, ip);

			}
		}

		if (debug_level == 2)
		{
			printf("\n#%d:构造并发送响应报文", count_c);

		}
		memcpy(sendbuf, buf, length);  // 构造响应报文
		// 1.构造头部
		unsigned short num;
		if (strcmp(ip, "0.0.0.0") == 0)  // 判断此ip是否应该被墙
		{
			num = htons(0x8183);
			memcpy(&sendbuf[2], &num, sizeof(unsigned short));
		}
		else
		{
			num = htons(0x8180);
			memcpy(&sendbuf[2], &num, sizeof(unsigned short));
		}

		if (strcmp(ip, "0.0.0.0") == 0)  // 此ip是否合法
		{

			num = htons(0x0);  // 假如非法直接不回答
		}
		else
		{
			num = htons(0x1);  // 合法则回答数为1
		}
		memcpy(&sendbuf[6], &num, sizeof(unsigned short));
		// 2.构造RR资源记录
		unsigned short name = htons(0xc00c);
		memcpy(res_rec, &name, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// TYPE字段
		unsigned short type = htons(0x0001);
		memcpy(res_rec + pos, &type, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// CLASS字段
		unsigned short _class = htons(0x0001);
		memcpy(res_rec + pos, &_class, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// TTL字段
		unsigned long ttl = htonl(0x00000080);
		memcpy(res_rec + pos, &ttl, sizeof(unsigned long));
		pos += sizeof(unsigned long);

		// RDLENGTH字段
		unsigned short RDlength = htons(0x0004);
		memcpy(res_rec + pos, &RDlength, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// RDATA字段
		unsigned long IP = (unsigned long)inet_addr(ip);
		memcpy(res_rec + pos, &IP, sizeof(unsigned long));
		pos += sizeof(unsigned long);
		pos += length;

		// 将HEADER和RR共同组成的DNS响应报文存入sendbuf准备发送(此处不考虑QUESTION部分)
		memcpy(sendbuf + length, res_rec, sizeof(res_rec));
		// 将构造好的报文段发给客户端
		length = sendto(my_socket, sendbuf, pos, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
		if (length < 0 && debug_level == 2)
		{
			printf("\n#%d:向客户端发送响应失败", count_c);
		}
		if (debug_level == 2)
		{
			printf("\n#%d:成功向本地客户端发送响应", count_c);
		}

	}

}


void handle_server_message(char* buf, int length, struct sockaddr_in server_addr)
{
	count_s++;  // 外部DNS响应与本地客户端请求对应
	char url[200];  // 声明用于保存url的数组
	if (debug_level)
	{
		print_debug_time();
		printf("\n#%d:收到来自外部服务器的响应", count_s);
		if (debug_level == 2) {
			// 显示完整响应报文
			show_message(buf, length);
		}
	}

	// 1.解析第一部分，Header头部
	unsigned short ID;
	// 从接收报文(buf区中)中获取ID号
	memcpy(&ID, buf, sizeof(unsigned short));
	int cur_id = ID - 1;

	//从ID映射表查找原来的ID号
	// ID映射表中的old_id字段就是原来本地客户端的id号
	memcpy(buf, &ID_table[cur_id].client_id, sizeof(unsigned short));
	// 将id映射表中的状态改了，表明该次向外部服务器发送的请求成功
	ID_table[cur_id].finished = 1;
	// 获取映射表中的client_addr字段，也就是本地客户端的地址结构体，后续与它进行通信
	struct sockaddr_in client_temp = ID_table[cur_id].client_addr;
	// QDCOUNT字段在第四个字节开始第六个字节结束,表示Question实体的数量,buf仅仅是缓冲数组的首地址
	int num_query = ntohs(*((unsigned short*)(buf + 4)));
	// ANCOUNT字段在第六个字节开始第八个字节结束，表示Answer实体的数量
	int num_response = ntohs(*((unsigned short*)(buf + 6)));
	// 移动指针,令p指向Question实体(12个字节的Header后面就是Question)
	char* p = buf + 12;

	// 2.解析第二部分，Question
	for (int i = 0; i < num_query; i++)
		// 将QUESTION的num_query个实体的url全部转换为正常的url
	{
		// 将字符计数法表示的的域名转换为正常形式的域名
		url_transform(p, url);

		while (*p > 0)
		{
			p += (*p) + 1;   // 直到*p=0即指向字符串结尾\0
		}
		p += 5;  // p=p+sizeof(char)*5（1字节\0，2字节QTYPE，2字节QCLASS）,p指向下一个QUESTION实体

	}


	if (num_response > 0 && debug_level == 2)
	{
		// 这个url就是我们需要查询的url
		printf("\n#%d:外部DNS服务器查询<URL:%s>成功，解析结果如下:", count_s, url);
	}
	if (num_response == 0 && debug_level == 2)
	{
		printf("\n#%d:外部DNS服务器查询<URL:%s>失败", count_s, url);
	}

	// 3.解析第三部分，分析来自外部DNS服务器的ANSWER,其中ANSWER|AUTHORITY|ADDITIONAL的实体均为RR(这里只分析ANSWER)
	for (int i = 0; i < num_response; ++i)
	{
		// 分析所有的ANSWER
		// NAME字段使用指针偏移表示，只占两个字节(规定)
		if ((unsigned char)*p == 0xc0)
		{
			p += 2;
		}
		else
		{
			while (*p > 0)
			{
				p += (*p) + 1;
			}
			++p;
		}

		// 获取ANSWER资源记录域(RR)的各个字段(不需要获取NAME，因为NAME就是前面获取的url)
		unsigned short type = ntohs(*(unsigned short*)p);  // TYPE字段
		p += sizeof(unsigned short);
		unsigned short _class = ntohs(*(unsigned short*)p);  // CLASS字段
		p += sizeof(unsigned short);
		unsigned short ttl_high_byte = ntohs(*(unsigned short*)p);  // TTL高位字节
		p += sizeof(unsigned short);
		unsigned short ttl_low_byte = ntohs(*(unsigned short*)p);  // TTL低位字节
		p += sizeof(unsigned short);
		// 将TTL高位和低位合并为int类型数据
		int ttl = (((int)ttl_high_byte) << 16) | ttl_low_byte;
		// 前面几个字段(TYPE CLASS TTL)的长度，此处令人混淆，并不代表DATALENGTH
		int rdlength = ntohs(*(unsigned short*)p);
		// TYPE A的资源数据Value是4字节的IP地址,即RDATA字段的字节数为4，现在p指向DATA字段
		p += sizeof(unsigned short);
		// 调试等级为2输出冗余信息
		if (debug_level == 2)
		{
			printf("\n#%d:收到的响应报文属性 TYPE: %d,  CLASS: %d,  TTL: %d", count_s, type, _class, ttl);
		}
		char ip[16];
		int ip1, ip2, ip3, ip4;  // 因为每个ip字段占一个字节，所以我们需要用四个ip段来接收，最后拼接

		// TYPE=1表示TYPE A，TYPE=5表示TYPE CNAME
		if (type == 1)
		{
			ip1 = (unsigned char)*p++;
			ip2 = (unsigned char)*p++;
			ip3 = (unsigned char)*p++;
			ip4 = (unsigned char)*p++;
			sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

			if (debug_level == 2)
			{
				printf("\n#%d:IPV4： %d.%d.%d.%d", count_s, ip1, ip2, ip3, ip4);
			}

			addDataToTable(url, ip, 1);
			break;
		}
		// 若TYPE不为A则直接跳过获取DATAip的步骤
		else
		{
			p += rdlength;
		}
	}
	// 将报文发送给客户端
	length = sendto(my_socket, buf, length, 0, (struct sockaddr*)&client_temp, sizeof(client_temp));
	// 响应消息
	if (debug_level == 2)
	{
		printf("\n#%d:成功向本地客户端转发响应", count_s);
	}
}

