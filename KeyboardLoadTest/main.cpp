/******************************
*	����ÿ����̵�����������
*
*		���ԵĽ��Ϊ 31
*
*******************************/
#include<iostream>
#include<Windows.h>
#include<thread>
#include<conio.h>		//���� getch() ������������ȡ�������������ӡ�ڿ���̨
using namespace std;

volatile bool res = false;
volatile int cnt = 0;

void loopGetch()
{
	char ch;
	while ((ch = _getch()) != 'Q') { ++cnt; }
	res = true;
}

void changeNewLine()
{
	while (!res) {
		cout << " " << cnt << endl;
		cnt = 0;
		Sleep(1000);
	}
}

int main()
{
	thread thd1 = thread(loopGetch);
	thread thd2 = thread(changeNewLine);
	thd2.detach();
	thd1.join();

	return 0;
}