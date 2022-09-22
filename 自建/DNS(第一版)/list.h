//���������Ķ��嵥����д����Ϊ����-IP ��ַӳ������ʹ���������ݽṹ���д洢�ͱ�ʾ��������ȫ���Ը���Ϊ˫�����

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED
#pragma warning(disable:4996)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct node
{
    char url[100];                                            //����
    char ip[21][50];                                          //ip��ַ�������ж��
    int num;                                                  //ip��ַ�ĸ���
    int round_num;                                            //���ؾ�����ؿ���ֱ��ɾ��
    struct node* next;
}Node, * nodeptr;                                            //�����������Լ�ͷָ��

nodeptr create_list();                                       //����������
nodeptr push_front(nodeptr head, char* url, char* ip);     //������ͷ����ӽ��
void print_list(nodeptr head);                               //��ӡ����
char* get_ip(nodeptr head, char* url);                      //����url���������㣬���ض�Ӧ��ip��ַ
nodeptr pop_back(nodeptr head);                             //ɾ����������һ�����
int size(nodeptr head);                                     //��ȡ����ĳ���
nodeptr move_to_head(nodeptr head,char* url);               //�ƶ�����������ͷ��

#endif // LIST_H_INCLUDED
