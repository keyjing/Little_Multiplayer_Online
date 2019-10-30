#include<iostream>
#include<conio.h>

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
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	char ch;
	cout << "Server or Client(s or c): ";
	while ((ch = _getch()) != 's' && ch != 'c');
	cout << endl;

	if (ch == 's') {
		Server server("CS_TEST", 2);
		cout << "Waiting Connect: " << endl;
		server.waitConnect(true);
	}
	else {
		Client client;
		cout << "Connecting to Server: " << endl;
		while (client.connectServer(true) < 0);k
	}

	WSACleanup();
	system("pause");
	return 0;
}