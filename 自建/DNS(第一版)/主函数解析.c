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

//1.读取用户输入的命令，设置调试等级，并根据用户命令指定外部DNS服务器的IP地址以及配置文件路径
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

//2.根据配置文件读取url-ip映射表至服务器内存中
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


//3.初始化id映射表
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

//4.初始化套接字信息
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

//5.调试等级为2的输出
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

//6.将字符计数法即带十六进制"字符计数"前缀的url转换为正常形式的域名(直到遇到\0符号为止)
//假设现在buf内待需要转换的url为"03api.04sina.o3com.02cn.00\0" 注意这里的数字全都是十六进制数
void transform_url(char* buf, char* result)
{
    int i = 0, j = 0, k = 0, len = strlen(buf);
    while (i < len)
    {
        if (buf[i] > 0 && buf[i] <= 63)                 //假如ASCLL是0-63即十六进制前缀，第二个数表示标签的长度
        {
            for (j = buf[i], i++; j > 0; j--, i++, k++) //将字符计数法中的url提取出来放入result中
            {
                result[k] = buf[i];
            }
        }
        if (buf[i] != 0)                                //如果还没结束即还没读到\0符号，则加"."做url分隔符
        {
            result[k] = '.';
            k++;
        }
    }
    result[k] = '\0';                                   //给转换完成的url添加结束符，我们得到"api.sina.com.cn\0"
}

//7.将新的映射关系记录存入cache
void insert_record_to_cache(char* url, char *ip)
{
    //先判断cache中是否有该元素
    char tmp[100];                        //声明并初始化temp缓冲区
    memset(tmp, '\0', sizeof(tmp));      //清空tmp缓冲区
    strcpy(tmp, get_ip(cache, url));      //根据url查找链表结点，返回对应的ip地址，并写入tmp缓冲区(list操作)

    if (strcmp(tmp, "NOTFOUND") != 0)    //若Cache中存在该域名，那我们只需加进去或者什么都不做就行
    {
        nodeptr cur=cache;
        while(strcmp(cur->url,url)!=0)
        {
            cur=cur->next;
        }
        for(int i=0;i<cur->num;i++)
        {
            if(strcmp(cur->ip[i],ip)==0)  //若url-ip映射关系在cache中找到，则什么都不做
            {
                return;
            }
        }
        if(cur->num>20)                   //一个url最多对应20个ip，多了直接退出
        {
            return;
        }
        strcpy(cache->ip[cur->num++], ip);//否则更新加入url-ip映射关系
    }
    else                                 //cache中不存在该域名
    {
        if (size(cache) >= 8)            //当cache大小为8认为已满
        {
            cache = pop_back(cache);     //先删除单链表最后一个结点
        }
        cache = push_front(cache, url, ip);//再加入新的url-ip映射关系(注意这个版本并没有用LRU算法)
        if(debug_level>1)                  //若调试等级等于2则输出冗余信息
            show_cache();
    }

}

//8.处理来自外部服务器的报文
void process_server_request(char *buf,int length,SOCKADDR_IN server_addr)
{
    char url[200];                              //声明用于保存url的数组

    if (debug_level > 1)                        //假如调试等级为2则展示冗余数据报信息
    {
        show_packet(buf, length);
    }

    //第一部分，Header头部
    unsigned short ID;
    memcpy(&ID, buf, sizeof(unsigned short));                      //从接收报文(buf区中)中获取ID号
    int cur_id = ID - 1;                                            //为什么cur_id要-1?因为我们的本地服务器会自动给转换过后的id+1(讲义的规定)
                                                                    //cur_id实际页就是本条id记录在id_table中的位置

    //从ID映射表查找原来的ID号
    memcpy(buf, &ID_table[cur_id].old_id, sizeof(unsigned short));//ID映射表中的old_id字段就是原来本地客户端的id号
    ID_table[cur_id].finished = TRUE;                               //将id映射表中的状态改了，表明该次向外部服务器发送的请求成功

    SOCKADDR_IN client_temp = ID_table[cur_id].client_addr;         //获取old_id映射表中的client_addr字段，也就是本地客户端的地址结构体，后续与它进行通信

    int num_query = ntohs(*((unsigned short*)(buf + 4)));          //QDCOUNT字段在第四个字节开始第六个字节结束,表示Question实体的数量,buf仅仅是缓冲数组的首地址
    int num_response = ntohs(*((unsigned short*)(buf + 6)));       //ANCOUNT字段在第六个字节开始第八个字节结束，表示Answer实体的数量

    char* p = buf + 12;                                             //移动指针,令p指向Question实体(12个字节的Header后面就是Question)

    //第二部分，Question部分
    for (int i = 0; i < num_query; i++)
    //将QUESTION的num_query个实体的url全部转换为正常的url，一般来说只会有一个QUESTION实体，所以此处不用num_query也行
    //multiple questions目前应该只是理论上存在，实际的服务器程序中应该都没有实现它
    {
        transform_url(p, url);                                      //将字符计数法表示的的域名转换为正常形式的域名

        while (*p > 0)
        {
            p += (*p) + 1;                                          //直到*p=0即指向字符串结尾\0
        }
        p += 5;                                                     //p=p+sizeof(char)*5（1字节\0，2字节QTYPE，2字节QCLASS）,p指向下一个QUESTION实体

    //url数组只会保留最后一次解析出来的url，因为实际上不管发多少个Question，每次发出一个请求报文，就仅仅只是查询一个url而已
    }

    if (num_response > 0 && debug_level > 1)                        //调试等级为2输出冗余信息
    {
        printf("外部DNS服务器查询到<Url : %s>的相关信息如下\n", url); //这个url就是我们需要查询的url
    }

    //第三部分，分析来自外部DNS服务器的ANSWER,其中ANSWER|AUTHORITY|ADDITIONAL的实体均为RR(这里只分析ANSWER)
    for (int i = 0; i < num_response; ++i)                           //分析所有的ANSWER
    {
        if ((unsigned char)*p == 0xc0)                              //NAME字段使用指针偏移表示，只占两个字节(规定)
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

        //获取ANSWER资源记录域(RR)的各个字段(不需要获取NAME，因为NAME就是前面获取的url)
        unsigned short type = ntohs(*(unsigned short*)p);           //TYPE字段
        p += sizeof(unsigned short);
        unsigned short _class = ntohs(*(unsigned short*)p);         //CLASS字段
        p += sizeof(unsigned short);
        unsigned short ttl_high_byte = ntohs(*(unsigned short*)p); //TTL高位字节
        p += sizeof(unsigned short);
        unsigned short ttl_low_byte = ntohs(*(unsigned short*)p);  //TTL低位字节
        p += sizeof(unsigned short);
        int ttl = (((int)ttl_high_byte) << 16) | ttl_low_byte;       //将TTL高位和低位合并为int类型数据

        int rdlength = ntohs(*(unsigned short*)p);                  //前面几个字段(TYPE CLASS TTL)的长度，此处令人混淆，并不代表DATALENGTH

        p += sizeof(unsigned short);                                //TYPE A的资源数据Value是4字节的IP地址,即RDATA字段的字节数为4，现在p指向DATA字段

        if (debug_level > 1)                                          //调试等级为2输出冗余信息
        {
            printf("TYPE: %d,  CLASS: %d,  TTL: %d\n", type, _class, ttl);
        }

        char ip[16];                                                  //大小大于8就可以
        int ip1, ip2, ip3, ip4;                                       //因为每个ip字段占一个字节，所以我们需要用四个ip段来接收，最后拼接


        if (type == 1)                                          // 咱们只分析ANSWER中TYPE为A的RR中的RDATA字段
        {
            ip1 = (unsigned char)*p++;
            ip2 = (unsigned char)*p++;
            ip3 = (unsigned char)*p++;
            ip4 = (unsigned char)*p++;
            sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);     //输入到指定字符串ip中

            if (debug_level>1)                                  //调试等级为2输出冗余信息
            {
                printf("IP地址： %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
            }

            insert_record_to_cache(url,ip);                     //将解析出来的url-ip映射存入本地DNS服务器的Cache中

            while(p&&*(p+1)&&*(p+=12))                          //多个ip响应
            {
                ip1=(unsigned char)*p++;
                ip2=(unsigned char)*p++;
                ip3=(unsigned char)*p++;
                ip4=(unsigned char)*p++;
                sprintf(ip,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);

                if(debug_level>1)
                {
                    printf("IP地址： %d.%d.%d.%d\n",ip1,ip2,ip3,ip4);
                }


                insert_record_to_cache(url, ip);                 //将映射加入到cache映射表
            }
            break;
        }
        else                                                    //若TYPE不为A则直接跳过获取DATAip的步骤
        {
            p += rdlength;
        }
    }
 /*
    最后用sendto()函数将包送回客户端
    sendto()函数：发送一个数据报，数据在buf中，需要提供地址
        @local_socket：外部服务器套接字
        @buf：发送数据缓冲区
        @sizeof(buf)：接收缓冲区大小
        @0：标志位flag，表示调用操作方式，默认设为0
        @client：目标数据发送源地址
        @sizeof(client)：地址大小
        返回值：length：成功接收到的数据的字符数（长度），接收失败返回SOCKET_ERROR(-1)
    */
    length = sendto(my_socket, buf, length, 0, (SOCKADDR*)&client_temp, sizeof(client_temp));
}

//9.需要向外部DNS服务器发送请求时，需要保存原id信息
unsigned short get_new_id(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished)
{
    int i = 0;
    for (i = 0; i != ID_TABLE_SIZE; ++i)//只有当id转换表没有满的时候才能存入客户端id
    {
        //存入客户端的之前，需要先找到一个合适的位置(假如找不到合适的位置就一直循环就行了慢慢找，因为不满所以肯定找得到)
        if ((ID_table[i].survival_time > 0 &&  ID_table[i].survival_time < time(NULL) )
            || ID_table[i].finished == TRUE)                 //合适的位置：指超时失效或者已完成的位置
        {
            if (ID_table[i].old_id != i + 1)                //确保新旧ID不同，所以+1处理
            {
                ID_table[i].old_id = ID;                    //保存ID
                ID_table[i].client_addr = client_addr;      //保存客户端套接字
                ID_table[i].finished = finished;            //标记该客户端的请求是否查询已经完成
                ID_table[i].survival_time = time(NULL) + 5; //生存时间设置为5秒

                //printf("\n原ID：%d",ID);
                //printf("\n新ID：%d", i);                    //新的id为i也就是在ID_table中的位置，并没有使用什么随机算法
                break;
            }
        }
    }
    if (i == ID_TABLE_SIZE)                                 //登记失败，当前id转换表已满
    {
        return 0;
    }
    return (unsigned short)i + 1;                         //返回ID号，注意这个ID号是+1处理的，原因未知
}

//10.处理来自客户端的消息(来自客户端的请求报文和响应报文格式几乎一致，QUESTION数量为1，其他三个字段一般来说不存在)
//其实处理客户端的函数应该先写，有助于理解处理服务器的代码
void process_client_request(char *buf,int length,SOCKADDR_IN client_addr)
{
    int show_cache_flag = 0;    //标记是否需要打印cache
    char old_url[200];          //转换前的url
    char new_url[200];          //转换后的url

    memcpy(old_url, &(buf[12]), length); //从请求报文的QNAME中获得url,HEADER长度为12字节，所以直接访问buf数组的第12个元素即QNAME字段
    transform_url(old_url, new_url);     //将请求报文中的字符串格式的url转换成普通模式的url，上面服务器也可以使用这种机制，因为实际上只有一个url

    int i = 0;

    while (*(buf + 12 + i))
    {
        i++;                             //跳过长度为i的QNAME，现在buf+12+i代表的就是QTYPE
    }
    i++;

    if (buf + 12 + i != NULL)           //只要QTYPE字段不是空的就执行一次里面的函数体
    {
        unsigned short type = ntohs(*(unsigned short*)(buf + 12 + i));  //获取TYPE字段
        if (type == 28&& strcmp(get_ip(url_ip_table,new_url),"0.0.0.0")!=0)//ASCLL28对应的是文件分割符，且域名对应的ip并非非法ip(假如这个url都不存在我们可以确定它没被手动设置为非法)
        {
            unsigned short ID;                                            //声明ID标识符(用于处理和id有关的信息)
            memcpy(&ID, buf, sizeof(unsigned short));                     //获取客户端的id(因为是第一个信息直接获取buf即可)
            unsigned short new_id = get_new_id(ID, client_addr, FALSE);   //将原id存入id映射表并获得新的id(为了后续服务器代理功能准备-这个顺序就该放在后面)

            if (new_id == 0)                                                //因为前面get_new_id()函数return了0
            {
                if (debug_level > 1)
                {
                    printf("登记失败，本地ID转换表已满！\n");
                }
            }
            else                                                            //假设请求到了新的id，注意这个id是位置+1得到的
            {
                memcpy(buf, &new_id, sizeof(unsigned short));               //将新id加入缓冲区
                length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
            //将域名查询请求发送至外部dns服务器(甚至没有判断内部有没有就直接往外发送了，这一段可以删了，重复书写)
            //这里是因为考虑到在内部找不到再发送请求是一件非常浪费时间的事情，不需要节约资源，节约时间
                if (debug_level > 1)
                {
                    printf("<转换过后新的域名: %s>\n\n", new_url);
                }
            }
            return;
        }
    }

    if (debug_level > 1)
        printf("\n\n---- 收到了来自客户端的消息： [IP:%s]----\n", inet_ntoa(client_addr.sin_addr));

    if (debug_level)    //只要调试等级不为0都需要打印时间戳
    {
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
    if (strcmp(table_result, "NOTFOUND") == 0 && strcmp(cache_result, "NOTFOUND") == 0) //若域名在本地文件、cache中均无法找到，需要上报外部dns服务器
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
            cache = move_to_head(cache, new_url);//将记录移到靠前的位置(LRU算法的实现)

            if (debug_level > 0)
            {
                if (debug_level > 1)
                    printf("从cache中查到\n");
                printf("<域名: %s , IP地址: %s>\n\n", new_url, ip);
                show_cache_flag = 1;
            }
        }

        char sendbuf[BUFFER_SIZE];
        memcpy(sendbuf, buf, length); //开始构造响应报文
        //构造头部
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

        if (strcmp(ip, "0.0.0.0") == 0)    //判断此ip是否应该被墙(你他妈不能写在一起？？？？因为写在一起就不能分段了)
        {

            num = htons(0x0);    //设置回答数为0
        }
        else
        {
            num = htons(0x1);	//否则设置回答数为1
        }
        memcpy(&sendbuf[6], &num, sizeof(unsigned short));
        //构造RR资源记录(响应报文没有Question部分，他妈的可以有QUESTION部分，怎么在理解的)
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

        //将头部和响应部分共同组成的DNS响应报文存入sendbuf，所以是有QUESTION部分的，响应报文就是请求报文的冗余
        memcpy(sendbuf + length, res_rec, sizeof(res_rec));

        length = sendto(my_socket, sendbuf, pos, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));
        //将构造好的报文段发给客户端

        if (length < 0 && debug_level>1)
        {
            printf("向客户端发包出现错误\n");
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


























