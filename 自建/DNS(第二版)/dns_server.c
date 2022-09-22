#include"dns_server.h"

//����ȫ�ֱ���
id_table ID_table[ID_TABLE_SIZE];	    //id��
SOCKET my_socket;                       //�׽���
SOCKADDR_IN client_addr;                //�ͻ����׽��ֵ�ַ�ṹ��
SOCKADDR_IN server_addr;                //�������׽��ֵ�ַ�ṹ��
int debug_level = 0;                    //��ʼ��debug�ȼ�
char server_ip[16] = "192.168.3.1";        //Ĭ���ⲿ������ip��ַ
char file_path[100] = "dnsrelay.txt";   //Ĭ�������ļ�·��
nodeptr inter_url_ip;                   //�ڴ��б����url-ipӳ��
nodeptr cache_url_ip;                   //cache�б����url-ipӳ��


void handle_client_message(char *buf,int length,SOCKADDR_IN client_addr)
{
    int show_cache_flag = 0;    //����Ƿ���Ҫ��ӡcache
    char old_url[200];          //ת��ǰ��url
    char new_url[200];          //ת�����url
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
        printf("\n-----------------%s �յ����Ա��ؿͻ��˵�����-----------------\n", temp);
    }

    memcpy(old_url, &(buf[12]), length); //�������ĵ�QNAME�л��url,HEADER����Ϊ12�ֽڣ���ֱ�ӷ���buf����ĵ�12��Ԫ�ؼ�QNAME�ֶ�
    url_transform(old_url, new_url);     //���������е��ַ�����ʽ��urlת������ͨģʽ��url

    while (*(buf + 12 + i))
    {
        i++;                             //��������Ϊi��QNAME������buf+12+i����ľ���QTYPE
    }
    i++;

   if (buf + 12 + i != NULL)           //ֻҪQTYPE�ֶβ��ǿյľ�ִ�д����
    {
        unsigned short type = ntohs(*(unsigned short*)(buf + 12 + i));  //��ȡTYPE�ֶ�
        if (type == 28&& strcmp(get_ip(inter_url_ip,new_url),"0.0.0.0")!=0)//ASCLL28��Ӧ�����ļ��ָ������������Ӧ��ip���ǷǷ�ip(�������url�����������ǿ���ȷ����û���ֶ�����Ϊ�Ƿ�)
        {
            unsigned short ID;                                            //����ID��ʶ��(���ڴ����id�йص���Ϣ)
            memcpy(&ID, buf, sizeof(unsigned short));                     //��ȡ�ͻ��˵�id(��Ϊ�ǵ�һ����Ϣֱ�ӻ�ȡbuf����)
            unsigned short new_id = id_transform(ID, client_addr, FALSE);   //��ԭid����idӳ�������µ�id(Ϊ�˺���������������׼��-���˳��͸÷��ں���)

            if (new_id == 0)
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
                //��������ѯ���������ⲿdns��������ԭ�����Ϊ��ȷ�������ܹ��õ���ʵ��url-ip
            }
            return;
        }
    }

    strcpy(inter_result, get_ip(inter_url_ip,new_url));     //��ӳ����в�ѯurl
    strcpy(cache_result, get_ip(cache_url_ip,new_url));     //��cache�в�ѯurl
    //�ӱ���(�ڴ����chache)�в�ѯ�Ƿ��иü�¼
    if (strcmp(inter_result, "NOTFOUND") == 0 && strcmp(cache_result, "NOTFOUND") == 0) //���������ڴ桢cache�о��޷��ҵ�����Ҫ���ⲿdns�������������
    {
        printf("���ط�������ѯ<URL:%s>ʧ�ܣ��������ⲿ��������������\n\n",new_url);
        unsigned short ID;
        memcpy(&ID, buf, sizeof(unsigned short));
        unsigned short new_id = id_transform(ID, client_addr, FALSE); //��ԭ�ͻ���id����ת���õ��±��ط�����id
        if (new_id == 0)                                               //����idת��ʧ���򱨴��˳�
        {
            if (debug_level)
            {
                printf("IDת��ʧ�ܣ�\n");
            }
        }
        else                                                            //��������ѯ���������ⲿdns������
        {
            memcpy(buf, &new_id, sizeof(unsigned short));
            length = sendto(my_socket, buf, length, 0, (struct sockaddr*) & server_addr, sizeof(server_addr));
            if (debug_level)
            {
                time_t t = time(NULL);
                char temp[64];
                strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
                printf("-----------------%s ���ⲿ��������������-----------------\n", temp);
                printf("��Ҫ��ѯ������Ϊ<URL:%s>\n",new_url);
            }
        }
    }
    else //���ڱ��ز�ѯ����������ip��ӳ��
    {
        if (strcmp(inter_result, "NOTFOUND") != 0) //����Ǵ��ڴ��в鵽
        {
            strcpy(ip, inter_result);
                if (debug_level){
                printf("���ڴ��в鵽<URL:%s>��Ӧ��IP���������:\n",new_url);
                printf("<URL: %s , IP: %s>\n\n", new_url, ip);
                }
        }
        else //����Ǵ�cache�в鵽
        {
            strcpy(ip, cache_result);
            cache_url_ip= move_to_head(cache_url_ip, new_url);//����¼�Ƶ���ǰ��λ��(LRU�㷨��ʵ��)

            if (debug_level)
            {
                printf("��cache�в鵽<URL:%s>��Ӧ��IP���������:\n",new_url);
                printf("<URL: %s , IP: %s>\n\n", new_url, ip);
                show_cache_flag = 1;
            }
        }


        memcpy(sendbuf, buf, length);       //������Ӧ����
        //1.����ͷ��
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

        if (strcmp(ip, "0.0.0.0") == 0)    //��ip�Ƿ�Ϸ�
        {

            num = htons(0x0);               //����Ƿ�ֱ�Ӳ��ش�
        }
        else
        {
            num = htons(0x1);	            //�Ϸ���ش���Ϊ1
        }
        memcpy(&sendbuf[6], &num, sizeof(unsigned short));
        //2.����RR��Դ��¼
        unsigned short name = htons(0xc00c);
        memcpy(res_rec, &name, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TYPE�ֶ�
        unsigned short type = htons(0x0001);
        memcpy(res_rec + pos, &type, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //CLASS�ֶ�
        unsigned short _class = htons(0x0001);
        memcpy(res_rec + pos, &_class, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //TTL�ֶ�
        unsigned long ttl = htonl(0x00000080);
        memcpy(res_rec + pos, &ttl, sizeof(unsigned long));
        pos += sizeof(unsigned long);

        //RDLENGTH�ֶ�
        unsigned short RDlength = htons(0x0004);
        memcpy(res_rec + pos, &RDlength, sizeof(unsigned short));
        pos += sizeof(unsigned short);

        //RDATA�ֶ�
        unsigned long IP = (unsigned long)inet_addr(ip);
        memcpy(res_rec + pos, &IP, sizeof(unsigned long));
        pos += sizeof(unsigned long);
        pos += length;

        //��HEADER��RR��ͬ��ɵ�DNS��Ӧ���Ĵ���sendbuf׼������(�˴�������QUESTION����)
        memcpy(sendbuf + length, res_rec, sizeof(res_rec));
        length = sendto(my_socket, sendbuf, pos, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));//������õı��Ķη����ͻ���
        if (length < 0 && debug_level!=0)
        {
            time_t t = time(NULL);
            char temp[64];
            strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
            printf("-----------------%s��ͻ��˷��ͱ��ĳ��ִ���-----------------\n", temp);
        }
        if (debug_level)
        {
            time_t t = time(NULL);
            char temp[64];
            strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
            printf("-----------------%s �򱾵ؿͻ��˷�����Ӧ-----------------\n", temp);
        }

    }

}


void handle_server_message(char *buf,int length,SOCKADDR_IN server_addr)
{
    char url[200];                              //�������ڱ���url������
    if (debug_level)                            //ֻҪ���Եȼ���Ϊ0�ʹ�ӡʱ���
    {
        time_t t = time(NULL);
        char temp[64];
        strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
        printf("\n-----------------%s �յ������ⲿ����������Ӧ-----------------\n", temp);
    }
    if (debug_level > 1)                                            //������Եȼ�Ϊ2��չʾ�������ݱ���Ϣ
    {
        show_message(buf, length);
    }
    //1.������һ���֣�Headerͷ��
    unsigned short ID;
    memcpy(&ID, buf, sizeof(unsigned short));                      //�ӽ��ձ���(buf����)�л�ȡID��
    int cur_id = ID - 1;                                            //Ϊʲôcur_idҪ-1?��Ϊת����������ظ�new_id��Ҫ+1(����Ĺ涨)
                                                                    //cur_idʵ��Ҳ���Ǳ���id��¼��id_table�е�λ��
    //��IDӳ������ԭ����ID��
    memcpy(buf, &ID_table[cur_id].client_id, sizeof(unsigned short));//IDӳ����е�old_id�ֶξ���ԭ�����ؿͻ��˵�id��
    ID_table[cur_id].finished = TRUE;                               //��idӳ����е�״̬���ˣ������ô����ⲿ���������͵�����ɹ�
    SOCKADDR_IN client_temp = ID_table[cur_id].client_addr;         //��ȡold_idӳ����е�client_addr�ֶΣ�Ҳ���Ǳ��ؿͻ��˵ĵ�ַ�ṹ�壬������������ͨ��
    int num_query = ntohs(*((unsigned short*)(buf + 4)));          //QDCOUNT�ֶ��ڵ��ĸ��ֽڿ�ʼ�������ֽڽ���,��ʾQuestionʵ�������,buf�����ǻ���������׵�ַ
    int num_response = ntohs(*((unsigned short*)(buf + 6)));       //ANCOUNT�ֶ��ڵ������ֽڿ�ʼ�ڰ˸��ֽڽ�������ʾAnswerʵ�������
    char* p = buf + 12;                                             //�ƶ�ָ��,��pָ��Questionʵ��(12���ֽڵ�Header�������Question)

    //2.�����ڶ����֣�Question
    for (int i = 0; i < num_query; i++)
    //��QUESTION��num_query��ʵ���urlȫ��ת��Ϊ������url��һ����˵ֻ����һ��QUESTIONʵ�壬���Դ˴�����num_queryҲ��
    //multiple questionsĿǰӦ��ֻ�������ϴ��ڣ�ʵ�ʵķ�����������Ӧ�ö�û��ʵ����
    {
        url_transform(p, url);                                      //���ַ���������ʾ�ĵ�����ת��Ϊ������ʽ������

        while (*p > 0)
        {
            p += (*p) + 1;                                          //ֱ��*p=0��ָ���ַ�����β\0
        }
        p += 5;                                                     //p=p+sizeof(char)*5��1�ֽ�\0��2�ֽ�QTYPE��2�ֽ�QCLASS��,pָ����һ��QUESTIONʵ��

    }


    if (num_response > 0 && debug_level)
    {
        printf("�ⲿDNS��������ѯ<URL:%s>�ɹ����������:\n", url);            //���url����������Ҫ��ѯ��url
    }

    //3.�����������֣����������ⲿDNS��������ANSWER,����ANSWER|AUTHORITY|ADDITIONAL��ʵ���ΪRR(����ֻ����ANSWER)
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


            if (type == 1)
        {
            ip1 = (unsigned char)*p++;
            ip2 = (unsigned char)*p++;
            ip3 = (unsigned char)*p++;
            ip4 = (unsigned char)*p++;
            sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            if (debug_level)
            {
                printf("IP��ַ1�� %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
            }

            insert_to_cache(url,ip);

            while(p&&*(p+1)&&*(p+=12))          //��ȡ�������������ĵ�IP��ַ������ǰ���12�ֽ�
            {
                ip1=(unsigned char)*p++;
                ip2=(unsigned char)*p++;
                ip3=(unsigned char)*p++;
                ip4=(unsigned char)*p++;
                sprintf(ip,"%d.%d.%d.%d",ip1,ip2,ip3,ip4);

                if(debug_level>1)
                {
                    printf("IP��ַ2�� %d.%d.%d.%d\n",ip1,ip2,ip3,ip4);
                }

                //��ӳ����뵽cacheӳ���
                insert_to_cache(url, ip);
            }
            break;
        }
        else                                                    //��TYPE��ΪA��ֱ��������ȡDATAip�Ĳ���
        {
            p += rdlength;
        }
    }
    //�����ķ��͸��ͻ���
    length = sendto(my_socket, buf, length, 0, (SOCKADDR*)&client_temp, sizeof(client_temp));
    //��Ӧ��Ϣ
    if (debug_level)
        {
            time_t t = time(NULL);
            char temp[64];
            strftime(temp, sizeof(temp), "%Y/%m/%d %X %A", localtime(&t));
            printf("\n-----------------%s �򱾵ؿͻ��˷�����Ӧ-----------------\n", temp);
        }
}


void handle_arguments(int argc, char* argv[])
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
            printf("ָ��DNS��������IP��ַ��  %s\n", server_ip);
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
    printf("debug=%d\n", debug_level);
}

void insert_to_inter()
{
    FILE* file;
    if ((file = fopen(file_path, "r")) == NULL)                 //��file_path��ȡ�ļ�ʧ��
    {
        if (debug_level)
            printf("��ȡ�����ļ�ʧ��\n");
        return;
    }

    char url[65] = "", ip[16] = "";

    if (debug_level)
        printf("���������ļ��ɹ�,�������:\n");
    while (fscanf(file, "%s %s", ip, url) > 0)
    {
        if (debug_level)
        {
            printf("<����: %s, IP��ַ : %s>\n", url, ip);
        }
        inter_url_ip = push_front(inter_url_ip, url, ip);       //������ڴ���
    }

    fclose(file);
}

void initialize_id_table()
{
    for (int i = 0; i < ID_TABLE_SIZE; i++)
    {//��ΪIDӳ�����һ���ṹ�壬������Ҫ�ݹ��������г�ʼ��
        ID_table[i].client_id= 0;
        ID_table[i].finished = TRUE;
        ID_table[i].survival_time = 0;
        memset(&(ID_table[i].client_addr), 0, sizeof(SOCKADDR_IN));
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
    if(debug_level)
        printf("socket���ӽ����ɹ���\n");

    //�����͡�����ģʽ�趨Ϊ������
    int non_block = 1;

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
    if (bind(my_socket, (struct sockaddr*) & client_addr, sizeof(client_addr)) < 0)
    {
        printf("socket�˿ڰ�ʧ�ܣ�\n");
        exit(1);
    }

    if (debug_level)
        printf("socket�˿ڰ󶨳ɹ���\n");
}

void show_message(char* buf, int len)
{
    unsigned char byte;
    if(debug_level > 1){
         printf("������������:\n");
    }

    for (int i = 0; i < len;)
    {
        byte = (unsigned char)buf[i];
        printf("%02x ", byte);
        i++;
        if (i % 16 == 0)    //ÿ��ֻ��ӡ16�ֽ�
            printf("\n");
    }
    if(debug_level > 1){
       printf("\n�����ܳ���Ϊ%d\n\n", len);
    }
}

void url_transform(char* buf, char* result)
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

void insert_to_cache(char* url, char *ip)
{
    //���ж�cache���Ƿ��и�Ԫ��
    char tmp[100];                        //��������ʼ��temp������
    memset(tmp, '\0', sizeof(tmp));      //���tmp������
    strcpy(tmp, get_ip(cache_url_ip, url));      //����url���������㣬���ض�Ӧ��ip��ַ����д��tmp������(list����)

    if (strcmp(tmp, "NOTFOUND") != 0)    //��Cache�д��ڸ�������������ֻ��ӽ�ȥ����ʲô����������
    {
        nodeptr cur=cache_url_ip;
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
        strcpy(cache_url_ip->ip[cur->num++], ip);//������¼���url-ipӳ���ϵ
    }
    else                                 //cache�в����ڸ�����
    {
        if (size(cache_url_ip) >= 8)            //��cache��СΪ8��Ϊ����
        {
            cache_url_ip = pop_back(cache_url_ip);     //��ɾ�����������һ�����
        }
        cache_url_ip = push_front(cache_url_ip, url, ip);//�ټ����µ�url-ipӳ���ϵ(ע������汾��û����LRU�㷨)
        if(debug_level>1){
            printf("����url-ipӳ����cache�ɹ���cache�е�ӳ�����£�\n");
            show_cache_url_ip();
        }

    }

}

unsigned short id_transform(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished)
{
    int i = 0;
    for (i = 0; i != ID_TABLE_SIZE; ++i)//ֻ�е�idת����û������ʱ����ܴ���ͻ���id
    {
        //����ͻ��˵�֮ǰ����Ҫ���ҵ�һ�����ʵ�λ��(�����Ҳ������ʵ�λ�þ�һֱѭ�������������ң���Ϊ�������Կ϶��ҵõ�)
        if ((ID_table[i].survival_time > 0 &&  ID_table[i].survival_time < time(NULL) )
            || ID_table[i].finished == TRUE)                 //���ʵ�λ�ã�ָ��ʱʧЧ��������ɵ�λ��
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

void show_cache_url_ip()
{
    printf("\ncache�е�URL-IP����\n");
    print_list(cache_url_ip);
}
