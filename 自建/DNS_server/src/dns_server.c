#include"dns_server.h"

// ����ȫ�ֱ���
id_table ID_table[ID_TABLE_SIZE];  // id��
#ifdef _WIN32 
SOCKET my_socket;  // �׽���                       
#elif __linux__
int my_socket;
#endif
struct sockaddr_in client_addr;  // �ͻ����׽��ֵ�ַ�ṹ��                
struct sockaddr_in server_addr;  // �������׽��ֵ�ַ�ṹ��                
int debug_level = 0;  // ��ʼ��debug�ȼ�                    
char server_ip[16] = "192.168.163.1";  // Ĭ���ⲿ������ip��ַ        
char file_path[100] = "dnsrelay.txt";  // Ĭ�������ļ�·��   

// �����ǰʱ���
void print_debug_time()
{
	time_t t = time(NULL);
	char temp[64];
	strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
	printf("\n\n����ʱ��:");
	printf(temp);
}


void handle_arguments(int argc, char* argv[])
{
	int is_assigned = 0;  // Ĭ���û���ʹ�õ�Ĭ������
	if (argc > 1 && argv[1][0] == '-')
	{
		if (argv[1][1] == 'd')
		{
			debug_level = 1;  // ���Եȼ���1      
		}
		if (argv[1][2] == 'd')
		{
			debug_level = 2;  // ���Եȼ���2      
		}
		// ��������������2������˵�û���ָ�����ⲿDNS��������IP��ַ  
		if (argc > 2)
		{
			is_assigned = 1;  // ����û�����������
			strcpy(server_ip, argv[2]);  // ʹ��ָ���ⲿDNS������IP��ַ
			printf("ָ��DNS��������IP��ַ��  %s\n", server_ip);
			if (argc == 4)  // ����ָ�����ⲿDNS��������IP��ַ����ָ���������ļ���·��
			{
				strcpy(file_path, argv[3]);
			}
		}
	}

	if (!is_assigned)  // δָ���ⲿ������������ΪĬ�Ϸ�����
	{
		printf("ʹ��Ĭ��DNS������IP��ַ��  %s \n", server_ip);
	}
	printf("debug=%d\n", debug_level);
}

void insert_to_inter()
{
	FILE* file;
	if ((file = fopen(file_path, "r")) == NULL)                 //��file_path��ȡ�ļ�ʧ��
	{
		printf("��ȡ�����ļ�ʧ��\n");
		return;
	}

	char url[100] = "", ip[16] = "";
	printf("���������ļ��ɹ�,�������:\n");
	while (fscanf(file, "%s %s", ip, url) > 0)
	{
		printf("<����: %s, IP��ַ : %s>\n", url, ip);
		/*
		inter_url_ip = push_front(inter_url_ip, url, ip);       //������ڴ���
		*/
		addDataToTable(url, ip, 0);  // ��ӵ�DNS_table��
	}
	fclose(file);
}

void initialize_id_table()
{
	for (int i = 0; i < ID_TABLE_SIZE; i++)
	{//��ΪIDӳ�����һ���ṹ�壬������Ҫ�ݹ��������г�ʼ��
		ID_table[i].client_id = 0;
		ID_table[i].finished = 1;
		ID_table[i].survival_time = 0;
		memset(&(ID_table[i].client_addr), 0, sizeof(struct sockaddr_in));
	}
}

void initialize_socket()
{
	my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//�������׽���ʧ�ܣ����˳�����
	if (my_socket < 0)
	{
		printf("socket���ӽ���ʧ�ܣ�\n");
		exit(1);
	}
	printf("socket���ӽ����ɹ���\n");



	client_addr.sin_family = AF_INET;            //IPv4
	client_addr.sin_addr.s_addr = INADDR_ANY;    //����ip��ַ���
	client_addr.sin_port = htons(53);           //�󶨵�53�˿�

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);//�󶨵��ⲿ������ip
	server_addr.sin_port = htons(53);

	/* ���ض˿ںţ�ͨ�ö˿ںţ������� */
	int reuse = 0;
	setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

	//���˿ں���socket����
	if (bind(my_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0)
	{
		printf("socket�˿ڰ�ʧ�ܣ�\n");
		exit(1);
	}
	printf("socket�˿ڰ󶨳ɹ���\n");
}

void show_message(char* buf, int len)
{
	unsigned char byte;
	if (debug_level == 2) {
		printf("\n����������������:\n");
	}

	for (int i = 0; i < len;)
	{
		byte = (unsigned char)buf[i];
		printf("%02x ", byte);
		i++;
		if (i % 16 == 0)    //ÿ����ʾ16�ֽ�
			printf("\n");
	}
	if (debug_level == 2) {
		printf("\n�����ܳ���Ϊ%d", len);
	}
}

void url_transform(char* str, char* result)
{
	int i = 0, j = 0, k = 0, len = strlen(str);
	while (i < len)
	{
		if (str[i] > 0 && str[i] <= 63)                 //����ASCLL��0-63��ʮ������ǰ׺���ڶ�������ʾ��ǩ�ĳ���
		{
			for (j = str[i], i++; j > 0; j--, i++, k++) //���ַ��������е�url��ȡ��������result��
			{
				result[k] = str[i];
			}
		}
		if (str[i] != 0)                                //�����û��������û����\0���ţ����"."��url�ָ���
		{
			result[k] = '.';
			k++;
		}
	}
	result[k] = '\0';                                   //��ת����ɵ�url��ӽ����������ǵõ�"api.sina.com.cn\0"
}

unsigned short id_transform(unsigned short ID, struct sockaddr_in client_addr, int finished)
{
	int i = 0;
	for (i = 0; i != ID_TABLE_SIZE; ++i)//ֻ�е�idת����û������ʱ����ܴ���ͻ���id
	{
		//����ͻ��˵�֮ǰ����Ҫ���ҵ�һ�����ʵ�λ��(�����Ҳ������ʵ�λ�þ�һֱѭ�������������ң���Ϊ�������Կ϶��ҵõ�)
		if ((ID_table[i].survival_time > 0 && ID_table[i].survival_time < time(NULL))
			|| ID_table[i].finished == 1)                 //���ʵ�λ�ã�ָ��ʱʧЧ��������ɵ�λ��
		{
			if (ID_table[i].client_id != i + 1)                //ȷ���¾�ID��ͬ������+1����
			{
				ID_table[i].client_id = ID;                    //����ID
				ID_table[i].client_addr = client_addr;      //����ͻ����׽���
				ID_table[i].finished = finished;            //��Ǹÿͻ��˵������Ƿ��ѯ�Ѿ����
				ID_table[i].survival_time = time(NULL) + 5; //����ʱ������Ϊ5��
				break;
			}
		}
	}
	if (i == ID_TABLE_SIZE)                                 //�Ǽ�ʧ�ܣ���ǰidת��������
	{
		return 0;
	}
	return (unsigned short)i + 1;                         //����ID�ţ�ע�����ID����+1�����(����涨)
}

