#ifndef _Server_h
#define _Server_h

#include"MyWSAInfo.h"
#include "Multicast.h"
#include<thread>

#define MAX_CONNECT			10				// ��������
#define MAX_BACKLOG			20				// ������������
#define SERV_PORT			1234			// �������˿�
#define SERV_MC_ADDR		"230.0.0.1"		// �ಥ���ַ
#define SERV_MC_PORT		2333			// �ಥ��˿�

#define SERV_SUCCESS		0
#define SERV_ERROR_SOCK		-1
#define SERV_ERROR_NO_MC	-2

class ControlOption				// Interface: �������Ŀ����������
{
public:
	virtual char createOpt() = 0;
};

class Server
{
	char name[BUFSIZE] = { 0 };				// ��������
	int clients = 0;						// ���ƵĿͻ����������� hostReserve
	int hostReserve = 1;					// �����Ŀͻ�������Ĭ�ϱ���һ��Ϊ������ר��
	SOCKET clientsSock[MAX_CONNECT] = { INVALID_SOCKET };	// ��ͻ������ӵ��׽���
	char options[MAX_CONNECT];				// �����ͷ��˵Ĳ���ָ�Ĭ���±� 0 Ϊ Server �����Ŀ���ָ��
	ControlOption* ctrlOpt = nullptr;		// Server����ָ������Ľӿ�

	volatile bool working = false;					// ������״̬
	volatile bool is_working_thd_stop = true;		// �����߳�״̬

	void setName(const char* _name) { ::charArrayCopy(name, _name, BUFSIZE); }
	void setClients(int _clients) {
		clients = _clients;
		if (clients < 0) clients = 0;
		else if (clients >= MAX_CONNECT) clients = MAX_CONNECT;
	}

	static void working_thd(Server* sp);		// �����̣߳��̵߳��ã���Ҫ�� static

	static int recvEveryOption();

	static int sendEachOptionS();

public:
	Server(const char* _name, int _clients) { 
		setName(_name); setClients(_clients);
		WSADATA wsa;
		::WSAStartup(MAKEWORD(2, 2), &wsa);
	}
	~Server() {
		while (!is_working_thd_stop) {		// ���̷߳��������źţ����̵߳ȴ�����
			stop();
			::Sleep(100);
		}
		if (ctrlOpt) delete ctrlOpt;
		for (auto elem : clientsSock)
			if (elem != INVALID_SOCKET) ::closesocket(elem);
		::WSACleanup();
	}

	void setHostReserve(int n) {
		hostReserve = n;
		if (n < 1) hostReserve = 1;
		else if (n >= MAX_CONNECT) hostReserve = MAX_CONNECT;
	}
	
	void setControlOpt(ControlOption* _ctrlOpt) { 
		if (ctrlOpt) delete ctrlOpt;
		ctrlOpt = _ctrlOpt; 
	}

	int waitConnect(bool openMulticast, bool showLog = false);

	void startWork() {					// ���� LockStep �㷨���й���
		std::thread thd(working_thd, this); 
		thd.detach();			// �����̷߳��룬�����������б�֤����
	}

	void stop() { working = false; }	//�����̷߳���ֹͣ�ź�
};

#endif
