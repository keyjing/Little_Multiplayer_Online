#include "Multicast.h"
#include"MyEasyLog.h"

using namespace std;

/*			��������			*/
long long getHashOfIP(const char* ip)		// ��ȡ IP �ַ����� Hash ֵ
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
	turnOff();		/* ע���ȴ��̹߳رգ������̻߳����			*/
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
	//�ಥ�׽���
	SOCKET sock = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,		/* WSASocketW �� protocal ������Ϊ 0 */
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "MC SENDER SOCKET", "Create Socket FAILED.");
		mc->running = false;
		mc->thd_finished();
		return;
	}
	//����ಥ��ַ
	sockaddr_in remote;
	::memset(&remote, 0, sizeof(sockaddr_in));
	remote.sin_family = AF_INET;
	remote.sin_port = htons(mc->mc_port);
	::inet_pton(AF_INET, mc->mc_ip, &remote.sin_addr);

	//�ಥ�����׽���
	SOCKET sockM = ::WSAJoinLeaf(sock, (SOCKADDR*)&remote, sizeof(SOCKADDR),
		NULL, NULL, NULL, NULL, JL_BOTH);
	if (sockM == INVALID_SOCKET) {
		::closesocket(sock);
		MyEasyLog::write(LOG_ERROR, "MC SENDER SOCKET", "Join Leaf FAILED.");
		mc->running = false;
		mc->thd_finished();
		return;
	}
	//��������ֱ�������̵߳��� turnOff()
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
	if (running)	// ͬһ̨����������ͬһ���˿ڶಥ
		return MC_OCCUPIED;
	running_thds = 1;
	running = true;
	thread thd(send_thd, this, msg);		//ͨ��������̷߳��Ͷಥ
	thd.detach();
	return MC_NO_OCCUPIED;
}

int Multicast::receiver(int maxfound, int time_limit, char msg[][BUFSIZE], char from_ip[][IP_LENGTH])
{
	if (maxfound <= 0 || time_limit <= 0)
		return MC_NO_RECEVICE;

	if (running)	// ͬһ̨����������ͬһ���˿ڶಥ
		return MC_OCCUPIED;
	running = true;

	SOCKET sock = ::WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,
		WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF | WSA_FLAG_OVERLAPPED);
	if (sock == INVALID_SOCKET) {
		MyEasyLog::write(LOG_ERROR, "MC RECEIVER SOCKET", "Create Socket FAILED.");
		running = false;
		return MC_SOCK_FAILED;
	}
	//���ص�ַ
	sockaddr_in local;
	::memset(&local, 0, sizeof(sockaddr_in));
	local.sin_family = AF_INET;
	local.sin_port = htons(mc_port);
	::inet_pton(AF_INET, INADDR_ANY, &local.sin_addr);	// INADDR_ANY 0.0.0.0 ��ʾ�������б����ö˿ڵ���Ϣ

	//�󶨣��� sock ����Ϊ��������
	if (::bind(sock, (SOCKADDR*)&local, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		::closesocket(sock);
		MyEasyLog::write(LOG_ERROR, "MC RECEIVER SOCKET", "Bind Socket FAILED.");
		running = false;
		return MC_SOCK_FAILED;
	}

	//����ಥ��
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

	//���ͷ���ַ
	sockaddr_in from;
	int addrSize = sizeof(SOCKADDR);
	int record = 0;		// �ҵ�����
	int count = 0;		// ���Ҵ���
	struct timeval timeout = { 0,  (time_limit / maxfound) };	// ÿ�β������ʱ��Ϊ time_limit/maxfound
	struct fd_set rfds;
	long long foundip_hash[MAX_FOUND_SERVER] = { 0 };
	while (record < maxfound && count++ < maxfound)
	{
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		if (::select(0, &rfds, NULL, NULL, &timeout) <= 0)
			continue;
		int ret = ::recvfrom(sock, msg[record], BUFSIZE, 0, (SOCKADDR*)&from, &addrSize);
		if (ret == SOCKET_ERROR)	// ���ӶϿ�
			break;
		::inet_ntop(AF_INET, &from.sin_addr, from_ip[record], IP_LENGTH);
		long long ip_hash = getHashOfIP(from_ip[record]);
		bool found = false;
		for (int i = 0; i < record && !found; ++i)
			if (foundip_hash[i] == ip_hash) found = true;
		if (found)		// �Ѽ�¼������
			break;
		// ���м�¼
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
