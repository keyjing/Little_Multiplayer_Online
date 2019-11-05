/******************************
*	测试每秒键盘的最多输入次数
*
*		测试的结果为 31
*
*******************************/
#include<iostream>
#include<Windows.h>
#include<thread>
#include<conio.h>		//包含 getch() 函数，立即获取键盘输入而不打印在控制台
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