#include<iostream>
#include<conio.h>
#include<thread>
#include<cstdlib>
#include "../Little_Multiplayer_Online//MyWSAInfo.h"
#include "../Little_Multiplayer_Online//MyWSAInfo.cpp"
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
			Server server("CS_TEST", 1);
			cout << "SERVER: Waiting Connect: " << endl;
			server.waitConnect(true, true);			// OPEN MULTICAST AND SHOW LOG
		});
		thread thdc([=]() {

			Client localClient;
			char local_ip[IP_LENGTH] = { 0 };
			getLocalIP(local_ip);
			cout << "CLIENT: Local Connect: " << endl;
			localClient.connectServer("127.0.0.1", true);		// 也可以用 "127.0.0.1"


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