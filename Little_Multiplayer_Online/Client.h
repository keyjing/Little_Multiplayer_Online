#ifndef _Client_h
#define _Client_h

#include"Multicast.h"
#include"Server.h"

#define NO_FOUND_SERVER		-1
#define FOUND_SERV_OVERFLOW	-2

#define CLIENT_OK				2
#define CLIENT_ALL_START		1		// ���пͻ��˾�����
#define CLIENT_CONN_SUCCESS		0		// ���ӷ������ɹ�
#define CLIENT_WAITING			-1		// �ȴ�����
#define CLIENT_ERROR			-2		// ����ʧ��

class PostMan
{
public:
	virtual void delivery(int id, const char* msg, int len) = 0;
};


// �ҵ��ķ������б�ṹ��
struct FoundServerResult {
	char name[MAX_FOUND_SERVER][MC_MSG_LENGTH] = { {0} };
	char ip[MAX_FOUND_SERVER][IP_LENGTH] = { {0} };
	int port[MAX_FOUND_SERVER] = { 0 };
	int found = 0;
};

// ����ģʽ
class Client
{
public:
	~Client();

	static Client* getInstance() {
		if (cp == nullptr) cp = new Client();
		return cp;
	}

	void setPostMan(PostMan* p) { if (postman) delete postman; postman = p; }

	int getIndex() const { return index; }
	int getClients() const { return clients; }

	// ͨ���ಥ���ҷ�����
	// @ parameter
	// @ mc_ip, mc_port: �ಥ IP �Ͷ˿�
	// @ maxfound: �����Ҹ���
	// @ time_limit: ����ʱ�䣬��λΪ����
	// @ fdservs: ����������IP���˿�
	// ����ֵ: �ҵ��ĸ���
	int findServer(const char* mc_ip, int mc_port, int maxfound, int time_limit, FoundServerResult& fdservs);

	int addOpts(const char* opts, int len);				// �������ݻ�����д��������

	int connServer(const char* server_ip, int serv_port, char* initenv);	// ͨ��ָ�� IP �� �˿� ���ӷ�����������ȡ��ʼ������Ϣ

	int start();			// ��������

	void stop(void);		// �ر�����ִ���߳�

private:
	Client() : clients(0), index(0), servSock(INVALID_SOCKET), msg_endpos(0),
		clock_signal(false), running(false), running_thds(0), postman(nullptr)
	{
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}

	static void send_thd(void);			// �����̵߳��ú�������շ������ݻ�����
	static void recv_thd(void);			// �����̵߳��ú�����д��������ݻ�����
	void thd_finished();				// �߳̽�����β�������߳�����һ

	static Client* cp;				// �������

	PostMan* postman;				// ���𽫽��յ���Ϣ�����䷢����

	int clients;					// ����˵����пͻ�����
	int index;						// �ڷ�����е� Index

	SOCKET servSock;				// ���������ӵ��׽���

	char send_msg[BUFSIZE] = { 0 };		// �������ݻ�����
	int msg_endpos;						// ��������βλ��
	std::mutex mt_send_msg;
	std::condition_variable cond_send_msg;

	//char recv_msg[BUFSIZE] = { 0 };		// �������ݻ�����
	//std::mutex mt_recv_msg;
	//std::condition_variable cond_recv_msg;

	bool clock_signal;					// ʱ���ź�
	std::mutex mt_signal;
	std::condition_variable cond_signal;

	volatile bool running;				// ����״̬
	int running_thds;					// ���������߳���
	std::mutex mt_thds;
	std::condition_variable cond_thds;
};




#endif