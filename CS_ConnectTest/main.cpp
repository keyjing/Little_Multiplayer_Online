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

volatile bool client_running = true;
volatile bool serv_running = false;

int main()
{
	char ch;
	cout << "Server or Client(s or c): ";
	while ((ch = _getch()) != 's' && ch != 'c');
	cout << endl;

	if (ch == 's') {
		thread thds([=]()
			{
				serv_running = true;
				Server* sp = Server::getInstance();
				sp->start("CS_TEST", 1234, 1, MC_DEFAULT_ADDR, MC_DEFAULT_PORT);
				while (client_running) Sleep(20);
				//this_thread::sleep_for(chrono::seconds(2));
				cout << "SERVER: TO STOP" << endl;
				sp->stop();
				cout << "SERVER: STOP FINISHED" << endl;
				serv_running = false;
			});
		thds.detach();

		Client* cp = Client::getInstance();
		cp->start("127.0.0.1", 1234);
		cout << "Press keyboard to send and see detail in log." << endl;
		cout << "Pree 'q' to end." << endl;
		while ((ch = _getch()) != 'q') cp->addOpts(&ch, 1);
		cout << endl;
		client_running = false;

		cout << "CLIENT: TO STOP" << endl;
		cp->stop();
		cout << "CLIENT: STOP FINISHED" << endl;

		while (serv_running) Sleep(20);
	}
	else {
		Client* cp = Client::getInstance();
		
		char name[MAX_FOUND_SERVER][BUFSIZE] = { {0} };
		char ip[MAX_FOUND_SERVER][IP_LENGTH] = { {0} };
		int port[MAX_FOUND_SERVER] = { 0 };

		int found = cp->findServer(MC_DEFAULT_ADDR, MC_DEFAULT_PORT, MAX_FOUND_SERVER,
			1000, name, ip, port);

		cout << "found = " << found << endl;
		if (found <= 0)
			return 1;
		for (int i = 0; i < found; ++i)
			cout << i << ". " << name[i] << " " << ip[i] << " " << port[i] << endl;
		cout << "Select index: ";
		int index = -1;
		while (index < 0 || index >= found) cin >> index;

		cp->start(ip[index], port[index]);

		cout << "Press keyboard to send and see detail in log." << endl;
		cout << "Pree 'q' to end." << endl;
		while ((ch = _getch()) != 'q') cp->addOpts(&ch, 1);
		cp->stop();
	}

	system("pause");
	
	return 0;
}