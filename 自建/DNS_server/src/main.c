#include "handle_message.h"

//条件编译实现linux和win下的兼容
#ifdef _WIN32
extern SOCKET my_socket;  // 套接字外部引用声明
#elif __linux__
extern int my_socket;
#endif

int main(int argc, char* argv[])
{
	// 处理shell命令，初始化调试信息、外部DNS服务器IP地址以及配置文件路径
	handle_arguments(argc, argv);
	// 创建dns_table表
	initTable();
	// 读取配置文件dnsrelay.txt，存入内存中的url-ip映射表中
	insert_to_inter();
	// 初始化id表
	initialize_id_table();
#ifdef _WIN32
	//声明结构体，WSAData功能是存放windows socket初始化信息
	struct WSAData wsaData;
	// 根据版本通知操作系统，启用SOCKET的动态链接库;WSA版本为2.2，错误返回值不为0
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	// 初始化套接字
	initialize_socket();
	// 声明缓冲区,这个缓冲区通于缓存来自客户端和来自外部服务器的报文
	char buf[BUFFER_SIZE];
	// 初始化一个常用地址结构体SOCKADDR_IN(注意通用地址结构体为sockaddr)
	struct sockaddr_in tmp_sockaddr;
	// 声明报文的长度length
	int length;
	// 常用地址结构体长度
	int sockaddr_in_size = sizeof(struct sockaddr_in);
	// 无限循环实现服务器持续监听
	while (1)
	{
		// 清空本地服务器的缓冲区
		memset(buf, '\0', BUFFER_SIZE);
		length = -1;  // 重置数据报长度

		// 服务器将会一致阻塞直到收到数据
		// 将recvfrom函数的返回值用length接收
		length = recvfrom(my_socket, buf, sizeof(buf), 0, (struct sockaddr*)&tmp_sockaddr, &sockaddr_in_size);
		// 数据报的长度是正数表正确收到了数据
		if (length > 0)
		{
			// 若该数据报的源地址端口号为UDP 53可以确定这是来自DNS服务器的数据报
			if (tmp_sockaddr.sin_port == htons(53))
			{
				// 接收外部服务器报文并进行处理
				handle_server_message(buf, length, tmp_sockaddr);
			}
			//否则认为这是本地客户端（Shell、浏览器解析器等）发送的报文（客户端的端口号是随机分配的）
			else
			{
				// 接收本地客户端报文并进行处理
				handle_client_message(buf, length, tmp_sockaddr);
			}
		}
	}
	//回收内存
	deleteTable();
	return 0;
}
