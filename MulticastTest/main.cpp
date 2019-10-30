#include<iostream>
#include<conio.h>
#include "../Little_Multiplayer_Online//MyWSAInfo.h"
#include "../Little_Multiplayer_Online//MyWSAInfo.cpp"
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
		char buffer[BUFSIZE] = { 0 };
		char ip[IP_LENGTH] = { 0 };
		if (mc.receiver(buffer, ip) == MC_NO_RECEVICE)
			cout << "NOTHING RECEIVED!" << endl;
		else
			cout << buffer << endl << ip << endl;
	}
	cout << "Over!" << endl;

	system("pause");

	return 0;
}
