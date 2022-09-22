#ifndef PUBLIC_H_INCLUDED
#define PUBLIC_H_INCLUDED
/*
	ʹ���������뽫Windows��Linux��ͷ�ļ����궨�塢������ͳһ���֡�
	�����Windowsƽ̨�ϱ��룬��������һ��������֧��
	�����Linuxƽ̨�ϱ��룬�������ڶ���������֧
*/
// ��Ϊwindowsϵͳ
#ifdef _WIN32 
#include <winsock2.h> 
#pragma comment(lib, "Ws2_32.lib") 
WSADATA wsaData;

// ��ΪLINUXϵͳ
#elif __linux__ 
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#define closesocket(socketfd) close(socketfd)
#endif

// ��󻺳����ռ�
#define BUFFER_SIZE 2048   
// IDӳ�����󳤶�
#define ID_TABLE_SIZE 32                

#endif // PUBLIC_H_INCLUDED
