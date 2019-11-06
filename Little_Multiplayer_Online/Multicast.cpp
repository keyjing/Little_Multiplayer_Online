#include "Multicast.h"
#include"MyEasyLog.h"

using namespace std;

/*			辅助函数			*/
long long getHashOfIP(const char* ip)		// 获取 IP 字符串的 Hash 值
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

Multicast::Multicast(const char* ip, int port): 
	mc_port(port), running(false), running_thds(0)
{
	::memcpy(mc_ip, ip, IP_LENGTH * sizeof(char));
	if (port <= 0 || port >= (1 << 16)) mc_port = MC_DEFAULT_PORT;
	WSADATA wsa;
	::WSAStartup(MAKEWORD(2, 2), &wsa);
}

Multicast::~Multicast()
{
	turnOff();		/* 注：等待线程关闭，否则线程会出错			*/
	::WSACleanup();
}

void Multicast::turnOff()
{
	unique_lock<mutex> locker_thds(mt_thds);
	while (running_thds > 0) {
		running = false;
		cond_thds.wait(locker_thds);
	}
	locker_thds.unlock();
	MyEasyLog::write(LOG_NOTICE, "MC CLOSE", mc_ip);
}

void Multicast::thd_finished()
{
	unique_lock<mutex> locker_thds(mt_thds);
	--running_thds;
	locker_thds.unlock();
	cond_thds.notify_one();
}

void Multicast::send_thd(Multicast* mc, const char* msg)
{
	//多播套接字
	SOCKET sock = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,		/* WSASocketW 中 protocal 不能设为 0 */
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "MC SENDER SOCKET", "Create Socket FAILED.");
		mc->running = false;
		mc->thd_finished();
		return;
	}
	//加入多播地址
	sockaddr_in remote;
	::memset(&remote, 0, sizeof(sockaddr_in));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(mc->mc_port);
	::inet_pton(AF_INET, mc->mc_ip, &remote.sin_addr);

	//多播管理套接字
	SOCKET sockM = ::WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(SOCKADDR),
		NULL, NULL, NULL, NULL, JL_BOTH);
	if (sockM == INVALID_SOCKET) {
		::closesocket(sock);
		MyEasyLog::write(LOG_ERROR, "MC SENDER SOCKET", "Join Leaf FAILED.");
		mc->running = false;
		mc->thd_finished();
		return;
	}
	//持续发送直到其他线程调用 turnOff()
	while (mc->running) {
		if (::sendto(sockM, msg, strlen(msg) + 1, 0,
			(SOCKADDR*)&remote, sizeof(SOCKADDR)) == SOCKET_ERROR) 
		{
			MyEasyLog::write(LOG_ERROR, "MC SENDER", "Send FAILED.");
			break;
		}
		this_thread::sleep_for(chrono::milliseconds(200));
	}
	::closesocket(sockM);
	::closesocket(sock);
	mc->running = false;
	MyEasyLog::write(LOG_NOTICE, "MC SENDER THREAD", "FINISHED");
	mc->thd_finished();
}

int Multicast::sender(const char* msg)
{
	if (running)	// 同一台主机不能再同一个端口多播
		return MC_OCCUPIED;
	running_thds = 1;
	running = true;
	thread thd(send_thd, this, msg);		//通过分离的线程发送多播
	thd.detach();
	return MC_NO_OCCUPIED;
}

int Multicast::receiver(int maxfound, int time_limit, char msg[][BUFSIZE], char from_ip[][IP_LENGTH])
{
	if (maxfound <= 0 || time_limit <= 0)
		return MC_NO_RECEVICE;

	if (running)	// 同一台主机不能再同一个端口多播
		return MC_OCCUPIED;
	running = true;

	SOCKET sock = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "MC RECEIVER SOCKET", "Create Socket FAILED.");
		running = false;
		return MC_SOCK_FAILED;
	}
	//本地地址
	sockaddr_in local;
	::memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(mc_port);
	::inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// INADDR_ANY 0.0.0.0 表示接受所有本机该端口的消息

	//绑定，将 sock 更改为被动接收
	if (::bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		::closesocket(sock);
		MyEasyLog::write(LOG_ERROR, "MC RECEIVER SOCKET", "Bind Socket FAILED.");
		running = false;
		return MC_SOCK_FAILED;
	}

	//加入多播组
	sockaddr_in remote;
	::memset(&remote, 0, sizeof(sockaddr_in));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(mc_port);
	inet_pton(AF_INET, mc_ip, &remote.sin_addr);

	SOCKET sockM = ::WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(SOCKADDR),
		NULL, NULL, NULL, NULL, JL_BOTH);
	if (sockM == INVALID_SOCKET) {
		::closesocket(sock);
		MyEasyLog::write(LOG_ERROR, "MC RECEIVER SOCKET", "Join Leaf FAILED.");
		running = false;
		return MC_SOCK_FAILED;
	}

	//发送方地址
	sockaddr_in from;
	int addrSize = sizeof(SOCKADDR);
	int record = 0;		// 找到个数
	int count = 0;		// 查找次数
	struct timeval timeout = { 0,  (time_limit / maxfound) };	// 每次查找最大时间为 time_limit/maxfound
	struct fd_set rfds;
	long long foundip_hash[MAX_FOUND_SERVER] = { 0 };
	while (record < maxfound && count++ < maxfound)
	{
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		if (::select(0, &rfds, NULL, NULL, &timeout) <= 0)
			continue;
		int ret = ::recvfrom(sock, msg[record], BUFSIZE, 0, (SOCKADDR*)&from, &addrSize);
		if (ret == SOCKET_ERROR)	// 连接断开
			break;
		::inet_ntop(AF_INET, &from.sin_addr, from_ip[record], IP_LENGTH);
		long long ip_hash = getHashOfIP(from_ip[record]);
		bool found = false;
		for (int i = 0; i < record && !found; ++i)
			if (foundip_hash[i] == ip_hash) found = true;
		if (found)		// 已记录，丢弃
			break;
		// 进行记录
		if (ret >= BUFSIZE) ret = BUFSIZE - 1;
		msg[record][ret] = '\0';
		++record;
	}
	::closesocket(sockM);
	::closesocket(sock);
	MyEasyLog::write(LOG_WARNING, "MC RECEIVER RECORD", record);
	running = true;
	return record;
}
