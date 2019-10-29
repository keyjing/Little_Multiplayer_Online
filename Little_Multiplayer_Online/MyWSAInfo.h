#ifndef _MyWSAInfo_h
#define _MyWSAInfo_h

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdlib>
#pragma comment(lib, "ws2_32.lib")

/*			1. IP 相关配置			*/
#define IP_LENGTH	16				// IP 字符串最长长度
#define BUFSIZE		1024

/*			2. 相关操作函数			*/
bool charArrayCopy(char* dir, const char* src, int len);		// IP 字符串复制
long long getHashOfIP(const char* ip);							//获取 IP 字符串的 Hash 值

class MyWSAInfo
{
	static WSADATA wsa;

public:
	static void initWSA() {
		if (WSAStartup(MAKEWORD(2, 2), &wsa) < 0)	//打开 Winscok 2.2 DLL
			exit(EXIT_FAILURE);						//打开失败时退出程序
	}

	static void clean() {
		WSACleanup();								//关闭 DLL
	}
};


#endif
