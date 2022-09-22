#ifndef DNS_SERVER_H_INCLUDED
#define DNS_SERVER_H_INCLUDED
#pragma warning(disable:4996)
#include <stdbool.h>
#include <time.h>
#include"public.h"
#include"list.h"

void show_packet(char* buf, int len);                                                   //输出完整的数据包信息
void show_cache();                                                                       //展示cache列表
void get_url_ip_map();                                                                   //读取域名-ip映射表至内存中
void insert_record_to_cache(char* url, char *ip);                                       //将新的映射关系记录存入cache
unsigned short get_new_id(unsigned short ID, SOCKADDR_IN client_addr, BOOL finished);  //向外部DNS服务器发送请求时需要保存原id信息
void transform_url(char* buf, char* result);                                            //将字符计数法表示的的域名转换为正常形式的域名(直到遇到\0符号为止)
void process_server_request(char *buf,int length,SOCKADDR_IN server_addr);              //处理来自服务器的消息
void process_client_request(char *buf,int length,SOCKADDR_IN client_addr);              //处理来自客户端的消息
void get_arguments(int argc, char* argv[]);                                             //设置调试等级，根据用户命令指定外部DNS服务器
void initialize_id_table();                                                              //初始化id转换表
void initialize_socket();                                                                //初始化套接字信息

typedef struct
{
    unsigned short old_id;             //客户端发给DNS服务器的客户端ID
    SOCKADDR_IN client_addr;            //客户端套接字
    int survival_time;                  //存活时间
    BOOL finished;                      //标记该请求是否完成
}id_table;                              //id映射表(结构体)


#endif // DNS_SERVER_H_INCLUDED

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
    //QDCOUNT 无符号16位整数，表示报文请求段中的问题记录数。
    //ANCOUNT 无符号16位整数，表示报文回答段中的回答记录数。


/*question实体的格式
      0	 1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  QNAME						|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					  QTYPE  					|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |					 QCLASS			    		|
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */
    /*结构：QNAME（最长63字节+'\0'）+QTYPE（2字节）+QCLASS（2字节）*/
    /*
    字符计数法：首先用一个特殊的字符(0)来表示标签的开始，然后用一个记数字段表明标签内的字符数，
    然后就知道了后面应该紧跟的字符数，从而也就可以确定结束的位置

    QNAME:字节数不定，以0x00作为结束符。表示查询的主机名。
    注意：众所周知，主机名被"."号分割成了多段标签。在QNAME中，每段标签前面加一个数字，表示接下来标签的长度。(字符计数法)
    比如：api.sina.com.cn表示成QNAME时，会在"api"前面加上一个字节0x03，"sina"前面加上一个字节0x04，"com"前面加上一个字节0x03，
    而"cn"前面加上一个字节0x02

    */

/*构造DNS报文资源记录（RR）区域（16字节）
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
/*
    NAME：长度不定，可能是真正的数据，也有可能是指针（其值表示的是真正的数据在整个数据中的字节索引数），
    还有可能是二者的混合（以指针结尾）。若是真正的数据，会以0x00结尾；若是指针，指针占2个字节，第一个字节的高2位为11
    TTL：占4个字节。表示RR生命周期，即RR缓存时长，单位是秒
    RDLENGTH：占2个字节。指定RDATA字段的字节数
    RDATA：即之前介绍的value，含义与TYPE有关

*/

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
        //NAME字段：资源记录中的域名通常是查询问题部分的域名的重复，可以使用2字节的偏移指针表示
        //最高2位为11，用于识别指针，最后四位为1100（12）表示头部长度
        //因此该字段的值为0xC00C
