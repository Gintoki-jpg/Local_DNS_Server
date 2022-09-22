#include"handle_message.h"

int count_c = 0;  // �ͻ���debug��Ϣ��ţ����Ⲣ����̨��Ϣ���ң�
int count_s = 0;  // ������debug��Ϣ��ţ����Ⲣ����̨��Ϣ���ң�
extern int debug_level;

#ifdef _WIN32
extern SOCKET my_socket;  // �׽���
#elif __linux__
extern int my_socket;
#endif
extern struct sockaddr_in client_addr;
extern struct sockaddr_in server_addr;
extern id_table ID_table[ID_TABLE_SIZE];

void handle_client_message(char* buf, int length, struct sockaddr_in client_addr)
{

	char old_url[200];  // ת��ǰ��url
	char new_url[200];  // ת�����url
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
		printf("\n#%d:�յ����Ա��ؿͻ��˵�����", count_c);
		if (debug_level == 2) {
			show_message(buf, length);  // ��ʾ��������
		}
	}

	// �������ĵ�QNAME�л��url,HEADER����Ϊ12�ֽڣ���ֱ�ӷ���buf����ĵ�12��Ԫ�ؼ�QNAME�ֶ�
	memcpy(old_url, &(buf[12]), length);
	// ���������е��ַ�����ʽ��urlת������ͨģʽ��url
	url_transform(old_url, new_url);


	// ��һ�����жϿͻ��˷�������IPV6���Ļ���IPV4���ģ�����IPV6�ı���ֻ���м̸��ⲿ�ķ���������
	while (*(buf + 12 + point))  // QNAME��0X00Ϊ������
	{
		point++;
	}
	point++;  // ��������Ϊi��QNAME������buf+12+i����ľ���QTYPE
	int getResult;
	char* temip = ip;
	getResult = getData(new_url, &temip);
	// ��QTYPE�ֶηǿ�
	if (buf + 12 + point != NULL)
	{
		unsigned short type = ntohs(*(unsigned short*)(buf + 12 + point));  // ��ȡTYPE�ֶ�

		if (type == 28 && strcmp(ip, "0.0.0.0") != 0)
		{
			if (debug_level == 2) {
				printf("\n#%d:�ͻ��˷�����IPV6���󣬽����ⲿ������Ѱ�����", count_c, new_url);
			}
			// ����ID��ʶ��(���ڴ����id�йص���Ϣ)
			unsigned short ID;
			// ��ȡ�ͻ��˵�id(��Ϊ�ǵ�һ����Ϣֱ�ӻ�ȡbuf����)
			memcpy(&ID, buf, sizeof(unsigned short));
			// ��ԭid����idӳ�������µ�id(Ϊ�˺���������������׼��-���˳��͸÷��ں���)
			unsigned short new_id = id_transform(ID, client_addr, 0);

			if (new_id == 0)
			{
				if (debug_level == 2)
				{
					printf("\n#%d:���ⲿ��������������ʧ�ܣ�����IDת����������", count_c);
				}
			}
			// �����������µ�id��ע�����id��λ��+1�õ���
			else
			{
				memcpy(buf, &new_id, sizeof(unsigned short));  // ����id���뻺����
				length = sendto(my_socket, buf, length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
				// ��������ѯ���������ⲿdns������
				if (debug_level == 2)
				{
					printf("\n#%d:�ɹ����ⲿ��������������", count_c);

				}
			}
			return;
		}

	}

	// �ӱ���(dns_table����chache)�в�ѯ�Ƿ��иü�¼
	// ��������dns_table��cache�о��޷��ҵ�����Ҫ���ⲿdns�������������
	if (getResult == 0)
	{
		if (debug_level == 2) {
			printf("\n#%d:���ط�������ѯ<URL:%s>ʧ�ܣ��������ⲿ��������������", count_c, new_url);
		}
		unsigned short ID;
		memcpy(&ID, buf, sizeof(unsigned short));
		// ��ԭ�ͻ���id����ת���õ��±��ط�����id
		unsigned short new_id = id_transform(ID, client_addr, 0);
		// ����idת��ʧ���򱨴��˳�
		if (new_id == 0)
		{
			if (debug_level == 2)
			{
				printf("\n#%d:���ⲿ��������������ʧ�ܣ�����IDת����������", count_c);
			}
		}
		// ��������ѯ���������ⲿdns������
		else
		{
			memcpy(buf, &new_id, sizeof(unsigned short));
			length = sendto(my_socket, buf, length, 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
			if (debug_level == 2)
			{
				printf("\n#%d:�ɹ����ⲿ��������������,��Ҫ��ѯ������Ϊ<URL:%s>", count_c, new_url);
			}
		}
	}
	// ���ڱ��ز�ѯ����������ip��ӳ��
	else
	{
		// ����Ǵ�dns_table�в鵽
		if (getResult == 1)
		{
			count_s++;
			if (debug_level == 2) {
				printf("\n#%d:���ڴ��в鵽<URL:%s>��Ӧ��IP���������:\n", count_c, new_url);
				printf("<URL: %s , IP: %s>", new_url, ip);

			}
		}
		// ����Ǵ�cache�в鵽
		else
		{
			count_s++;  // Ϊ�˱����������Ӧͬ��

			if (debug_level == 2)
			{
				printf("\n#%d:��cache�в鵽<URL:%s>��Ӧ��IP���������:\n", count_c, new_url);
				printf("<URL: %s , IP: %s>", new_url, ip);

			}
		}

		if (debug_level == 2)
		{
			printf("\n#%d:���첢������Ӧ����", count_c);

		}
		memcpy(sendbuf, buf, length);  // ������Ӧ����
		// 1.����ͷ��
		unsigned short num;
		if (strcmp(ip, "0.0.0.0") == 0)  // �жϴ�ip�Ƿ�Ӧ�ñ�ǽ
		{
			num = htons(0x8183);
			memcpy(&sendbuf[2], &num, sizeof(unsigned short));
		}
		else
		{
			num = htons(0x8180);
			memcpy(&sendbuf[2], &num, sizeof(unsigned short));
		}

		if (strcmp(ip, "0.0.0.0") == 0)  // ��ip�Ƿ�Ϸ�
		{

			num = htons(0x0);  // ����Ƿ�ֱ�Ӳ��ش�
		}
		else
		{
			num = htons(0x1);  // �Ϸ���ش���Ϊ1
		}
		memcpy(&sendbuf[6], &num, sizeof(unsigned short));
		// 2.����RR��Դ��¼
		unsigned short name = htons(0xc00c);
		memcpy(res_rec, &name, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// TYPE�ֶ�
		unsigned short type = htons(0x0001);
		memcpy(res_rec + pos, &type, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// CLASS�ֶ�
		unsigned short _class = htons(0x0001);
		memcpy(res_rec + pos, &_class, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// TTL�ֶ�
		unsigned long ttl = htonl(0x00000080);
		memcpy(res_rec + pos, &ttl, sizeof(unsigned long));
		pos += sizeof(unsigned long);

		// RDLENGTH�ֶ�
		unsigned short RDlength = htons(0x0004);
		memcpy(res_rec + pos, &RDlength, sizeof(unsigned short));
		pos += sizeof(unsigned short);

		// RDATA�ֶ�
		unsigned long IP = (unsigned long)inet_addr(ip);
		memcpy(res_rec + pos, &IP, sizeof(unsigned long));
		pos += sizeof(unsigned long);
		pos += length;

		// ��HEADER��RR��ͬ��ɵ�DNS��Ӧ���Ĵ���sendbuf׼������(�˴�������QUESTION����)
		memcpy(sendbuf + length, res_rec, sizeof(res_rec));
		// ������õı��Ķη����ͻ���
		length = sendto(my_socket, sendbuf, pos, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
		if (length < 0 && debug_level == 2)
		{
			printf("\n#%d:��ͻ��˷�����Ӧʧ��", count_c);
		}
		if (debug_level == 2)
		{
			printf("\n#%d:�ɹ��򱾵ؿͻ��˷�����Ӧ", count_c);
		}

	}

}


void handle_server_message(char* buf, int length, struct sockaddr_in server_addr)
{
	count_s++;  // �ⲿDNS��Ӧ�뱾�ؿͻ��������Ӧ
	char url[200];  // �������ڱ���url������
	if (debug_level)
	{
		print_debug_time();
		printf("\n#%d:�յ������ⲿ����������Ӧ", count_s);
		if (debug_level == 2) {
			// ��ʾ������Ӧ����
			show_message(buf, length);
		}
	}

	// 1.������һ���֣�Headerͷ��
	unsigned short ID;
	// �ӽ��ձ���(buf����)�л�ȡID��
	memcpy(&ID, buf, sizeof(unsigned short));
	int cur_id = ID - 1;

	//��IDӳ������ԭ����ID��
	// IDӳ����е�old_id�ֶξ���ԭ�����ؿͻ��˵�id��
	memcpy(buf, &ID_table[cur_id].client_id, sizeof(unsigned short));
	// ��idӳ����е�״̬���ˣ������ô����ⲿ���������͵�����ɹ�
	ID_table[cur_id].finished = 1;
	// ��ȡӳ����е�client_addr�ֶΣ�Ҳ���Ǳ��ؿͻ��˵ĵ�ַ�ṹ�壬������������ͨ��
	struct sockaddr_in client_temp = ID_table[cur_id].client_addr;
	// QDCOUNT�ֶ��ڵ��ĸ��ֽڿ�ʼ�������ֽڽ���,��ʾQuestionʵ�������,buf�����ǻ���������׵�ַ
	int num_query = ntohs(*((unsigned short*)(buf + 4)));
	// ANCOUNT�ֶ��ڵ������ֽڿ�ʼ�ڰ˸��ֽڽ�������ʾAnswerʵ�������
	int num_response = ntohs(*((unsigned short*)(buf + 6)));
	// �ƶ�ָ��,��pָ��Questionʵ��(12���ֽڵ�Header�������Question)
	char* p = buf + 12;

	// 2.�����ڶ����֣�Question
	for (int i = 0; i < num_query; i++)
		// ��QUESTION��num_query��ʵ���urlȫ��ת��Ϊ������url
	{
		// ���ַ���������ʾ�ĵ�����ת��Ϊ������ʽ������
		url_transform(p, url);

		while (*p > 0)
		{
			p += (*p) + 1;   // ֱ��*p=0��ָ���ַ�����β\0
		}
		p += 5;  // p=p+sizeof(char)*5��1�ֽ�\0��2�ֽ�QTYPE��2�ֽ�QCLASS��,pָ����һ��QUESTIONʵ��

	}


	if (num_response > 0 && debug_level == 2)
	{
		// ���url����������Ҫ��ѯ��url
		printf("\n#%d:�ⲿDNS��������ѯ<URL:%s>�ɹ��������������:", count_s, url);
	}
	if (num_response == 0 && debug_level == 2)
	{
		printf("\n#%d:�ⲿDNS��������ѯ<URL:%s>ʧ��", count_s, url);
	}

	// 3.�����������֣����������ⲿDNS��������ANSWER,����ANSWER|AUTHORITY|ADDITIONAL��ʵ���ΪRR(����ֻ����ANSWER)
	for (int i = 0; i < num_response; ++i)
	{
		// �������е�ANSWER
		// NAME�ֶ�ʹ��ָ��ƫ�Ʊ�ʾ��ֻռ�����ֽ�(�涨)
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

		// ��ȡANSWER��Դ��¼��(RR)�ĸ����ֶ�(����Ҫ��ȡNAME����ΪNAME����ǰ���ȡ��url)
		unsigned short type = ntohs(*(unsigned short*)p);  // TYPE�ֶ�
		p += sizeof(unsigned short);
		unsigned short _class = ntohs(*(unsigned short*)p);  // CLASS�ֶ�
		p += sizeof(unsigned short);
		unsigned short ttl_high_byte = ntohs(*(unsigned short*)p);  // TTL��λ�ֽ�
		p += sizeof(unsigned short);
		unsigned short ttl_low_byte = ntohs(*(unsigned short*)p);  // TTL��λ�ֽ�
		p += sizeof(unsigned short);
		// ��TTL��λ�͵�λ�ϲ�Ϊint��������
		int ttl = (((int)ttl_high_byte) << 16) | ttl_low_byte;
		// ǰ�漸���ֶ�(TYPE CLASS TTL)�ĳ��ȣ��˴����˻�������������DATALENGTH
		int rdlength = ntohs(*(unsigned short*)p);
		// TYPE A����Դ����Value��4�ֽڵ�IP��ַ,��RDATA�ֶε��ֽ���Ϊ4������pָ��DATA�ֶ�
		p += sizeof(unsigned short);
		// ���Եȼ�Ϊ2���������Ϣ
		if (debug_level == 2)
		{
			printf("\n#%d:�յ�����Ӧ�������� TYPE: %d,  CLASS: %d,  TTL: %d", count_s, type, _class, ttl);
		}
		char ip[16];
		int ip1, ip2, ip3, ip4;  // ��Ϊÿ��ip�ֶ�ռһ���ֽڣ�����������Ҫ���ĸ�ip�������գ����ƴ��

		// TYPE=1��ʾTYPE A��TYPE=5��ʾTYPE CNAME
		if (type == 1)
		{
			ip1 = (unsigned char)*p++;
			ip2 = (unsigned char)*p++;
			ip3 = (unsigned char)*p++;
			ip4 = (unsigned char)*p++;
			sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

			if (debug_level == 2)
			{
				printf("\n#%d:IPV4�� %d.%d.%d.%d", count_s, ip1, ip2, ip3, ip4);
			}

			addDataToTable(url, ip, 1);
			break;
		}
		// ��TYPE��ΪA��ֱ��������ȡDATAip�Ĳ���
		else
		{
			p += rdlength;
		}
	}
	// �����ķ��͸��ͻ���
	length = sendto(my_socket, buf, length, 0, (struct sockaddr*)&client_temp, sizeof(client_temp));
	// ��Ӧ��Ϣ
	if (debug_level == 2)
	{
		printf("\n#%d:�ɹ��򱾵ؿͻ���ת����Ӧ", count_s);
	}
}

