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

class MyWSAInfo
{
public:
	static void initWSA() {
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) < 0)	//�� Winscok 2.2 DLL
			exit(EXIT_FAILURE);						//��ʧ��ʱ�˳�����
	}

	static void clean() {
		WSACleanup();								//�ر� DLL
	}
};


#endif
