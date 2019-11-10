#include<iostream>
#include<conio.h>

#include "../Little_Multiplayer_Online/Multicast.h"
#include "../Little_Multiplayer_Online/Multicast.cpp"

using namespace std;

int main()
{
	char ch;
	cout << "Server or Client(s or c): ";
	while ((ch = _getch()) != 's' && ch != 'c');
	cout << endl;

	Multicast mc("233.0.0.1", 2333);

	if (ch == 's') {
		cout << "Sending \"Hello World\" until press 'q'!" << endl;
		mc.sender("Hello World");
		while ((ch = _getch()) != 'q');
		mc.turnOff();
	}
	else {
		cout << "Receving: ";
		char buffer[10][MC_MSG_LENGTH] = { 0 };
		char ip[10][IP_LENGTH] = { 0 };
		int maxfound = 10;
		int timelimit = 3000;
		if (mc.receiver(maxfound, timelimit, buffer, ip) == MC_NO_RECEVICE)
			cout << "NOTHING RECEIVED!" << endl;
		else
			cout << buffer << endl << ip << endl;
	}
	cout << "Over!" << endl;

	system("pause");

	return 0;
}
