#include"dns_server.h"

// 定义全局变量
id_table ID_table[ID_TABLE_SIZE];  // id表
#ifdef _WIN32 
SOCKET my_socket;  // 套接字                       
#elif __linux__
int my_socket;
#endif
struct sockaddr_in client_addr;  // 客户端套接字地址结构体                
struct sockaddr_in server_addr;  // 服务器套接字地址结构体                
int debug_level = 0;  // 初始化debug等级                    
char server_ip[16] = "192.168.163.1";  // 默认外部服务器ip地址        
char file_path[100] = "dnsrelay.txt";  // 默认配置文件路径   

// 输出当前时间戳
void print_debug_time()
{
	time_t t = time(NULL);
	char temp[64];
	strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
	printf("\n\n本地时间:");
	printf(temp);
}


void handle_arguments(int argc, char* argv[])
{
	int is_assigned = 0;  // 默认用户都使用的默认配置
	if (argc > 1 && argv[1][0] == '-')
	{
		if (argv[1][1] == 'd')
		{
			debug_level = 1;  // 调试等级：1      
		}
		if (argv[1][2] == 'd')
		{
			debug_level = 2;  // 调试等级：2      
		}
		// 若参数数量大于2，就是说用户还指定了外部DNS服务器的IP地址  
		if (argc > 2)
		{
			is_assigned = 1;  // 标记用户进行了设置
			strcpy(server_ip, argv[2]);  // 使用指定外部DNS服务器IP地址
			printf("指定DNS服务器的IP地址：  %s\n", server_ip);
			if (argc == 4)  // 不仅指定了外部DNS服务器的IP地址，还指定了配置文件的路径
			{
				strcpy(file_path, argv[3]);
			}
		}
	}

	if (!is_assigned)  // 未指定外部服务器，则设为默认服务器
	{
		printf("使用默认DNS服务器IP地址：  %s \n", server_ip);
	}
	printf("debug=%d\n", debug_level);
}

void insert_to_inter()
{
	FILE* file;
	if ((file = fopen(file_path, "r")) == NULL)                 //从file_path读取文件失败
	{
		printf("读取配置文件失败\n");
		return;
	}

	char url[100] = "", ip[16] = "";
	printf("加载配置文件成功,结果如下:\n");
	while (fscanf(file, "%s %s", ip, url) > 0)
	{
		printf("<域名: %s, IP地址 : %s>\n", url, ip);
		/*
		inter_url_ip = push_front(inter_url_ip, url, ip);       //添加至内存中
		*/
		addDataToTable(url, ip, 0);  // 添加到DNS_table中
	}
	fclose(file);
}

void initialize_id_table()
{
	for (int i = 0; i < ID_TABLE_SIZE; i++)
	{//因为ID映射表是一个结构体，所以需要递归索引进行初始化
		ID_table[i].client_id = 0;
		ID_table[i].finished = 1;
		ID_table[i].survival_time = 0;
		memset(&(ID_table[i].client_addr), 0, sizeof(struct sockaddr_in));
	}
}

void initialize_socket()
{
	my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//若创建套接字失败，则退出程序
	if (my_socket < 0)
	{
		printf("socket连接建立失败！\n");
		exit(1);
	}
	printf("socket连接建立成功！\n");



	client_addr.sin_family = AF_INET;            //IPv4
	client_addr.sin_addr.s_addr = INADDR_ANY;    //本地ip地址随机
	client_addr.sin_port = htons(53);           //绑定到53端口

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);//绑定到外部服务器ip
	server_addr.sin_port = htons(53);

	/* 本地端口号，通用端口号，允许复用 */
	int reuse = 0;
	setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

	//将端口号与socket关联
	if (bind(my_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0)
	{
		printf("socket端口绑定失败！\n");
		exit(1);
	}
	printf("socket端口绑定成功！\n");
}

void show_message(char* buf, int len)
{
	unsigned char byte;
	if (debug_level == 2) {
		printf("\n完整报文内容如下:\n");
	}

	for (int i = 0; i < len;)
	{
		byte = (unsigned char)buf[i];
		printf("%02x ", byte);
		i++;
		if (i % 16 == 0)    //每行显示16字节
			printf("\n");
	}
	if (debug_level == 2) {
		printf("\n报文总长度为%d", len);
	}
}

void url_transform(char* str, char* result)
{
	int i = 0, j = 0, k = 0, len = strlen(str);
	while (i < len)
	{
		if (str[i] > 0 && str[i] <= 63)                 //假如ASCLL是0-63即十六进制前缀，第二个数表示标签的长度
		{
			for (j = str[i], i++; j > 0; j--, i++, k++) //将字符计数法中的url提取出来放入result中
			{
				result[k] = str[i];
			}
		}
		if (str[i] != 0)                                //如果还没结束即还没读到\0符号，则加"."做url分隔符
		{
			result[k] = '.';
			k++;
		}
	}
	result[k] = '\0';                                   //给转换完成的url添加结束符，我们得到"api.sina.com.cn\0"
}

unsigned short id_transform(unsigned short ID, struct sockaddr_in client_addr, int finished)
{
	int i = 0;
	for (i = 0; i != ID_TABLE_SIZE; ++i)//只有当id转换表没有满的时候才能存入客户端id
	{
		//存入客户端的之前，需要先找到一个合适的位置(假如找不到合适的位置就一直循环就行了慢慢找，因为不满所以肯定找得到)
		if ((ID_table[i].survival_time > 0 && ID_table[i].survival_time < time(NULL))
			|| ID_table[i].finished == 1)                 //合适的位置：指超时失效或者已完成的位置
		{
			if (ID_table[i].client_id != i + 1)                //确保新旧ID不同，所以+1处理
			{
				ID_table[i].client_id = ID;                    //保存ID
				ID_table[i].client_addr = client_addr;      //保存客户端套接字
				ID_table[i].finished = finished;            //标记该客户端的请求是否查询已经完成
				ID_table[i].survival_time = time(NULL) + 5; //生存时间设置为5秒
				break;
			}
		}
	}
	if (i == ID_TABLE_SIZE)                                 //登记失败，当前id转换表已满
	{
		return 0;
	}
	return (unsigned short)i + 1;                         //返回ID号，注意这个ID号是+1处理的(讲义规定)
}

