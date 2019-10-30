#include "MyWSAInfo.h"

bool charArrayCopy(char* dir, const char* src, int len)
{
	if (src == NULL || src[0] == '\0')
		return false;
	int i = 0;
	while (i < len && src[i] != '\0') dir[i] = src[i++];
	i = (i < len) ? i : len - 1;
	dir[i] = '\0';
	return true;
}

long long getHashOfIP(const char* ip)
{
	if (ip == NULL || ip[0] == '\0')
		return 0;
	int sub[4] = { 0 };
	int k = 0;
	for (int i = 0; i < IP_LENGTH && ip[i] != '\0'; ++i) {
		if (ip[i] >= '0' && ip[i] <= '9')
			sub[k] = sub[k] * 10 + ip[i] - '0';
		else if (++k == 4)
			break;
	}
	long long res = 0;
	for (int i = 0; i < 4; ++i)
		res = (res << 8) + sub[i];
	return res;
}

void getLocalIP(char* ip)
{
	// 获取本机 IP
	char hostname[BUFSIZE] = { 0 };
	gethostname(hostname, sizeof(hostname));
	//需要关闭SDL检查：Project properties -> Configuration Properties -> C/C++ -> General -> SDL checks -> No
	hostent* host = gethostbyname(hostname);
	charArrayCopy(ip, inet_ntoa(*(in_addr*)*host->h_addr_list), IP_LENGTH);
}
