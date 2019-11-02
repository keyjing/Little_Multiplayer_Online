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

int main()
{
	char ch;
	cout << "Server or Client(s or c): ";
	while ((ch = _getch()) != 's' && ch != 'c');
	cout << endl;

	if (ch == 's') {
		thread thds([=]() {
			Server server("CS_TEST", 1, nullptr);
			cout << "SERVER: Waiting Connect: " << endl;
			server.waitConnect(true, true);			// OPEN MULTICAST AND SHOW LOG
			Sleep(1000);
			cout << "SERVER: Provide server until press 'q'!" << endl;
			cout << "SERVER: With log showing? (y/n): ";
			char ch;
			while ((ch = _getch()) != 'y' && ch != 'n');
			cout << endl << "SERVER: Starting..." << endl;
			server.start(ch == 'y');
			while ((ch = _getch()) != 'q');
			server.stop();
			cout << "SERVER: OVER!" << endl;
		});
		thread thdc([=]() {

			Client localClient;
			char local_ip[IP_LENGTH] = { 0 };
			getLocalIP(local_ip);
			cout << "CLIENT: Local Connect: " << endl;
			localClient.connectServer("127.0.0.1", true);		// 也可以用 "127.0.0.1"
			Sleep(2000);

		});
		thds.join();
		thdc.detach();
	}
	else {
		Client client;
		cout << "Connecting to Server: " << endl;
		client.connectServer(NULL, true);	//不指定服务器 IP，局域网查找
	}

	system("pause");
	
	return 0;
}