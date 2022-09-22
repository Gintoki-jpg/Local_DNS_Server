#include "handle_message.h"

#ifdef _WIN32 
extern SOCKET my_socket;                       //套接字
#elif __linux__
extern int my_socket;
#endif
/*
extern nodeptr inter_url_ip;                   //内存中保存的url-ip映射
extern nodeptr cache_url_ip;                   //cache中保存的url-ip映射
*/

int main(int argc, char* argv[])
{
    handle_arguments(argc, argv);    //处理shell命令，初始化调试信息、外部DNS服务器IP地址以及配置文件路径
    /*
    inter_url_ip = create_list();    //在内存中创建url-ip映射表(空链表)
    cache_url_ip = create_list();    //在cache中创建url-ip映射表(空链表)
    */
    initTable();  // 创建dns_table表
    insert_to_inter();               //读取配置文件dnsrelay.txt，存入内存中的url-ip映射表中
    initialize_id_table();           //初始化id表
    #ifdef _WIN32 
    struct WSAData wsaData;                 //初始化Socket库
    WSAStartup(MAKEWORD(2, 2), &wsaData);//根据版本通知操作系统，启用SOCKET的动态链接库;WSA版本为2.2，错误返回值不为0
    #endif
    initialize_socket();             //初始化套接字
    char buf[BUFFER_SIZE];          //声明缓冲区,这个缓冲区通于缓存来自客户端和来自外部服务器的报文
    struct sockaddr_in tmp_sockaddr;       //初始化一个常用地址结构体SOCKADDR_IN(注意通用地址结构体为sockaddr)
    int length;                     //声明报文的长度length
    int sockaddr_in_size = sizeof(struct sockaddr_in);//声明通用地址结构体长度

    while (1)                                            //无限循环实现服务器持续监听
    {
        memset(buf, '\0', BUFFER_SIZE);                     //清空本地服务器的缓冲区
        length = -1;                                        //重置数据报长度

        //服务器将会一致阻塞直到收到数据

        /*
            recvfrom()函数：接收一个数据报，将数据存至buf中，并保存源地址
            local_socket：已连接的本地套接口
            buf:接收数据缓冲区
            sizeof(buf)：接收缓冲区大小
            0：标志位flag，表示调用操作方式，默认设为0
            client:捕获到的数据发送源地址（Socket地址）
            sockaddr_in_size:地址长度
            返回值：recvLength：成功接收到的数据的字符数（长度），接收失败返回SOCKET_ERROR(-1)
        */

        length = recvfrom(my_socket, buf, sizeof(buf), 0, (struct sockaddr*)&tmp_sockaddr, &sockaddr_in_size);//将recvfrom函数的返回值用length接收
//        printf("%d",length);
        if (length > 0)                                     //数据报的长度是正数表正确收到了数据
        {

            if (tmp_sockaddr.sin_port == htons(53))         //若该数据报的源地址端口号为53可以确定这是来自DNS服务器的数据报(这是规定，无论使用UDP还是TCP)
            {
                handle_server_message(buf, length, tmp_sockaddr);//接收外部服务器报文并进行处理
            }
            else
            {
                handle_client_message(buf, length, tmp_sockaddr);//接收本地客户端报文并进行处理
            }
        }

    }
    deleteTable();
    return 0;
}
