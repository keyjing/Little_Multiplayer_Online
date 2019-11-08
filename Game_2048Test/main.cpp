#include<iostream>
#include"../Little_Multiplayer_Online/Game_2048.h"
#include"../Little_Multiplayer_Online/Game_2048.cpp"
#include<conio.h>

using namespace std;

int main()
{
	Game_2048 game(99);
	char ch = 100;
	while (1)
	{
		for (int i = 0; i < GAME_SIZE; ++i)
		{
			for (int j = 0; j < GAME_SIZE; ++j)
				cout << game.matrix[i][j] << " ";
			cout << endl;
		}
		ch = _getch();
		int res = 0;
		switch (ch)
		{
		case 'w': res = game.up(99); break;
		case 's': res = game.down(99); break;
		case 'a': res = game.left(99); break;
		case 'd': res = game.right(99); break;
		case 'q': return 0;
		}
		switch (res)
		{
		case GAME_RES_FAILED:
			cout << "FAILED" << endl;
			return 1;
		case GAME_RES_SUCCESS:
			cout << "SUCCESS" << endl;
			return 0;
		default:
			system("cls");
		}
	}

	return 0;
}