#include "public.h"
#include"dns_server.h"
#include"list.h"

//����ȫ�ֱ���
int count = 0;                          //���
id_table ID_table[ID_TABLE_SIZE];	    //IDӳ���
SOCKET my_socket;                       //�׽���
SOCKADDR_IN client_addr;                //�ͻ����׽��ֵ�ַ�ṹ��
SOCKADDR_IN server_addr;                //�������׽��ֵ�ַ�ṹ��
int debug_level = 0;                    //��ʼ��debug�ȼ� ���Եȼ�Ϊ 0 ʱ��û�е�����Ϣ���|���Եȼ�Ϊ 1 ʱ������򵥵�ʱ�����꣬��ţ���ѯ����|���Եȼ�Ϊ 2 ʱ���������ĵ�����Ϣ
char server_ip[16] = "10.3.9.4";        //Ĭ�Ϸ�����ip��ַ
char file_path[100] = "dnsrelay.txt";   //Ĭ�������ļ�·��
nodeptr url_ip_table;                   //����IP��ַӳ���
nodeptr cache;                          //���ٻ���

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



//���Եȼ�Ϊ1�����
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

//չʾcache�б�
void show_cache()
{
    printf("\ncache�б�\n");
    print_list(cache);
}

//���������ļ���ȡurl-ipӳ������������ڴ���
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

//���µ�ӳ���ϵ��¼����cache
void insert_record_to_cache(char* url, char *ip)
{
    //���ж�cache���Ƿ��и�Ԫ��
    char tmp[100];
    memset(tmp, '\0', sizeof(tmp));
    strcpy(tmp, get_ip(cache, url));

    if (strcmp(tmp, "NOTFOUND") != 0)    //cache���Ѵ��ڸ�����
    {
        nodeptr cur=cache;
        while(strcmp(cur->url,url)!=0)
        {
            cur=cur->next;
        }
        for(int i=0;i<cur->num;i++)
        {
            if(strcmp(cur->ip[i],ip)==0)//��ӳ���ϵ��cache���ҵ���ʲô������
            {
                return;
            }
        }
        if(cur->num>20)//ӳ���ֻ��20��ӳ�䣬����20�򷵻�
        {
            return;
        }
        strcpy(cache->ip[cur->num++], ip);      //�������ӳ���ϵ
    }
    else    //cache�в����ڸ�����
    {
        if (size(cache) >= 8)  //cache��СΪ8������
        {
            cache = pop_back(cache);    //�滻cache��¼
        }
        cache = push_front(cache, url, ip);
        if(debug_level>1)
            show_cache();
    }

}

//��Ҫ���ⲿDNS��������������ʱ����Ҫ����ԭid��Ϣ
unsigned short get_new_id(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished)
{
    int i = 0;
    for (i = 0; i != ID_TABLE_SIZE; ++i)
    {
        //����֮ǰ����Ҫ���ҵ�һ�����ʵ�λ��
        //���ʵ�λ�ã�ָ��ʱʧЧ��������ɵ�λ��
        if ((ID_table[i].survival_time > 0 &&  ID_table[i].survival_time < time(NULL) )
            || ID_table[i].finished == TRUE)
        {
            if (ID_table[i].old_id != i + 1)      //ȷ���¾�ID��ͬ
            {
                ID_table[i].old_id = ID;     //����ID
                ID_table[i].client_addr = client_addr;   //���ÿͻ����׽���
                ID_table[i].finished = finished;  //����Ƿ��ѯ�Ѿ����
                ID_table[i].survival_time = time(NULL) + 5;     //ʱ������Ϊ5��

                //printf("\nԭID��%d",ID);
                //printf("\n��ID��%d", i);
                break;
            }
        }
    }
    if (i == ID_TABLE_SIZE) //�Ǽ�ʧ�ܣ���ǰת��������
    {
        return 0;
    }
    return (unsigned short)i + 1; //������ID��
}

//���ַ���������ʾ�ĵ�����ת��Ϊ������ʽ������(ֱ������\0����Ϊֹ)
void transform_url(char* buf, char* result)
{
    int i = 0, j = 0, k = 0, len = strlen(buf);
    while (i < len)
    {
        if (buf[i] > 0 && buf[i] <= 63)         //����
        {
            for (j = buf[i], i++; j > 0; j--, i++, k++) //����url
            {
                result[k] = buf[i];
            }
        }
        if (buf[i] != 0) //�����û�������Ӹ�"."���ָ���
        {
            result[k] = '.';
            k++;
        }
    }
    result[k] = '\0'; //��ӽ�����
}

//���������ⲿ����������Ϣ
void process_server_request(char *buf,int length,SOCKADDR_IN server_addr)
{
    char url[200];                              //�������ڱ���url������

    if (debug_level > 1)                        //���������ϢΪ1��չʾ��Ҫ���ݱ���Ϣ
    {
        show_packet(buf, length);
    }

    //�ӽ��ձ����л�ȡID��
    unsigned short ID;
    memcpy(&ID, buf, sizeof(unsigned short));
    int cur_id = ID - 1;

    //��IDӳ������ԭ����ID��
    memcpy(buf, &ID_table[cur_id].old_id, sizeof(unsigned short));
    ID_table[cur_id].finished = TRUE;

    //��ȡ�ͻ�����Ϣ
    SOCKADDR_IN client_temp = ID_table[cur_id].client_addr;

    /*��Ӧ����header��ʽ
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
    //������QDCOUNT �޷���16λ������ʾ����������е������¼����
    //��Դ��¼��ANCOUNT �޷���16λ������ʾ���Ļش���еĻش��¼����

    int num_query = ntohs(*((unsigned short*)(buf + 4)));
    int num_response = ntohs(*((unsigned short*)(buf + 6)));

    char* p = buf + 12; //��pָ��question�ֶ�



    //��ȡquestion�ֶ����е�url
    /*question��ʽ
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
        p += 5; //ָ����һ��Queries����
        /*�ṹ����ѯ�����63�ֽ�+'\0'��+��ѯ���ͣ�2�ֽڣ�+��ѯ�ࣨ2�ֽڣ�*/
    }

    if (num_response > 0 && debug_level > 1)
    {
        printf("�����ⲿDNS������ <Url : %s>\n", url);
    }

    //�����ⲿDNS�������Ļ�Ӧ
    /*
    ����DNS������Դ��¼��RR������16�ֽڣ�
      0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |												|
    /												/
    /					  NAME��2��				    /
    |												|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  TYPE��2��					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 CLASS��2��					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  TTL��4��					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					RDLENGTH��2��				|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    /					 RDATA��4��					/
    /												/
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */

    for (int i = 0; i < num_response; ++i)
    {
        if ((unsigned char)*p == 0xc0) /* ����NAME�ֶ�ʹ��ָ��ƫ�Ʊ�ʾ */
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
        //�����ֽ��Ǵӵ��ֽڵ����ֽڵģ�����0001�����������ֽ�˳��Ӹ��ֽڵ����ֽڣ���Ӧ0100��
        //ntohs���������ֽ�˳��ת��Ϊ�����ֽ�˳��
        //htons: �������ֽ�˳��ת��Ϊ�����ֽ�˳��

        //��ȡ��Դ��¼��ĸ����ֶ�
        unsigned short type = ntohs(*(unsigned short*)p);           //TYPE�ֶ�
        p += sizeof(unsigned short);
        unsigned short _class = ntohs(*(unsigned short*)p);         //CLASS�ֶ�
        p += sizeof(unsigned short);
        unsigned short ttl_high_byte = ntohs(*(unsigned short*)p); //TTL��λ�ֽ�
        p += sizeof(unsigned short);
        unsigned short ttl_low_byte = ntohs(*(unsigned short*)p);  //TTL��λ�ֽ�
        p += sizeof(unsigned short);
        int ttl = (((int)ttl_high_byte) << 16) | ttl_low_byte;    //�ϲ���������int��������

        int rdlength = ntohs(*(unsigned short*)p);  //���ݳ���
        //������1��TYPE A��¼����Դ������4�ֽڵ�IP��ַ
        p += sizeof(unsigned short);

        if (debug_level > 1)
        {
            printf("����: %d,  ��: %d,  TTL: %d\n", type, _class, ttl);
        }

        char ip[16];
        int ip1, ip2, ip3, ip4;

        // ���ͣ�A�����������IPv4��ַ
        if (type == 1)
        {
            ip1 = (unsigned char)*p++;
            ip2 = (unsigned char)*p++;
            ip3 = (unsigned char)*p++;
            ip4 = (unsigned char)*p++;
            sprintf(ip, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);

            if (debug_level>1)
            {
                printf("IP��ַ1�� %d.%d.%d.%d\n", ip1, ip2, ip3, ip4);
            }

            insert_record_to_cache(url,ip);

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
                insert_record_to_cache(url, ip);
            }
            break;
        }
        else
        {
            p += rdlength;
        }
    }

    //�����ͻؿͻ���
    //sendto()����������һ�����ݱ���������buf�У���Ҫ�ṩ��ַ
    /*
        local_socket���ⲿ�������׽���
        buf���������ݻ�����
        sizeof(buf)�����ջ�������С
        0����־λflag����ʾ���ò�����ʽ��Ĭ����Ϊ0
        client��Ŀ�����ݷ���Դ��ַ
        sizeof(client)����ַ��С
        ����ֵ��length���ɹ����յ������ݵ��ַ��������ȣ�������ʧ�ܷ���SOCKET_ERROR(-1)
    */
    length = sendto(my_socket, buf, length, 0, (SOCKADDR*)&client_temp, sizeof(client_temp));
}

//�������Կͻ��˵���Ϣ
void process_client_request(char *buf,int length,SOCKADDR_IN client_addr)
{
    int show_cache_flag = 0;    //�Ƿ��ӡcache�ı�־
    char old_url[200];          //ת��ǰ��url
    char new_url[200];          //ת�����url

    memcpy(old_url, &(buf[12]), length); //�ӱ����л��url,��ͷ����12�ֽ�
    transform_url(old_url, new_url);     //urlת��

    int i = 0;

    while (*(buf + 12 + i))
    {
        i++;
    }
    i++;

    if (buf + 12 + i != NULL)
    {
        unsigned short type = ntohs(*(unsigned short*)(buf + 12 + i));  //TYPE�ֶ�
        if (type == 28&& strcmp(get_ip(url_ip_table,new_url),"0.0.0.0")!=0)
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
                if (debug_level > 1)
                {
                    printf("<����: %s>\n\n", new_url);
                }
            }
            return;
        }
    }



    if (debug_level > 1)
        printf("\n\n---- �յ��˿ͻ��˵���Ϣ�� [IP:%s]----\n", inet_ntoa(client_addr.sin_addr));

    if (debug_level)
    {
        //��ӡʱ���
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
    if (strcmp(table_result, "NOTFOUND") == 0 && strcmp(cache_result, "NOTFOUND") == 0)    //�������ڱ����ļ���cache���޷��ҵ�����Ҫ�ϱ��ⲿdns������
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
            cache = move_to_head(cache, new_url);//����¼�Ƶ���ǰ��λ��

            if (debug_level > 0)
            {
                if (debug_level > 1)
                    printf("��cache�в鵽\n");
                printf("<����: %s , IP��ַ: %s>\n\n", new_url, ip);
                show_cache_flag = 1;
            }
        }

        char sendbuf[BUFFER_SIZE];
        memcpy(sendbuf, buf, length); //��ʼ�������ݰ�

        /*��Ӧ����header��ʽ
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
        //QRΪ1��ʾ��Ӧ��
        //OPCODEΪ0��ʾ��׼��ѯ(QUERY)
        //AA��Ȩ���𰸣�Ϊ0��ʾӦ����������Ǹ�������Ȩ������������
        //TCΪ0��ʾ����δ���ض�
        //RD�������ݹ飩Ϊ1��ʾϣ�����õݹ��ѯ
        //RA���ݹ���ã�Ϊ1��ʾ���ַ�����֧�ֵݹ��ѯ
        //ZΪ0����Ϊ�����ֶ�
        //RCODEΪ0����ʾû�в��
        //��˱���ͷ���ĵ�3-4�ֽ�Ϊֵ8180



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

        if (strcmp(ip, "0.0.0.0") == 0)    //�жϴ�ip�Ƿ�Ӧ�ñ�ǽ
        {

            num = htons(0x0);    //���ûش���Ϊ0
        }
        else
        {
            num = htons(0x1);	//�������ûش���Ϊ1
        }
        memcpy(&sendbuf[6], &num, sizeof(unsigned short));

        /*����DNS������Դ��¼��RR������
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

        //�����ĺ���Ӧ���ֹ�ͬ���DNS��Ӧ���Ĵ���sendbuf
        memcpy(sendbuf + length, res_rec, sizeof(res_rec));

        length = sendto(my_socket, sendbuf, pos, 0, (SOCKADDR*)&client_addr, sizeof(client_addr));
        //������õı��Ķη����ͻ���

        if (length < 0 && debug_level>1)
        {
            printf("��ͻ��˷�������\n");
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

//��ȡ�û������������õ��Եȼ����������û�����ָ���ⲿDNS��������IP��ַ�Լ������ļ�·��
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

//��ʼ��idӳ���
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

//��ʼ���׽�����Ϣ
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


