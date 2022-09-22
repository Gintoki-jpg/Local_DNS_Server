//这里把链表的定义单独书写，因为域名-IP 地址映射表可以使用其他数据结构进行存储和表示，后续完全可以更换为双链表等

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED
#pragma warning(disable:4996)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct node
{
    char url[100];                                            //域名
    char ip[21][50];                                          //ip地址，可能有多个
    int num;                                                  //ip地址的个数
    int round_num;                                            //负载均衡相关可以直接删除
    struct node* next;
}Node, * nodeptr;                                            //声明链表结点以及头指针

nodeptr create_list();                                       //创建空链表
nodeptr push_front(nodeptr head, char* url, char* ip);     //向链表头部添加结点
void print_list(nodeptr head);                               //打印链表
char* get_ip(nodeptr head, char* url);                      //根据url查找链表结点，返回对应的ip地址
nodeptr pop_back(nodeptr head);                             //删除链表的最后一个结点
int size(nodeptr head);                                     //获取链表的长度
nodeptr move_to_head(nodeptr head,char* url);               //移动结点至链表的头部

#endif // LIST_H_INCLUDED
