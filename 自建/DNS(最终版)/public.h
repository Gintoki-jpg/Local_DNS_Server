#ifndef PUBLIC_H_INCLUDED
#define PUBLIC_H_INCLUDED
//ʹ���������뽫Windows��Linux��ͷ�ļ����궨�塢������ͳһ���֡������Windowsƽ̨�ϱ��룬��������һ��������֧�������Linuxƽ̨�ϱ��룬�������ڶ���������֧
#ifdef _WIN32 // ������֧1
#include <winsock2.h> 
#pragma comment(lib, "Ws2_32.lib") 
WSADATA wsaData;


#elif __linux__ // ������֧2
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#define closesocket(socketfd) close(socketfd)
#endif



#define BUFFER_SIZE 2048                //��󻺳����ռ�
#define ID_TABLE_SIZE 32                //IDӳ�����󳤶�



#endif // PUBLIC_H_INCLUDED
