#include "handle_message.h"

#ifdef _WIN32 
extern SOCKET my_socket;                       //�׽���
#elif __linux__
extern int my_socket;
#endif
/*
extern nodeptr inter_url_ip;                   //�ڴ��б����url-ipӳ��
extern nodeptr cache_url_ip;                   //cache�б����url-ipӳ��
*/

int main(int argc, char* argv[])
{
    handle_arguments(argc, argv);    //����shell�����ʼ��������Ϣ���ⲿDNS������IP��ַ�Լ������ļ�·��
    /*
    inter_url_ip = create_list();    //���ڴ��д���url-ipӳ���(������)
    cache_url_ip = create_list();    //��cache�д���url-ipӳ���(������)
    */
    initTable();  // ����dns_table��
    insert_to_inter();               //��ȡ�����ļ�dnsrelay.txt�������ڴ��е�url-ipӳ�����
    initialize_id_table();           //��ʼ��id��
    #ifdef _WIN32 
    struct WSAData wsaData;                 //��ʼ��Socket��
    WSAStartup(MAKEWORD(2, 2), &wsaData);//���ݰ汾֪ͨ����ϵͳ������SOCKET�Ķ�̬���ӿ�;WSA�汾Ϊ2.2�����󷵻�ֵ��Ϊ0
    #endif
    initialize_socket();             //��ʼ���׽���
    char buf[BUFFER_SIZE];          //����������,���������ͨ�ڻ������Կͻ��˺������ⲿ�������ı���
    struct sockaddr_in tmp_sockaddr;       //��ʼ��һ�����õ�ַ�ṹ��SOCKADDR_IN(ע��ͨ�õ�ַ�ṹ��Ϊsockaddr)
    int length;                     //�������ĵĳ���length
    int sockaddr_in_size = sizeof(struct sockaddr_in);//����ͨ�õ�ַ�ṹ�峤��

    while (1)                                            //����ѭ��ʵ�ַ�������������
    {
        memset(buf, '\0', BUFFER_SIZE);                     //��ձ��ط������Ļ�����
        length = -1;                                        //�������ݱ�����

        //����������һ������ֱ���յ�����

        /*
            recvfrom()����������һ�����ݱ��������ݴ���buf�У�������Դ��ַ
            local_socket�������ӵı����׽ӿ�
            buf:�������ݻ�����
            sizeof(buf)�����ջ�������С
            0����־λflag����ʾ���ò�����ʽ��Ĭ����Ϊ0
            client:���񵽵����ݷ���Դ��ַ��Socket��ַ��
            sockaddr_in_size:��ַ����
            ����ֵ��recvLength���ɹ����յ������ݵ��ַ��������ȣ�������ʧ�ܷ���SOCKET_ERROR(-1)
        */

        length = recvfrom(my_socket, buf, sizeof(buf), 0, (struct sockaddr*)&tmp_sockaddr, &sockaddr_in_size);//��recvfrom�����ķ���ֵ��length����
//        printf("%d",length);
        if (length > 0)                                     //���ݱ��ĳ�������������ȷ�յ�������
        {

            if (tmp_sockaddr.sin_port == htons(53))         //�������ݱ���Դ��ַ�˿ں�Ϊ53����ȷ����������DNS�����������ݱ�(���ǹ涨������ʹ��UDP����TCP)
            {
                handle_server_message(buf, length, tmp_sockaddr);//�����ⲿ���������Ĳ����д���
            }
            else
            {
                handle_client_message(buf, length, tmp_sockaddr);//���ձ��ؿͻ��˱��Ĳ����д���
            }
        }

    }
    deleteTable();
    return 0;
}
