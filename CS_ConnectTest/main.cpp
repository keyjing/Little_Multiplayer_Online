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
				sp->setName("CS TEST");
				sp->setClients(1);
				sp->start();
				while (client_running) Sleep(20);
				//this_thread::sleep_for(chrono::seconds(2));
				cout << "SERVER: TO STOP" << endl;
				sp->stop();
				cout << "SERVER: STOP FINISHED" << endl;
				serv_running = false;
			});
		thds.detach();

		Client* cp = Client::getInstance();
		cp->connServByIP("127.0.0.1");
		cp->start();
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
		cp->findServer();
		cp->start();
		cout << "Press keyboard to send and see detail in log." << endl;
		cout << "Pree 'q' to end." << endl;
		while ((ch = _getch()) != 'q') cp->addOpts(&ch, 1);
		cp->stop();
	}

	system("pause");
	
	return 0;
}