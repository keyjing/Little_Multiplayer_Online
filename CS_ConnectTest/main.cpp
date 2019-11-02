#include<iostream>
#include<conio.h>
#include<thread>
#include<cstdlib>

#include "../Little_Multiplayer_Online/Multicast.h"
#include "../Little_Multiplayer_Online/Multicast.cpp"
#include "../Little_Multiplayer_Online/Server.h"
#include "../Little_Multiplayer_Online/Server.cpp"
#include "../Little_Multiplayer_Online/Client.h"
#include "../Little_Multiplayer_Online/Client.cpp"

using namespace std;

volatile bool running = true;

int main()
{
	char ch;
	cout << "Server or Client(s or c): ";
	while ((ch = _getch()) != 's' && ch != 'c');
	cout << endl;

	if (ch == 's') {
		thread thds([=]()
			{
				Server server("CS TEST", 1, nullptr);
				server.waitConnect(true, true);
				server.start(true);
				while (running) Sleep(20);
				server.stop();
				cout << "SERVER: STOP." << endl;
			});
		thds.detach();

		Client localClient;
		localClient.connectServer("127.0.0.1", true);
		cout << endl << endl;
		localClient.start(false);
		while ((ch = _getch()) != 'q') localClient.addOpts(&ch, 1);
		cout << endl;
		running = false;
		Sleep(100);
		localClient.stop();

		//thread thdc([=]() {
		//	Sleep(100);
		//	Client localClient;
		//	/*char ip[IP_LENGTH] = { 0 };
		//	getLocalIP(ip);*/
		//	localClient.connectServer("127.0.0.1", true);		// 也可以用 "127.0.0.1"

		//	int trycnt = 100;
		//	while (localClient.start(true) == CLIENT_ERROR)
		//		if (--trycnt == 0) {
		//			cout << "CLIENT: WAIT ALL CLIENT OK TIME OUT.\n";
		//			return 1;
		//		}

		//	char ch[10] = "ABCDEFG";
		//	int cnt = 10;
		//	while (cnt-- > 0)
		//	{
		//		localClient.addOpts(ch, 10);
		//		Sleep(100);
		//	}
		//	localClient.stop();
		//	cout << endl << "LOCAL CLIENT: OVER!" << endl;
		//});
		//thdc.detach();

		//Server server("CS_TEST", 1, nullptr);
		//cout << "SERVER: Waiting Connect: " << endl;
		//server.waitConnect(true, true);			// OPEN MULTICAST AND SHOW LOG

		//if (server.start() == SERV_ERROR_SOCK) {
		//	cout << "SERVER: ERROR START.\n";
		//	return 1;
		//}

		//cout << "SERVER: Provide server until press 'q'!" << endl;
		//while ((ch = _getch()) != 'q');
		//server.stop();
		//cout << "SERVER: OVER!" << endl;
	}
	else {
		Client client;
		cout << "Connecting to Server: " << endl;
		client.connectServer(NULL, true);	//不指定服务器 IP，局域网查找
		cout << endl << endl;
		cout << "CLIENT: Send until press 'q'." << endl;

		int trycnt = 100;
		while (client.start(true) == CLIENT_ERROR)
			if (--trycnt == 0) {
				cout << "CLIENT: WAIT ALL CLIENT OK TIME OUT.\n";
				return 1;
			}

		while ((ch = _getch()) != 'q')
		{
			client.addOpts(&ch, 1, true);
		}
		client.stop();
		cout << endl << "CLIENT: OVER!" << endl;
	}

	system("pause");
	
	return 0;
}