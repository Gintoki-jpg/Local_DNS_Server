int main(int argc, char* argv[])
{
    printf("�����ʽ: dnsrelay [ -d | -dd ] [ <������IP��ַ> ] [ <�����ļ�·��> ]\n");

    //show_hint_info();//����������Ϣ

    get_arguments(argc, argv);       //��ȡ�û����������ʼ��������Ϣ���ⲿDNS������IP��ַ�Լ������ļ�·��

    url_ip_table = create_list();    //���ڴ��д���url-ipӳ���(������)

    cache = create_list();           //��cache�д���url-ipӳ���(������)

    get_url_ip_map();                //��ȡ�����ļ�dnsrelay.txt�������ڴ��url-ipӳ�����


    initialize_id_table();           //��ʼ��IDӳ���

    WSADATA wsaData;                 //�洢Socket���ʼ����Ϣ

    WSAStartup(MAKEWORD(2, 2), &wsaData);  //���ݰ汾֪ͨ����ϵͳ������SOCKET�Ķ�̬���ӿ�

    initialize_socket();             //��ʼ���׽���

    char buf[BUFFER_SIZE];          //����������,���������ͨ�ڻ������Կͻ��˺������ⲿ�������ı���
    SOCKADDR_IN tmp_sockaddr;       //��ʼ��һ�����õ�ַ�ṹ��SOCKADDR_IN(ע��ͨ�õ�ַ�ṹ��Ϊsockaddr)
    int length;                     //�������ݱ����ַ��������ȣ�
    int sockaddr_in_size = sizeof(SOCKADDR_IN);//����ͨ�õ�ַ�ṹ�峤��

    while (TRUE)                    //����ѭ��ʵ�ַ�������������
    {
        memset(buf, '\0', BUFFER_SIZE);//��ձ��ط������Ļ�����
        /*
        ʹ��recvfrom()�����ӿͻ��˽���������
        recvfrom()����������һ�����ݱ��������ݴ���buf�У�������Դ��ַ�ṹ��
            @local_socket�������ӵı����׽ӿ�
            @buf:�������ݻ�����
            @sizeof(buf)�����ջ�������С
            @0����־λflag����ʾ���ò�����ʽ��Ĭ����Ϊ0
            @client:���񵽵����ݷ���Դ��ַ��Socket��ַ��
            @sockaddr_in_size:��ַ����
            ����ֵ��recvLength���ɹ����յ������ݵ��ַ��������ȣ�������ʧ�ܷ���SOCKET_ERROR(-1)
        */

        //����������һ������ֱ���յ�����
        length = -1;                                        //�������ݱ�����(����ȷʵӦ��ÿ��ѭ����Ҫ����)
        length= recvfrom(my_socket, buf, sizeof(buf), 0, (struct sockaddr*) & tmp_sockaddr, &sockaddr_in_size);//��recvfrom�����ķ���ֵ��length����

        if (length > 0)                                     //���ݱ��ĳ�����������ʾ�յ�������
        {
            if (tmp_sockaddr.sin_port == htons(53))         //����˵������ݱ���Դ��ַ�˿ں�Ϊ53����ȷ�����������ⲿDNS������������(���ǹ涨)
            {
                process_server_request(buf, length, tmp_sockaddr);//�����ⲿ���������ݲ����к�������
            }
            else
            {
                process_client_request(buf, length, tmp_sockaddr); //���ձ��ؿͻ������ݲ����к�������
            }
        }
    }
    return 0;
}

//1.��ȡ�û������������õ��Եȼ����������û�����ָ���ⲿDNS��������IP��ַ�Լ������ļ�·��
//dnsrelay [-d | -dd] [dns-server-ipaddr] [filename]  �û����������ĸ�����
void get_arguments(int argc, char* argv[])
{
    bool is_assigned = false;   //Ĭ���û���ʹ�õ�Ĭ������

    if (argc > 1 && argv[1][0] == '-')
    {
        if (argv[1][1] == 'd')
        {
            debug_level=1;      //���Եȼ���1
        }
        if (argv[1][2] == 'd')
        {
            debug_level=2;      //���Եȼ���2
        }
        if (argc > 2)           //��������������2������˵�û���ָ�����ⲿDNS��������IP��ַ
        {
            is_assigned = true; //����û�����������
            strcpy(server_ip, argv[2]);//ʹ��ָ���ⲿDNS������IP��ַ
            printf("ָ��DNS������IP��ַ��  %s\n", server_ip);
            if (argc == 4)	    //����ָ�����ⲿDNS��������IP��ַ����ָ���������ļ���·��
            {
                strcpy(file_path, argv[3]);
            }
        }
    }

    if(!is_assigned) //δָ���ⲿ������������ΪĬ�Ϸ�����
    {
        printf("ʹ��Ĭ��DNS������IP��ַ��  %s \n", server_ip);
    }
    printf("���Եȼ��� %d\n", debug_level);
}

//2.���������ļ���ȡurl-ipӳ������������ڴ���
void get_url_ip_map()
{
    FILE* file;
    if ((file = fopen(file_path, "r")) == NULL)                 //��file_path��ȡ�ļ�ʧ��
    {
        if (debug_level > 1)
            printf("��ȡ�����ļ�ʧ��\n");
        return;
    }

    char url[65] = "", ip[16] = "";

    if (debug_level > 1)
        printf("���������ļ�:\n");
    while (fscanf(file, "%s %s", ip, url) > 0)
    {
        if (debug_level > 1)
        {
            printf("<����: %s, IP��ַ : %s>\n", url, ip);
        }
        url_ip_table = push_front(url_ip_table, url, ip);       //�����url-ipӳ���(������)��
    }

    fclose(file);
}


//3.��ʼ��idӳ���
void initialize_id_table()
{
    for (int i = 0; i < ID_TABLE_SIZE; i++)
    {//��ΪIDӳ�����һ���ṹ�壬������Ҫ�ݹ��������г�ʼ��
        ID_table[i].old_id = 0;
        ID_table[i].finished = TRUE;
        ID_table[i].survival_time = 0;
        memset(&(ID_table[i].client_addr), 0, sizeof(SOCKADDR_IN));
    }
}

//4.��ʼ���׽�����Ϣ
void initialize_socket()
{
    my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    //�����׽���ʧ�ܣ����˳�����
    if (my_socket < 0)
    {
        printf("���ӽ���ʧ�ܣ�\n");
        exit(1);
    }
    if(debug_level>1)
        printf("���ӽ����ɹ���\n");

    //�����͡�����ģʽ�趨Ϊ������
    int non_block = 1;
    //ioctlsocket(my_socket, FIONBIO, (u_long FAR*) & non_block);

    client_addr.sin_family = AF_INET;            //IPv4
    client_addr.sin_addr.s_addr = INADDR_ANY;    //����ip��ַ���
    client_addr.sin_port = htons(53);      //�󶨵�53�˿�

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);//�󶨵��ⲿ������
    server_addr.sin_port = htons(53);

    /* ���ض˿ںţ�ͨ�ö˿ںţ������� */
    int reuse = 0;
    setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    //���˿ں���socket����
    if (bind(my_socket, (struct sockaddr*) & client_addr, sizeof(client_addr)) < 0)
    {
        printf("�׽��ֶ˿ڰ�ʧ�ܡ�\n");
        exit(1);
    }

    if (debug_level > 1)
        printf("�׽��ֶ˿ڰ󶨳ɹ���\n");
}

//5.���Եȼ�Ϊ2�����
void show_packet(char* buf, int len)
{
    unsigned char byte;
    printf("���ݰ�����:\n");
    for (int i = 0; i < len;)
    {
        byte = (unsigned char)buf[i];
        printf("%02x ", byte);
        i++;
        if (i % 16 == 0)    //ÿ��ֻ��ӡ16�ֽ�
            printf("\n");
    }
    printf("\n���ݰ��ܳ��� = %d\n\n", len);
}

//6.���ַ�����������ʮ������"�ַ�����"ǰ׺��urlת��Ϊ������ʽ������(ֱ������\0����Ϊֹ)
//��������buf�ڴ���Ҫת����urlΪ"03api.04sina.o3com.02cn.00\0" ע�����������ȫ����ʮ��������
void transform_url(char* buf, char* result)
{
    int i = 0, j = 0, k = 0, len = strlen(buf);
    while (i < len)
    {
        if (buf[i] > 0 && buf[i] <= 63)                 //����ASCLL��0-63��ʮ������ǰ׺���ڶ�������ʾ��ǩ�ĳ���
        {
            for (j = buf[i], i++; j > 0; j--, i++, k++) //���ַ��������е�url��ȡ��������result��
            {
                result[k] = buf[i];
            }
        }
        if (buf[i] != 0)                                //�����û��������û����\0���ţ����"."��url�ָ���
        {
            result[k] = '.';
            k++;
        }
    }
    result[k] = '\0';                                   //��ת����ɵ�url��ӽ����������ǵõ�"api.sina.com.cn\0"
}

//7.���µ�ӳ���ϵ��¼����cache
void insert_record_to_cache(char* url, char *ip)
{
    //���ж�cache���Ƿ��и�Ԫ��
    char tmp[100];                        //��������ʼ��temp������
    memset(tmp, '\0', sizeof(tmp));      //���tmp������
    strcpy(tmp, get_ip(cache, url));      //����url���������㣬���ض�Ӧ��ip��ַ����д��tmp������(list����)

    if (strcmp(tmp, "NOTFOUND") != 0)    //��Cache�д��ڸ�������������ֻ��ӽ�ȥ����ʲô����������
    {
        nodeptr cur=cache;
        while(strcmp(cur->url,url)!=0)
        {
            cur=cur->next;
        }
        for(int i=0;i<cur->num;i++)
        {
            if(strcmp(cur->ip[i],ip)==0)  //��url-ipӳ���ϵ��cache���ҵ�����ʲô������
            {
                return;
            }
        }
        if(cur->num>20)                   //һ��url����Ӧ20��ip������ֱ���˳�
        {
            return;
        }
        strcpy(cache->ip[cur->num++], ip);//������¼���url-ipӳ���ϵ
    }
    else                                 //cache�в����ڸ�����
    {
        if (size(cache) >= 8)            //��cache��СΪ8��Ϊ����
        {
            cache = pop_back(cache);     //��ɾ�����������һ�����
        }
        cache = push_front(cache, url, ip);//�ټ����µ�url-ipӳ���ϵ(ע������汾��û����LRU�㷨)
        if(debug_level>1)                  //�����Եȼ�����2�����������Ϣ
            show_cache();
    }

}

//8.���������ⲿ�������ı���
void process_server_request(char *buf,int length,SOCKADDR_IN server_addr)
{
    char url[200];                              //�������ڱ���url������

    if (debug_level > 1)                        //������Եȼ�Ϊ2��չʾ�������ݱ���Ϣ
    {
        show_packet(buf, length);
    }

    //��һ���֣�Headerͷ��
    unsigned short ID;
    memcpy(&ID, buf, sizeof(unsigned short));                      //�ӽ��ձ���(buf����)�л�ȡID��
    int cur_id = ID - 1;                                            //Ϊʲôcur_idҪ-1?��Ϊ���ǵı��ط��������Զ���ת�������id+1(����Ĺ涨)
                                                                    //cur_idʵ��ҳ���Ǳ���id��¼��id_table�е�λ��

    //��IDӳ������ԭ����ID��
    memcpy(buf, &ID_table[cur_id].old_id, sizeof(unsigned short));//IDӳ����е�old_id�ֶξ���ԭ�����ؿͻ��˵�id��
    ID_table[cur_id].finished = TRUE;                               //��idӳ����е�״̬���ˣ������ô����ⲿ���������͵�����ɹ�

    SOCKADDR_IN client_temp = ID_table[cur_id].client_addr;         //��ȡold_idӳ����е�client_addr�ֶΣ�Ҳ���Ǳ��ؿͻ��˵ĵ�ַ�ṹ�壬������������ͨ��

    int num_query = ntohs(*((unsigned short*)(buf + 4)));          //QDCOUNT�ֶ��ڵ��ĸ��ֽڿ�ʼ�������ֽڽ���,��ʾQuestionʵ�������,buf�����ǻ���������׵�ַ
    int num_response = ntohs(*((unsigned short*)(buf + 6)));       //ANCOUNT�ֶ��ڵ������ֽڿ�ʼ�ڰ˸��ֽڽ�������ʾAnswerʵ�������

    char* p = buf + 12;                                             //�ƶ�ָ��,��pָ��Questionʵ��(12���ֽڵ�Header�������Question)

    //�ڶ����֣�Question����
    for (int i = 0; i < num_query; i++)
    //��QUESTION��num_query��ʵ���urlȫ��ת��Ϊ������url��һ����˵ֻ����һ��QUESTIONʵ�壬���Դ˴�����num_queryҲ��
    //multiple questionsĿǰӦ��ֻ�������ϴ��ڣ�ʵ�ʵķ�����������Ӧ�ö�û��ʵ����
    {
        transform_url(p, url);                                      //���ַ���������ʾ�ĵ�����ת��Ϊ������ʽ������

        while (*p > 0)
        {
            p += (*p) + 1;                                          //ֱ��*p=0��ָ���ַ�����β\0
        }
        p += 5;                                                     //p=p+sizeof(char)*5��1�ֽ�\0��2�ֽ�QTYPE��2�ֽ�QCLASS��,pָ����һ��QUESTIONʵ��

    //url����ֻ�ᱣ�����һ�ν���������url����Ϊʵ���ϲ��ܷ����ٸ�Question��ÿ�η���һ�������ģ��ͽ���ֻ�ǲ�ѯһ��url����
    }

    if (num_response > 0 && debug_level > 1)                        //���Եȼ�Ϊ2���������Ϣ
    {
        printf("�ⲿDNS��������ѯ��<Url : %s>�������Ϣ����\n", url); //���url����������Ҫ��ѯ��url
    }

    //�������֣����������ⲿDNS��������ANSWER,����ANSWER|AUTHORITY|ADDITIONAL��ʵ���ΪRR(����ֻ����ANSWER)
    for (int i = 0; i < num_response; ++i)                           //�������е�ANSWER
    {
        if ((unsigned char)*p == 0xc0)                              //NAME�ֶ�ʹ��ָ��ƫ�Ʊ�ʾ��ֻռ�����ֽ�(�涨)
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

        //��ȡANSWER��Դ��¼��(RR)�ĸ����ֶ�(����Ҫ��ȡNAME����ΪNAME����ǰ���ȡ��url)
        unsigned short type = ntohs(*(unsigned short*)p);           //TYPE�ֶ�
        p += sizeof(unsigned short);
        unsigned short _class = ntohs(*(unsigned short*)p);         //CLASS�ֶ�
        p += sizeof(unsigned short);
        unsigned short ttl_high_byte = ntohs(*(unsigned short*)p); //TTL��λ�ֽ�
        p += sizeof(unsigned short);
        unsigned short ttl_low_byte = ntohs(*(unsigned short*)p);  //TTL��λ�ֽ�
        p += sizeof(unsigned short);
        int ttl = (((int)ttl_high_byte) << 16) | ttl_low_byte;       //��TTL��λ�͵�λ�ϲ�Ϊint��������

        int rdlength = ntohs(*(unsigned short*)p);                  //ǰ�漸���ֶ�(TYPE CLASS TTL)�ĳ��ȣ��˴����˻�������������DATALENGTH

        p += sizeof(unsigned short);                                //TYPE A����Դ����Value��4�ֽڵ�IP��ַ,��RDATA�ֶε��ֽ���Ϊ4������pָ��DATA�ֶ�

        if (debug_level > 1)                                          //���Եȼ�Ϊ2���������Ϣ
        {
            printf("TYPE: %d,  CLASS: %d,  TTL: %d\n", type, _class, ttl);
        }

        char ip[16];                                                  //��С����8�Ϳ���
        int ip1, ip2, ip3, ip4;                                       //��Ϊÿ��ip�ֶ�ռһ���ֽڣ�����������Ҫ���ĸ�ip�������գ����ƴ��


        if (type == 1)                                          // ����ֻ����ANSWER��TYPEΪA��RR�е�RDATA�ֶ�
        {
            ip1 = (unsigned char)*p++;
            ip2 = (unsigned char)*p++;
            ip3 = (unsigned char)*p++;
            ip4 = (unsigned char)*p++;
            sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);     //���뵽ָ���ַ���ip��

            if (debug_level>1)                                  //���Եȼ�Ϊ2���������Ϣ
            {
                printf("IP��ַ�� %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
            }

            insert_record_to_cache(url,ip);                     //������������url-ipӳ����뱾��DNS��������Cache��

            while(p&&*(p+1)&&*(p+=12))                          //���ip��Ӧ
            {
                ip1=(unsigned char)*p++;
                ip2=(unsigned char)*p++;
                ip3=(unsigned char)*p++;
                ip4=(unsigned char)*p++;
                sprintf(ip,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);

                if(debug_level>1)
                {
                    printf("IP��ַ�� %d.%d.%d.%d\n",ip1,ip2,ip3,ip4);
                }


                insert_record_to_cache(url, ip);                 //��ӳ����뵽cacheӳ���
            }
            break;
        }
        else                                                    //��TYPE��ΪA��ֱ��������ȡDATAip�Ĳ���
        {
            p += rdlength;
        }
    }
 /*
    �����sendto()���������ͻؿͻ���
    sendto()����������һ�����ݱ���������buf�У���Ҫ�ṩ��ַ
        @local_socket���ⲿ�������׽���
        @buf���������ݻ�����
        @sizeof(buf)�����ջ�������С
        @0����־λflag����ʾ���ò�����ʽ��Ĭ����Ϊ0
        @client��Ŀ�����ݷ���Դ��ַ
        @sizeof(client)����ַ��С
        ����ֵ��length���ɹ����յ������ݵ��ַ��������ȣ�������ʧ�ܷ���SOCKET_ERROR(-1)
    */
    length = sendto(my_socket, buf, length, 0, (SOCKADDR*)&client_temp, sizeof(client_temp));
}

//9.��Ҫ���ⲿDNS��������������ʱ����Ҫ����ԭid��Ϣ
unsigned short get_new_id(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished)
{
    int i = 0;
    for (i = 0; i != ID_TABLE_SIZE; ++i)//ֻ�е�idת����û������ʱ����ܴ���ͻ���id
    {
        //����ͻ��˵�֮ǰ����Ҫ���ҵ�һ�����ʵ�λ��(�����Ҳ������ʵ�λ�þ�һֱѭ�������������ң���Ϊ�������Կ϶��ҵõ�)
        if ((ID_table[i].survival_time > 0 &&  ID_table[i].survival_time < time(NULL) )
            || ID_table[i].finished == TRUE)                 //���ʵ�λ�ã�ָ��ʱʧЧ��������ɵ�λ��
        {
            if (ID_table[i].old_id != i + 1)                //ȷ���¾�ID��ͬ������+1����
            {
                ID_table[i].old_id = ID;                    //����ID
                ID_table[i].client_addr = client_addr;      //����ͻ����׽���
                ID_table[i].finished = finished;            //��Ǹÿͻ��˵������Ƿ��ѯ�Ѿ����
                ID_table[i].survival_time = time(NULL) + 5; //����ʱ������Ϊ5��

                //printf("\nԭID��%d",ID);
                //printf("\n��ID��%d", i);                    //�µ�idΪiҲ������ID_table�е�λ�ã���û��ʹ��ʲô����㷨
                break;
            }
        }
    }
    if (i == ID_TABLE_SIZE)                                 //�Ǽ�ʧ�ܣ���ǰidת��������
    {
        return 0;
    }
    return (unsigned short)i + 1;                         //����ID�ţ�ע�����ID����+1����ģ�ԭ��δ֪
}

//10.�������Կͻ��˵���Ϣ(���Կͻ��˵������ĺ���Ӧ���ĸ�ʽ����һ�£�QUESTION����Ϊ1�����������ֶ�һ����˵������)
//��ʵ����ͻ��˵ĺ���Ӧ����д����������⴦��������Ĵ���
void process_client_request(char *buf,int length,SOCKADDR_IN client_addr)
{
    int show_cache_flag = 0;    //����Ƿ���Ҫ��ӡcache
    char old_url[200];          //ת��ǰ��url
    char new_url[200];          //ת�����url

    memcpy(old_url, &(buf[12]), length); //�������ĵ�QNAME�л��url,HEADER����Ϊ12�ֽڣ�����ֱ�ӷ���buf����ĵ�12��Ԫ�ؼ�QNAME�ֶ�
    transform_url(old_url, new_url);     //���������е��ַ�����ʽ��urlת������ͨģʽ��url�����������Ҳ����ʹ�����ֻ��ƣ���Ϊʵ����ֻ��һ��url

    int i = 0;

    while (*(buf + 12 + i))
    {
        i++;                             //��������Ϊi��QNAME������buf+12+i����ľ���QTYPE
    }
    i++;

    if (buf + 12 + i != NULL)           //ֻҪQTYPE�ֶβ��ǿյľ�ִ��һ������ĺ�����
    {
        unsigned short type = ntohs(*(unsigned short*)(buf + 12 + i));  //��ȡTYPE�ֶ�
        if (type == 28&& strcmp(get_ip(url_ip_table,new_url),"0.0.0.0")!=0)//ASCLL28��Ӧ�����ļ��ָ������������Ӧ��ip���ǷǷ�ip(�������url�����������ǿ���ȷ����û���ֶ�����Ϊ�Ƿ�)
        {
            unsigned short ID;                                            //����ID��ʶ��(���ڴ����id�йص���Ϣ)
            memcpy(&ID, buf, sizeof(unsigned short));                     //��ȡ�ͻ��˵�id(��Ϊ�ǵ�һ����Ϣֱ�ӻ�ȡbuf����)
            unsigned short new_id = get_new_id(ID, client_addr, FALSE);   //��ԭid����idӳ�������µ�id(Ϊ�˺���������������׼��-���˳��͸÷��ں���)

            if (new_id == 0)                                                //��Ϊǰ��get_new_id()����return��0
            {
                if (debug_level > 1)
                {
                    printf("�Ǽ�ʧ�ܣ�����IDת����������\n");
                }
            }
            else                                                            //�����������µ�id��ע�����id��λ��+1�õ���
            {
                memcpy(buf, &new_id, sizeof(unsigned short));               //����id���뻺����
                length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
            //��������ѯ���������ⲿdns������(����û���ж��ڲ���û�о�ֱ�����ⷢ���ˣ���һ�ο���ɾ�ˣ��ظ���д)
            //��������Ϊ���ǵ����ڲ��Ҳ����ٷ���������һ���ǳ��˷�ʱ������飬����Ҫ��Լ��Դ����Լʱ��
                if (debug_level > 1)
                {
                    printf("<ת�������µ�����: %s>\n\n", new_url);
                }
            }
            return;
        }
    }

    if (debug_level > 1)
        printf("\n\n---- �յ������Կͻ��˵���Ϣ�� [IP:%s]----\n", inet_ntoa(client_addr.sin_addr));

    if (debug_level)    //ֻҪ���Եȼ���Ϊ0����Ҫ��ӡʱ���
    {
        time_t t = time(NULL);
        char temp[64];
        strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
        printf("%s\t#%d\n", temp,count++);
    }

    char ip[100];
    char table_result[200];
    char cache_result[200];
    strcpy(table_result, get_ip(url_ip_table,new_url));     //��ӳ����в�ѯ����ip��ַ���
    strcpy(cache_result, get_ip(cache, new_url));           //��cache�в�ѯ����IP��ַ���

    //�ӱ���ӳ����в�ѯ�Ƿ��иü�¼
    if (strcmp(table_result, "NOTFOUND") == 0 && strcmp(cache_result, "NOTFOUND") == 0) //�������ڱ����ļ���cache�о��޷��ҵ�����Ҫ�ϱ��ⲿdns������
    {

        unsigned short ID;
        memcpy(&ID, buf, sizeof(unsigned short));
        unsigned short new_id = get_new_id(ID, client_addr, FALSE); //��ԭid����idת����

        if (new_id == 0)
        {
            if (debug_level > 1)
            {
                printf("�Ǽ�ʧ�ܣ�����IDת����������\n");
            }
        }
        else
        {
            memcpy(buf, &new_id, sizeof(unsigned short));
            length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
            //��������ѯ���������ⲿdns������
            if (debug_level > 0)
            {
                printf("<����: %s>\n\n", new_url);
            }
        }
    }
    else //���ڱ��ز�ѯ����������ip��ӳ��
    {
        if (strcmp(table_result, "NOTFOUND") != 0) //����Ǵ�ӳ����в鵽
        {
            strcpy(ip, table_result);
            if (debug_level > 0)
            {
                if (debug_level > 1)
                    printf("��ӳ����в鵽\n");
                printf("<����: %s , IP��ַ: %s>\n\n", new_url, ip);
            }
        }
        else //����Ǵ�cache�в鵽
        {
            strcpy(ip, cache_result);
            cache = move_to_head(cache, new_url);//����¼�Ƶ���ǰ��λ��(LRU�㷨��ʵ��)

            if (debug_level > 0)
            {
                if (debug_level > 1)
                    printf("��cache�в鵽\n");
                printf("<����: %s , IP��ַ: %s>\n\n", new_url, ip);
                show_cache_flag = 1;
            }
        }

        char sendbuf[BUFFER_SIZE];
        memcpy(sendbuf, buf, length); //��ʼ������Ӧ����
        //����ͷ��
        unsigned short num;

        if (strcmp(ip, "0.0.0.0") == 0)    //�жϴ�ip�Ƿ�Ӧ�ñ�ǽ
        {
            num = htons(0x8183);
            memcpy(&sendbuf[2], &num, sizeof(unsigned short));
        }
        else
        {
            num = htons(0x8180);
            memcpy(&sendbuf[2], &num, sizeof(unsigned short));
        }

        if (strcmp(ip, "0.0.0.0") == 0)    //�жϴ�ip�Ƿ�Ӧ�ñ�ǽ(�����費��д��һ�𣿣�������Ϊд��һ��Ͳ��ֶܷ���)
        {

            num = htons(0x0);    //���ûش���Ϊ0
        }
        else
        {
            num = htons(0x1);	//�������ûش���Ϊ1
        }
        memcpy(&sendbuf[6], &num, sizeof(unsigned short));
        //����RR��Դ��¼(��Ӧ����û��Question���֣�����Ŀ�����QUESTION���֣���ô������)
        int pos = 0;
        char res_rec[16];

        //NAME�ֶΣ���Դ��¼�е�����ͨ���ǲ�ѯ���ⲿ�ֵ��������ظ�������ʹ��2�ֽڵ�ƫ��ָ���ʾ
        //���2λΪ11������ʶ��ָ�룬�����λΪ1100��12����ʾͷ������
        //��˸��ֶε�ֵΪ0xC00C
        unsigned short name = htons(0xc00c);
        memcpy(res_rec, &name, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TYPE�ֶΣ���Դ��¼�����ͣ�1��ʾIPv4��ַ
        unsigned short type = htons(0x0001);
        memcpy(res_rec + pos, &type, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //CLASS�ֶΣ���ѯ�࣬ͨ��Ϊ1��IN��������Internet����
        unsigned short _class = htons(0x0001);
        memcpy(res_rec + pos, &_class, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TTL�ֶΣ���Դ��¼����ʱ�䣨���޸ģ�
        unsigned long ttl = htonl(0x00000080);
        memcpy(res_rec + pos, &ttl, sizeof(unsigned long));
        pos += sizeof(unsigned long);

        //RDLENGTH�ֶΣ���Դ���ݳ��ȣ�������1��TYPE A��¼����Դ������4�ֽڵ�IP��ַ
        unsigned short RDlength = htons(0x0004);
        memcpy(res_rec + pos, &RDlength, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //RDATA�ֶΣ�������һ��IP��ַ
        unsigned long IP = (unsigned long)inet_addr(ip);
        memcpy(res_rec + pos, &IP, sizeof(unsigned long));
        pos += sizeof(unsigned long);
        pos += length;

        //��ͷ������Ӧ���ֹ�ͬ��ɵ�DNS��Ӧ���Ĵ���sendbuf����������QUESTION���ֵģ���Ӧ���ľ��������ĵ�����
        memcpy(sendbuf + length, res_rec, sizeof(res_rec));

        length = sendto(my_socket, sendbuf, pos, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));
        //������õı��Ķη����ͻ���

        if (length < 0 && debug_level>1)
        {
            printf("��ͻ��˷������ִ���\n");
        }

        char* p;
        p = sendbuf + length - 4;
        if (debug_level > 1)
        {
            printf("�������ݱ� <����: %s ��IP: %u.%u.%u.%u>\n", new_url, (unsigned char)*p, (unsigned char)*(p + 1), (unsigned char)*(p + 2), (unsigned char)*(p + 3));
        }
        if (show_cache_flag && debug_level>1)
        {
            show_cache();
        }
    }

}


























