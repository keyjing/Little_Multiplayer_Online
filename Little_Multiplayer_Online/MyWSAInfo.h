#ifndef _MyWSAInfo_h
#define _MyWSAInfo_h

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<cstdlib>
#pragma comment(lib, "ws2_32.lib")

/*			1. IP �������			*/
#define IP_LENGTH	16				// IP �ַ��������
#define BUFSIZE		1024

/*			2. ��ز�������			*/
bool charArrayCopy(char* dir, const char* src, int len);		// IP �ַ�������
long long getHashOfIP(const char* ip);							// ��ȡ IP �ַ����� Hash ֵ
void getLocalIP(char* ip);										// ��ȡ��ǰ���� IP


#endif
