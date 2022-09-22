#include "handle_message.h"

//��������ʵ��linux��win�µļ���
#ifdef _WIN32
extern SOCKET my_socket;  // �׽����ⲿ��������
#elif __linux__
extern int my_socket;
#endif

int main(int argc, char* argv[])
{
	// ����shell�����ʼ��������Ϣ���ⲿDNS������IP��ַ�Լ������ļ�·��
	handle_arguments(argc, argv);
	// ����dns_table��
	initTable();
	// ��ȡ�����ļ�dnsrelay.txt�������ڴ��е�url-ipӳ�����
	insert_to_inter();
	// ��ʼ��id��
	initialize_id_table();
#ifdef _WIN32
	//�����ṹ�壬WSAData�����Ǵ��windows socket��ʼ����Ϣ
	struct WSAData wsaData;
	// ���ݰ汾֪ͨ����ϵͳ������SOCKET�Ķ�̬���ӿ�;WSA�汾Ϊ2.2�����󷵻�ֵ��Ϊ0
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	// ��ʼ���׽���
	initialize_socket();
	// ����������,���������ͨ�ڻ������Կͻ��˺������ⲿ�������ı���
	char buf[BUFFER_SIZE];
	// ��ʼ��һ�����õ�ַ�ṹ��SOCKADDR_IN(ע��ͨ�õ�ַ�ṹ��Ϊsockaddr)
	struct sockaddr_in tmp_sockaddr;
	// �������ĵĳ���length
	int length;
	// ���õ�ַ�ṹ�峤��
	int sockaddr_in_size = sizeof(struct sockaddr_in);
	// ����ѭ��ʵ�ַ�������������
	while (1)
	{
		// ��ձ��ط������Ļ�����
		memset(buf, '\0', BUFFER_SIZE);
		length = -1;  // �������ݱ�����

		// ����������һ������ֱ���յ�����
		// ��recvfrom�����ķ���ֵ��length����
		length = recvfrom(my_socket, buf, sizeof(buf), 0, (struct sockaddr*)&tmp_sockaddr, &sockaddr_in_size);
		// ���ݱ��ĳ�������������ȷ�յ�������
		if (length > 0)
		{
			// �������ݱ���Դ��ַ�˿ں�ΪUDP 53����ȷ����������DNS�����������ݱ�
			if (tmp_sockaddr.sin_port == htons(53))
			{
				// �����ⲿ���������Ĳ����д���
				handle_server_message(buf, length, tmp_sockaddr);
			}
			//������Ϊ���Ǳ��ؿͻ��ˣ�Shell��������������ȣ����͵ı��ģ��ͻ��˵Ķ˿ں����������ģ�
			else
			{
				// ���ձ��ؿͻ��˱��Ĳ����д���
				handle_client_message(buf, length, tmp_sockaddr);
			}
		}
	}
	//�����ڴ�
	deleteTable();
	return 0;
}
