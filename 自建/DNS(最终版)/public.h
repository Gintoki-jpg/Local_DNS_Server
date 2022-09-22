#ifndef PUBLIC_H_INCLUDED
#define PUBLIC_H_INCLUDED
//使用条件编译将Windows和Linux的头文件、宏定义、函数等统一名字。如果在Windows平台上编译，将保留第一个条件分支；如果在Linux平台上编译，将保留第二个条件分支
#ifdef _WIN32 // 条件分支1
#include <winsock2.h> 
#pragma comment(lib, "Ws2_32.lib") 
WSADATA wsaData;


#elif __linux__ // 条件分支2
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#define closesocket(socketfd) close(socketfd)
#endif



#define BUFFER_SIZE 2048                //最大缓冲区空间
#define ID_TABLE_SIZE 32                //ID映射表最大长度



#endif // PUBLIC_H_INCLUDED
