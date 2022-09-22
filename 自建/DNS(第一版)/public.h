//公用头文件中存放宏定义、结构体、类声明、函数声明、全局变量声明、typedef声明

#ifndef PUBLIC_H_INCLUDED
#define PUBLIC_H_INCLUDED
#include <winsock2.h>                    //socket编程库
#pragma comment(lib, "Ws2_32.lib")       //网络编程库

#define BUFFER_SIZE 2048                //最大缓冲区空间
#define ID_TABLE_SIZE 32                //ID映射表最大长度

#endif // PUBLIC_H_INCLUDED
