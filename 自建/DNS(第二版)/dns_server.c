#include"dns_server.h"

//定义全局变量
id_table ID_table[ID_TABLE_SIZE];	    //id表
SOCKET my_socket;                       //套接字
SOCKADDR_IN client_addr;                //客户端套接字地址结构体
SOCKADDR_IN server_addr;                //服务器套接字地址结构体
int debug_level = 0;                    //初始化debug等级
char server_ip[16] = "192.168.3.1";        //默认外部服务器ip地址
char file_path[100] = "dnsrelay.txt";   //默认配置文件路径
nodeptr inter_url_ip;                   //内存中保存的url-ip映射
nodeptr cache_url_ip;                   //cache中保存的url-ip映射


void handle_client_message(char *buf,int length,SOCKADDR_IN client_addr)
{
    int show_cache_flag = 0;    //标记是否需要打印cache
    char old_url[200];          //转换前的url
    char new_url[200];          //转换后的url
    int i = 0;
    char ip[100];
    char inter_result[200];
    char cache_result[200];
    char sendbuf[BUFFER_SIZE];
    int pos = 0;
    char res_rec[16];

    if (debug_level)
    {
        time_t t = time(NULL);
        char temp[64];
        strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
        printf("\n-----------------%s 收到来自本地客户端的请求-----------------\n", temp);
    }

    memcpy(old_url, &(buf[12]), length); //从请求报文的QNAME中获得url,HEADER长度为12字节，故直接访问buf数组的第12个元素即QNAME字段
    url_transform(old_url, new_url);     //将请求报文中的字符串格式的url转换成普通模式的url

    while (*(buf + 12 + i))
    {
        i++;                             //跳过长度为i的QNAME，现在buf+12+i代表的就是QTYPE
    }
    i++;

   if (buf + 12 + i != NULL)           //只要QTYPE字段不是空的就执行代码块
    {
        unsigned short type = ntohs(*(unsigned short*)(buf + 12 + i));  //获取TYPE字段
        if (type == 28&& strcmp(get_ip(inter_url_ip,new_url),"0.0.0.0")!=0)//ASCLL28对应的是文件分割符，且域名对应的ip并非非法ip(假如这个url都不存在我们可以确定它没被手动设置为非法)
        {
            unsigned short ID;                                            //声明ID标识符(用于处理和id有关的信息)
            memcpy(&ID, buf, sizeof(unsigned short));                     //获取客户端的id(因为是第一个信息直接获取buf即可)
            unsigned short new_id = id_transform(ID, client_addr, FALSE);   //将原id存入id映射表并获得新的id(为了后续服务器代理功能准备-这个顺序就该放在后面)

            if (new_id == 0)
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
                //将域名查询请求发送至外部dns服务器，原因就是为了确保我们能够得到真实的url-ip
            }
            return;
        }
    }

    strcpy(inter_result, get_ip(inter_url_ip,new_url));     //从映射表中查询url
    strcpy(cache_result, get_ip(cache_url_ip,new_url));     //从cache中查询url
    //从本地(内存或者chache)中查询是否有该记录
    if (strcmp(inter_result, "NOTFOUND") == 0 && strcmp(cache_result, "NOTFOUND") == 0) //若域名在内存、cache中均无法找到，需要向外部dns服务器请求帮助
    {
        printf("本地服务器查询<URL:%s>失败，即将向外部服务器发送请求\n\n",new_url);
        unsigned short ID;
        memcpy(&ID, buf, sizeof(unsigned short));
        unsigned short new_id = id_transform(ID, client_addr, FALSE); //将原客户端id进行转换得到新本地服务器id
        if (new_id == 0)                                               //假如id转换失败则报错并退出
        {
            if (debug_level)
            {
                printf("ID转换失败！\n");
            }
        }
        else                                                            //将域名查询请求发送至外部dns服务器
        {
            memcpy(buf, &new_id, sizeof(unsigned short));
            length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
            if (debug_level)
            {
                time_t t = time(NULL);
                char temp[64];
                strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
                printf("-----------------%s 向外部服务器发送请求-----------------\n", temp);
                printf("需要查询的域名为<URL:%s>\n",new_url);
            }
        }
    }
    else //若在本地查询到了域名和ip的映射
    {
        if (strcmp(inter_result, "NOTFOUND") != 0) //如果是从内存中查到
        {
            strcpy(ip, inter_result);
                if (debug_level){
                printf("从内存中查到<URL:%s>对应的IP，结果如下:\n",new_url);
                printf("<URL: %s , IP: %s>\n\n", new_url, ip);
                }
        }
        else //如果是从cache中查到
        {
            strcpy(ip, cache_result);
            cache_url_ip= move_to_head(cache_url_ip, new_url);//将记录移到靠前的位置(LRU算法的实现)

            if (debug_level)
            {
                printf("从cache中查到<URL:%s>对应的IP，结果如下:\n",new_url);
                printf("<URL: %s , IP: %s>\n\n", new_url, ip);
                show_cache_flag = 1;
            }
        }


        memcpy(sendbuf, buf, length);       //构造响应报文
        //1.构造头部
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

        if (strcmp(ip, "0.0.0.0") == 0)    //此ip是否合法
        {

            num = htons(0x0);               //假如非法直接不回答
        }
        else
        {
            num = htons(0x1);	            //合法则回答数为1
        }
        memcpy(&sendbuf[6], &num, sizeof(unsigned short));
        //2.构造RR资源记录
        unsigned short name = htons(0xc00c);
        memcpy(res_rec, &name, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TYPE字段
        unsigned short type = htons(0x0001);
        memcpy(res_rec + pos, &type, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //CLASS字段
        unsigned short _class = htons(0x0001);
        memcpy(res_rec + pos, &_class, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TTL字段
        unsigned long ttl = htonl(0x00000080);
        memcpy(res_rec + pos, &ttl, sizeof(unsigned long));
        pos += sizeof(unsigned long);

        //RDLENGTH字段
        unsigned short RDlength = htons(0x0004);
        memcpy(res_rec + pos, &RDlength, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //RDATA字段
        unsigned long IP = (unsigned long)inet_addr(ip);
        memcpy(res_rec + pos, &IP, sizeof(unsigned long));
        pos += sizeof(unsigned long);
        pos += length;

        //将HEADER和RR共同组成的DNS响应报文存入sendbuf准备发送(此处不考虑QUESTION部分)
        memcpy(sendbuf + length, res_rec, sizeof(res_rec));
        length = sendto(my_socket, sendbuf, pos, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));//将构造好的报文段发给客户端
        if (length < 0 && debug_level!=0)
        {
            time_t t = time(NULL);
            char temp[64];
            strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
            printf("-----------------%s向客户端发送报文出现错误-----------------\n", temp);
        }
        if (debug_level)
        {
            time_t t = time(NULL);
            char temp[64];
            strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
            printf("-----------------%s 向本地客户端发送响应-----------------\n", temp);
        }

    }

}


void handle_server_message(char *buf,int length,SOCKADDR_IN server_addr)
{
    char url[200];                              //声明用于保存url的数组
    if (debug_level)                            //只要调试等级不为0就打印时间戳
    {
        time_t t = time(NULL);
        char temp[64];
        strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
        printf("\n-----------------%s 收到来自外部服务器的响应-----------------\n", temp);
    }
    if (debug_level > 1)                                            //假如调试等级为2则展示冗余数据报信息
    {
        show_message(buf, length);
    }
    //1.解析第一部分，Header头部
    unsigned short ID;
    memcpy(&ID, buf, sizeof(unsigned short));                      //从接收报文(buf区中)中获取ID号
    int cur_id = ID - 1;                                            //为什么cur_id要-1?因为转换过后避免重复new_id还要+1(讲义的规定)
                                                                    //cur_id实际也就是本条id记录在id_table中的位置
    //从ID映射表查找原来的ID号
    memcpy(buf, &ID_table[cur_id].client_id, sizeof(unsigned short));//ID映射表中的old_id字段就是原来本地客户端的id号
    ID_table[cur_id].finished = TRUE;                               //将id映射表中的状态改了，表明该次向外部服务器发送的请求成功
    SOCKADDR_IN client_temp = ID_table[cur_id].client_addr;         //获取old_id映射表中的client_addr字段，也就是本地客户端的地址结构体，后续与它进行通信
    int num_query = ntohs(*((unsigned short*)(buf + 4)));          //QDCOUNT字段在第四个字节开始第六个字节结束,表示Question实体的数量,buf仅仅是缓冲数组的首地址
    int num_response = ntohs(*((unsigned short*)(buf + 6)));       //ANCOUNT字段在第六个字节开始第八个字节结束，表示Answer实体的数量
    char* p = buf + 12;                                             //移动指针,令p指向Question实体(12个字节的Header后面就是Question)

    //2.解析第二部分，Question
    for (int i = 0; i < num_query; i++)
    //将QUESTION的num_query个实体的url全部转换为正常的url，一般来说只会有一个QUESTION实体，所以此处不用num_query也行
    //multiple questions目前应该只是理论上存在，实际的服务器程序中应该都没有实现它
    {
        url_transform(p, url);                                      //将字符计数法表示的的域名转换为正常形式的域名

        while (*p > 0)
        {
            p += (*p) + 1;                                          //直到*p=0即指向字符串结尾\0
        }
        p += 5;                                                     //p=p+sizeof(char)*5（1字节\0，2字节QTYPE，2字节QCLASS）,p指向下一个QUESTION实体

    }


    if (num_response > 0 && debug_level)
    {
        printf("外部DNS服务器查询<URL:%s>成功，结果如下:\n", url);            //这个url就是我们需要查询的url
    }

    //3.解析第三部分，分析来自外部DNS服务器的ANSWER,其中ANSWER|AUTHORITY|ADDITIONAL的实体均为RR(这里只分析ANSWER)
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


            if (type == 1)
        {
            ip1 = (unsigned char)*p++;
            ip2 = (unsigned char)*p++;
            ip3 = (unsigned char)*p++;
            ip4 = (unsigned char)*p++;
            sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            if (debug_level)
            {
                printf("IP地址1： %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
            }

            insert_to_cache(url,ip);

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
                insert_to_cache(url, ip);
            }
            break;
        }
        else                                                    //若TYPE不为A则直接跳过获取DATAip的步骤
        {
            p += rdlength;
        }
    }
    //将报文发送给客户端
    length = sendto(my_socket, buf, length, 0, (SOCKADDR*)&client_temp, sizeof(client_temp));
    //响应消息
    if (debug_level)
        {
            time_t t = time(NULL);
            char temp[64];
            strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
            printf("\n-----------------%s 向本地客户端发送响应-----------------\n", temp);
        }
}


void handle_arguments(int argc, char* argv[])
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
            printf("指定DNS服务器的IP地址：  %s\n", server_ip);
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
    printf("debug=%d\n", debug_level);
}

void insert_to_inter()
{
    FILE* file;
    if ((file = fopen(file_path, "r")) == NULL)                 //从file_path读取文件失败
    {
        if (debug_level)
            printf("读取配置文件失败\n");
        return;
    }

    char url[65] = "", ip[16] = "";

    if (debug_level)
        printf("加载配置文件成功,结果如下:\n");
    while (fscanf(file, "%s %s", ip, url) > 0)
    {
        if (debug_level)
        {
            printf("<域名: %s, IP地址 : %s>\n", url, ip);
        }
        inter_url_ip = push_front(inter_url_ip, url, ip);       //添加至内存中
    }

    fclose(file);
}

void initialize_id_table()
{
    for (int i = 0; i < ID_TABLE_SIZE; i++)
    {//因为ID映射表是一个结构体，所以需要递归索引进行初始化
        ID_table[i].client_id= 0;
        ID_table[i].finished = TRUE;
        ID_table[i].survival_time = 0;
        memset(&(ID_table[i].client_addr), 0, sizeof(SOCKADDR_IN));
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
    if(debug_level)
        printf("socket连接建立成功！\n");

    //将发送、接受模式设定为非阻塞
    int non_block = 1;

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
    if (bind(my_socket, (struct sockaddr*) & client_addr, sizeof(client_addr)) < 0)
    {
        printf("socket端口绑定失败！\n");
        exit(1);
    }

    if (debug_level)
        printf("socket端口绑定成功！\n");
}

void show_message(char* buf, int len)
{
    unsigned char byte;
    if(debug_level > 1){
         printf("报文内容如下:\n");
    }

    for (int i = 0; i < len;)
    {
        byte = (unsigned char)buf[i];
        printf("%02x ", byte);
        i++;
        if (i % 16 == 0)    //每行只打印16字节
            printf("\n");
    }
    if(debug_level > 1){
       printf("\n报文总长度为%d\n\n", len);
    }
}

void url_transform(char* buf, char* result)
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

void insert_to_cache(char* url, char *ip)
{
    //先判断cache中是否有该元素
    char tmp[100];                        //声明并初始化temp缓冲区
    memset(tmp, '\0', sizeof(tmp));      //清空tmp缓冲区
    strcpy(tmp, get_ip(cache_url_ip, url));      //根据url查找链表结点，返回对应的ip地址，并写入tmp缓冲区(list操作)

    if (strcmp(tmp, "NOTFOUND") != 0)    //若Cache中存在该域名，那我们只需加进去或者什么都不做就行
    {
        nodeptr cur=cache_url_ip;
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
        strcpy(cache_url_ip->ip[cur->num++], ip);//否则更新加入url-ip映射关系
    }
    else                                 //cache中不存在该域名
    {
        if (size(cache_url_ip) >= 8)            //当cache大小为8认为已满
        {
            cache_url_ip = pop_back(cache_url_ip);     //先删除单链表最后一个结点
        }
        cache_url_ip = push_front(cache_url_ip, url, ip);//再加入新的url-ip映射关系(注意这个版本并没有用LRU算法)
        if(debug_level>1){
            printf("插入url-ip映射至cache成功，cache中的映射如下：\n");
            show_cache_url_ip();
        }

    }

}

unsigned short id_transform(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished)
{
    int i = 0;
    for (i = 0; i != ID_TABLE_SIZE; ++i)//只有当id转换表没有满的时候才能存入客户端id
    {
        //存入客户端的之前，需要先找到一个合适的位置(假如找不到合适的位置就一直循环就行了慢慢找，因为不满所以肯定找得到)
        if ((ID_table[i].survival_time > 0 &&  ID_table[i].survival_time < time(NULL) )
            || ID_table[i].finished == TRUE)                 //合适的位置：指超时失效或者已完成的位置
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

void show_cache_url_ip()
{
    printf("\ncache中的URL-IP如下\n");
    print_list(cache_url_ip);
}
