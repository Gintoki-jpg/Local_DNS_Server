#include "public.h"
#include"dns_server.h"
#include"list.h"

//定义全局变量
int count = 0;                          //序号
id_table ID_table[ID_TABLE_SIZE];	    //ID映射表
SOCKET my_socket;                       //套接字
SOCKADDR_IN client_addr;                //客户端套接字地址结构体
SOCKADDR_IN server_addr;                //服务器套接字地址结构体
int debug_level = 0;                    //初始化debug等级 调试等级为 0 时，没有调试信息输出|调试等级为 1 时，输出简单的时间坐标，序号，查询域名|调试等级为 2 时，输出冗余的调试信息
char server_ip[16] = "10.3.9.4";        //默认服务器ip地址
char file_path[100] = "dnsrelay.txt";   //默认配置文件路径
nodeptr url_ip_table;                   //域名IP地址映射表
nodeptr cache;                          //高速缓存

int main(int argc, char* argv[])
{
    printf("命令格式: dnsrelay [ -d | -dd ] [ <服务器IP地址> ] [ <配置文件路径> ]\n");

    //show_hint_info();//输出设计者信息

    get_arguments(argc, argv);       //读取用户输入命令，初始化调试信息、外部DNS服务器IP地址以及配置文件路径

    url_ip_table = create_list();    //在内存中创建url-ip映射表(空链表)

    cache = create_list();           //在cache中创建url-ip映射表(空链表)

    get_url_ip_map();                //读取配置文件dnsrelay.txt，存入内存的url-ip映射表中


    initialize_id_table();           //初始化ID映射表

    WSADATA wsaData;                 //存储Socket库初始化信息

    WSAStartup(MAKEWORD(2, 2), &wsaData);  //根据版本通知操作系统，启用SOCKET的动态链接库

    initialize_socket();             //初始化套接字

    char buf[BUFFER_SIZE];          //声明缓冲区,这个缓冲区通于缓存来自客户端和来自外部服务器的报文
    SOCKADDR_IN tmp_sockaddr;       //初始化一个常用地址结构体SOCKADDR_IN(注意通用地址结构体为sockaddr)
    int length;                     //声明数据报的字符数（长度）
    int sockaddr_in_size = sizeof(SOCKADDR_IN);//声明通用地址结构体长度

    while (TRUE)                    //无限循环实现服务器持续监听
    {
        memset(buf, '\0', BUFFER_SIZE);//清空本地服务器的缓冲区
        /*
        使用recvfrom()函数从客户端接收请求报文
        recvfrom()函数：接收一个数据报，将数据存至buf中，并保存源地址结构体
            @local_socket：已连接的本地套接口
            @buf:接收数据缓冲区
            @sizeof(buf)：接收缓冲区大小
            @0：标志位flag，表示调用操作方式，默认设为0
            @client:捕获到的数据发送源地址（Socket地址）
            @sockaddr_in_size:地址长度
            返回值：recvLength：成功接收到的数据的字符数（长度），接收失败返回SOCKET_ERROR(-1)
        */

        //服务器将会一致阻塞直到收到数据
        length = -1;                                        //重置数据报长度(重置确实应该每个循环都要进行)
        length= recvfrom(my_socket, buf, sizeof(buf), 0, (struct sockaddr*) & tmp_sockaddr, &sockaddr_in_size);//将recvfrom函数的返回值用length接收

        if (length > 0)                                     //数据报的长度是正数表示收到了数据
        {
            if (tmp_sockaddr.sin_port == htons(53))         //假如说这个数据报的源地址端口号为53可以确定这是来自外部DNS服务器的数据(这是规定)
            {
                process_server_request(buf, length, tmp_sockaddr);//接收外部服务器数据并进行后续处理
            }
            else
            {
                process_client_request(buf, length, tmp_sockaddr); //接收本地客户端数据并进行后续处理
            }
        }
    }
    return 0;
}



//调试等级为1的输出
void show_packet(char* buf, int len)
{
    unsigned char byte;
    printf("数据包内容:\n");
    for (int i = 0; i < len;)
    {
        byte = (unsigned char)buf[i];
        printf("%02x ", byte);
        i++;
        if (i % 16 == 0)    //每行只打印16字节
            printf("\n");
    }
    printf("\n数据包总长度 = %d\n\n", len);
}

//展示cache列表
void show_cache()
{
    printf("\ncache列表\n");
    print_list(cache);
}

//根据配置文件读取url-ip映射表至服务器内存中
void get_url_ip_map()
{
    FILE* file;
    if ((file = fopen(file_path, "r")) == NULL)                 //从file_path读取文件失败
    {
        if (debug_level > 1)
            printf("读取配置文件失败\n");
        return;
    }

    char url[65] = "", ip[16] = "";

    if (debug_level > 1)
        printf("加载配置文件:\n");
    while (fscanf(file, "%s %s", ip, url) > 0)
    {
        if (debug_level > 1)
        {
            printf("<域名: %s, IP地址 : %s>\n", url, ip);
        }
        url_ip_table = push_front(url_ip_table, url, ip);       //添加至url-ip映射表(单链表)中
    }

    fclose(file);
}

//将新的映射关系记录存入cache
void insert_record_to_cache(char* url, char *ip)
{
    //先判断cache中是否有该元素
    char tmp[100];
    memset(tmp, '\0', sizeof(tmp));
    strcpy(tmp, get_ip(cache, url));

    if (strcmp(tmp, "NOTFOUND") != 0)    //cache中已存在该域名
    {
        nodeptr cur=cache;
        while(strcmp(cur->url,url)!=0)
        {
            cur=cur->next;
        }
        for(int i=0;i<cur->num;i++)
        {
            if(strcmp(cur->ip[i],ip)==0)//若映射关系在cache中找到，什么都不做
            {
                return;
            }
        }
        if(cur->num>20)//映射表只存20个映射，多于20则返回
        {
            return;
        }
        strcpy(cache->ip[cur->num++], ip);      //否则添加映射关系
    }
    else    //cache中不存在该域名
    {
        if (size(cache) >= 8)  //cache大小为8，已满
        {
            cache = pop_back(cache);    //替换cache记录
        }
        cache = push_front(cache, url, ip);
        if(debug_level>1)
            show_cache();
    }

}

//需要向外部DNS服务器发送请求时，需要保存原id信息
unsigned short get_new_id(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished)
{
    int i = 0;
    for (i = 0; i != ID_TABLE_SIZE; ++i)
    {
        //存入之前，需要先找到一个合适的位置
        //合适的位置：指超时失效或者已完成的位置
        if ((ID_table[i].survival_time > 0 &&  ID_table[i].survival_time < time(NULL) )
            || ID_table[i].finished == TRUE)
        {
            if (ID_table[i].old_id != i + 1)      //确保新旧ID不同
            {
                ID_table[i].old_id = ID;     //设置ID
                ID_table[i].client_addr = client_addr;   //设置客户端套接字
                ID_table[i].finished = finished;  //标记是否查询已经完成
                ID_table[i].survival_time = time(NULL) + 5;     //时间设置为5秒

                //printf("\n原ID：%d",ID);
                //printf("\n新ID：%d", i);
                break;
            }
        }
    }
    if (i == ID_TABLE_SIZE) //登记失败，当前转换表已满
    {
        return 0;
    }
    return (unsigned short)i + 1; //返回新ID号
}

//将字符计数法表示的的域名转换为正常形式的域名(直到遇到\0符号为止)
void transform_url(char* buf, char* result)
{
    int i = 0, j = 0, k = 0, len = strlen(buf);
    while (i < len)
    {
        if (buf[i] > 0 && buf[i] <= 63)         //计数
        {
            for (j = buf[i], i++; j > 0; j--, i++, k++) //复制url
            {
                result[k] = buf[i];
            }
        }
        if (buf[i] != 0) //如果还没结束，加个"."做分隔符
        {
            result[k] = '.';
            k++;
        }
    }
    result[k] = '\0'; //添加结束符
}

//处理来自外部服务器的消息
void process_server_request(char *buf,int length,SOCKADDR_IN server_addr)
{
    char url[200];                              //声明用于保存url的数组

    if (debug_level > 1)                        //假如调试信息为1则展示简要数据报信息
    {
        show_packet(buf, length);
    }

    //从接收报文中获取ID号
    unsigned short ID;
    memcpy(&ID, buf, sizeof(unsigned short));
    int cur_id = ID - 1;

    //从ID映射表查找原来的ID号
    memcpy(buf, &ID_table[cur_id].old_id, sizeof(unsigned short));
    ID_table[cur_id].finished = TRUE;

    //获取客户端信息
    SOCKADDR_IN client_temp = ID_table[cur_id].client_addr;

    /*响应报文header格式
      0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					   ID						|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z	|	RCODE	|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 QDCOUNT					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 ANCOUNT					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 NSCOUNT					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 ARCOUNT					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */
    //问题数QDCOUNT 无符号16位整数表示报文请求段中的问题记录数。
    //资源记录数ANCOUNT 无符号16位整数表示报文回答段中的回答记录数。

    int num_query = ntohs(*((unsigned short*)(buf + 4)));
    int num_response = ntohs(*((unsigned short*)(buf + 6)));

    char* p = buf + 12; //将p指向question字段



    //读取question字段所有的url
    /*question格式
      0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  QNAME						|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  QTYPE  					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 QCLASS			    		|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */
    for (int i = 0; i < num_query; i++)
    {
        transform_url(p, url);
        while (*p > 0)
        {
            p += (*p) + 1;
        }
        p += 5; //指向下一个Queries区域，
        /*结构：查询名（最长63字节+'\0'）+查询类型（2字节）+查询类（2字节）*/
    }

    if (num_response > 0 && debug_level > 1)
    {
        printf("来自外部DNS服务器 <Url : %s>\n", url);
    }

    //分析外部DNS服务器的回应
    /*
    构造DNS报文资源记录（RR）区域（16字节）
      0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |												|
    /												/
    /					  NAME（2）				    /
    |												|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  TYPE（2）					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 CLASS（2）					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  TTL（4）					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					RDLENGTH（2）				|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    /					 RDATA（4）					/
    /												/
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */

    for (int i = 0; i < num_response; ++i)
    {
        if ((unsigned char)*p == 0xc0) /* 表明NAME字段使用指针偏移表示 */
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
        //网络字节是从低字节到高字节的（例如0001），而主机字节顺序从高字节到低字节（对应0100）
        //ntohs：将网络字节顺序转换为主机字节顺序
        //htons: 将主机字节顺序转换为网络字节顺序

        //获取资源记录域的各个字段
        unsigned short type = ntohs(*(unsigned short*)p);           //TYPE字段
        p += sizeof(unsigned short);
        unsigned short _class = ntohs(*(unsigned short*)p);         //CLASS字段
        p += sizeof(unsigned short);
        unsigned short ttl_high_byte = ntohs(*(unsigned short*)p); //TTL高位字节
        p += sizeof(unsigned short);
        unsigned short ttl_low_byte = ntohs(*(unsigned short*)p);  //TTL低位字节
        p += sizeof(unsigned short);
        int ttl = (((int)ttl_high_byte) << 16) | ttl_low_byte;    //合并成正常的int类型数据

        int rdlength = ntohs(*(unsigned short*)p);  //数据长度
        //对类型1（TYPE A记录）资源数据是4字节的IP地址
        p += sizeof(unsigned short);

        if (debug_level > 1)
        {
            printf("类型: %d,  类: %d,  TTL: %d\n", type, _class, ttl);
        }

        char ip[16];
        int ip1, ip2, ip3, ip4;

        // 类型：A，由域名获得IPv4地址
        if (type == 1)
        {
            ip1 = (unsigned char)*p++;
            ip2 = (unsigned char)*p++;
            ip3 = (unsigned char)*p++;
            ip4 = (unsigned char)*p++;
            sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            if (debug_level>1)
            {
                printf("IP地址1： %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
            }

            insert_record_to_cache(url,ip);

            while(p&&*(p+1)&&*(p+=12))          //获取服务器发来报文的IP地址，跳过前面的12字节
            {
                ip1=(unsigned char)*p++;
                ip2=(unsigned char)*p++;
                ip3=(unsigned char)*p++;
                ip4=(unsigned char)*p++;
                sprintf(ip,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);

                if(debug_level>1)
                {
                    printf("IP地址2： %d.%d.%d.%d\n",ip1,ip2,ip3,ip4);
                }

                //将映射加入到cache映射表
                insert_record_to_cache(url, ip);
            }
            break;
        }
        else
        {
            p += rdlength;
        }
    }

    //将包送回客户端
    //sendto()函数：发送一个数据报，数据在buf中，需要提供地址
    /*
        local_socket：外部服务器套接字
        buf：发送数据缓冲区
        sizeof(buf)：接收缓冲区大小
        0：标志位flag，表示调用操作方式，默认设为0
        client：目标数据发送源地址
        sizeof(client)：地址大小
        返回值：length：成功接收到的数据的字符数（长度），接收失败返回SOCKET_ERROR(-1)
    */
    length = sendto(my_socket, buf, length, 0, (SOCKADDR*)&client_temp, sizeof(client_temp));
}

//处理来自客户端的消息
void process_client_request(char *buf,int length,SOCKADDR_IN client_addr)
{
    int show_cache_flag = 0;    //是否打印cache的标志
    char old_url[200];          //转换前的url
    char new_url[200];          //转换后的url

    memcpy(old_url, &(buf[12]), length); //从报文中获得url,报头长度12字节
    transform_url(old_url, new_url);     //url转换

    int i = 0;

    while (*(buf + 12 + i))
    {
        i++;
    }
    i++;

    if (buf + 12 + i != NULL)
    {
        unsigned short type = ntohs(*(unsigned short*)(buf + 12 + i));  //TYPE字段
        if (type == 28&& strcmp(get_ip(url_ip_table,new_url),"0.0.0.0")!=0)
        {
            unsigned short ID;
            memcpy(&ID, buf, sizeof(unsigned short));
            unsigned short new_id = get_new_id(ID, client_addr, FALSE); //将原id存入id转换表

            if (new_id == 0)
            {
                if (debug_level > 1)
                {
                    printf("登记失败，本地ID转换表已满！\n");
                }
            }
            else
            {
                memcpy(buf, &new_id, sizeof(unsigned short));
                length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
                //将域名查询请求发送至外部dns服务器
                if (debug_level > 1)
                {
                    printf("<域名: %s>\n\n", new_url);
                }
            }
            return;
        }
    }



    if (debug_level > 1)
        printf("\n\n---- 收到了客户端的消息： [IP:%s]----\n", inet_ntoa(client_addr.sin_addr));

    if (debug_level)
    {
        //打印时间戳
        time_t t = time(NULL);
        char temp[64];
        strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
        printf("%s\t#%d\n", temp,count++);
    }

    char ip[100];
    char table_result[200];
    char cache_result[200];
    strcpy(table_result, get_ip(url_ip_table,new_url));     //从映射表中查询到的ip地址结果
    strcpy(cache_result, get_ip(cache, new_url));           //从cache中查询到的IP地址结果

    //从本地映射表中查询是否有该记录
    if (strcmp(table_result, "NOTFOUND") == 0 && strcmp(cache_result, "NOTFOUND") == 0)    //若域名在本地文件、cache中无法找到，需要上报外部dns服务器
    {

        unsigned short ID;
        memcpy(&ID, buf, sizeof(unsigned short));
        unsigned short new_id = get_new_id(ID, client_addr, FALSE); //将原id存入id转换表

        if (new_id == 0)
        {
            if (debug_level > 1)
            {
                printf("登记失败，本地ID转换表已满！\n");
            }
        }
        else
        {
            memcpy(buf, &new_id, sizeof(unsigned short));
            length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
            //将域名查询请求发送至外部dns服务器
            if (debug_level > 0)
            {
                printf("<域名: %s>\n\n", new_url);
            }
        }
    }
    else //若在本地查询到了域名和ip的映射
    {
        if (strcmp(table_result, "NOTFOUND") != 0) //如果是从映射表中查到
        {
            strcpy(ip, table_result);
            if (debug_level > 0)
            {
                if (debug_level > 1)
                    printf("从映射表中查到\n");
                printf("<域名: %s , IP地址: %s>\n\n", new_url, ip);
            }
        }
        else //如果是从cache中查到
        {
            strcpy(ip, cache_result);
            cache = move_to_head(cache, new_url);//将记录移到靠前的位置

            if (debug_level > 0)
            {
                if (debug_level > 1)
                    printf("从cache中查到\n");
                printf("<域名: %s , IP地址: %s>\n\n", new_url, ip);
                show_cache_flag = 1;
            }
        }

        char sendbuf[BUFFER_SIZE];
        memcpy(sendbuf, buf, length); //开始构造数据包

        /*响应报文header格式
          0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					   ID						|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |QR|   Opcode  |AA|TC|RD|RA|   Z	|	RCODE	|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					 QDCOUNT					|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					 ANCOUNT					|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					 NSCOUNT					|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					 ARCOUNT					|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
         */
        //QR为1表示响应报
        //OPCODE为0表示标准查询(QUERY)
        //AA（权威答案）为0表示应答服务器不是该域名的权威解析服务器
        //TC为0表示报文未被截断
        //RD（期望递归）为1表示希望采用递归查询
        //RA（递归可用）为1表示名字服务器支持递归查询
        //Z为0，作为保留字段
        //RCODE为0，表示没有差错
        //因此报文头部的第3-4字节为值8180



        unsigned short num;

        if (strcmp(ip, "0.0.0.0") == 0)    //判断此ip是否应该被墙
        {
            num = htons(0x8183);
            memcpy(&sendbuf[2], &num, sizeof(unsigned short));
        }
        else
        {
            num = htons(0x8180);
            memcpy(&sendbuf[2], &num, sizeof(unsigned short));
        }

        if (strcmp(ip, "0.0.0.0") == 0)    //判断此ip是否应该被墙
        {

            num = htons(0x0);    //设置回答数为0
        }
        else
        {
            num = htons(0x1);	//否则设置回答数为1
        }
        memcpy(&sendbuf[6], &num, sizeof(unsigned short));

        /*构造DNS报文资源记录（RR）区域
          0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |												|
        /												/
        /					  NAME						/
        |												|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					  TYPE						|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					 CLASS						|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					  TTL						|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |					RDLENGTH					|
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        /					 RDATA						/
        /												/
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        */

        int pos = 0;
        char res_rec[16];

        //NAME字段：资源记录中的域名通常是查询问题部分的域名的重复，可以使用2字节的偏移指针表示
        //最高2位为11，用于识别指针，最后四位为1100（12）表示头部长度
        //因此该字段的值为0xC00C
        unsigned short name = htons(0xc00c);
        memcpy(res_rec, &name, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TYPE字段：资源记录的类型，1表示IPv4地址
        unsigned short type = htons(0x0001);
        memcpy(res_rec + pos, &type, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //CLASS字段：查询类，通常为1（IN），代表Internet数据
        unsigned short _class = htons(0x0001);
        memcpy(res_rec + pos, &_class, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TTL字段：资源记录生存时间（待修改）
        unsigned long ttl = htonl(0x00000080);
        memcpy(res_rec + pos, &ttl, sizeof(unsigned long));
        pos += sizeof(unsigned long);

        //RDLENGTH字段：资源数据长度，对类型1（TYPE A记录）资源数据是4字节的IP地址
        unsigned short RDlength = htons(0x0004);
        memcpy(res_rec + pos, &RDlength, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //RDATA字段：这里是一个IP地址
        unsigned long IP = (unsigned long)inet_addr(ip);
        memcpy(res_rec + pos, &IP, sizeof(unsigned long));
        pos += sizeof(unsigned long);
        pos += length;

        //请求报文和响应部分共同组成DNS响应报文存入sendbuf
        memcpy(sendbuf + length, res_rec, sizeof(res_rec));

        length = sendto(my_socket, sendbuf, pos, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));
        //将构造好的报文段发给客户端

        if (length < 0 && debug_level>1)
        {
            printf("向客户端发包错误\n");
        }

        char* p;
        p = sendbuf + length - 4;
        if (debug_level > 1)
        {
            printf("发送数据报 <域名: %s ，IP: %u.%u.%u.%u>\n", new_url, (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
        }
        if (show_cache_flag && debug_level>1)
        {
            show_cache();
        }
    }

}

//读取用户输入的命令，设置调试等级，并根据用户命令指定外部DNS服务器的IP地址以及配置文件路径
//dnsrelay [-d | -dd] [dns-server-ipaddr] [filename]  用户最多就输入四个参数
void get_arguments(int argc, char* argv[])
{
    bool is_assigned = false;   //默认用户都使用的默认配置

    if (argc > 1 && argv[1][0] == '-')
    {
        if (argv[1][1] == 'd')
        {
            debug_level=1;      //调试等级：1
        }
        if (argv[1][2] == 'd')
        {
            debug_level=2;      //调试等级：2
        }
        if (argc > 2)           //若参数数量大于2，就是说用户还指定了外部DNS服务器的IP地址
        {
            is_assigned = true; //标记用户进行了设置
            strcpy(server_ip, argv[2]);//使用指定外部DNS服务器IP地址
            printf("指定DNS服务器IP地址：  %s\n", server_ip);
            if (argc == 4)	    //不仅指定了外部DNS服务器的IP地址，还指定了配置文件的路径
            {
                strcpy(file_path, argv[3]);
            }
        }
    }

    if(!is_assigned) //未指定外部服务器，则设为默认服务器
    {
        printf("使用默认DNS服务器IP地址：  %s \n", server_ip);
    }
    printf("调试等级： %d\n", debug_level);
}

//初始化id映射表
void initialize_id_table()
{
    for (int i = 0; i < ID_TABLE_SIZE; i++)
    {//因为ID映射表是一个结构体，所以需要递归索引进行初始化
        ID_table[i].old_id = 0;
        ID_table[i].finished = TRUE;
        ID_table[i].survival_time = 0;
        memset(&(ID_table[i].client_addr), 0, sizeof(SOCKADDR_IN));
    }
}

//初始化套接字信息
void initialize_socket()
{
    my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    //创建套接字失败，则退出程序
    if (my_socket < 0)
    {
        printf("连接建立失败！\n");
        exit(1);
    }
    if(debug_level>1)
        printf("连接建立成功！\n");

    //将发送、接受模式设定为非阻塞
    int non_block = 1;
    //ioctlsocket(my_socket, FIONBIO, (u_long FAR*) & non_block);

    client_addr.sin_family = AF_INET;            //IPv4
    client_addr.sin_addr.s_addr = INADDR_ANY;    //本地ip地址随机
    client_addr.sin_port = htons(53);      //绑定到53端口

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);//绑定到外部服务器
    server_addr.sin_port = htons(53);

    /* 本地端口号，通用端口号，允许复用 */
    int reuse = 0;
    setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    //将端口号与socket关联
    if (bind(my_socket, (struct sockaddr*) & client_addr, sizeof(client_addr)) < 0)
    {
        printf("套接字端口绑定失败。\n");
        exit(1);
    }

    if (debug_level > 1)
        printf("套接字端口绑定成功。\n");
}


