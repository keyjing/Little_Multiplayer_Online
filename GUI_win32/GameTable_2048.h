#ifndef _GameTable_2048_h
#define _GameTable_2048_h

#include "WinGameControl.h"
#include "../Little_Multiplayer_Online//Game_2048.h"
#include "../Little_Multiplayer_Online/Server.h"

// ��Ϸ�̸�ʽ
#define MY_TABLE_WIDTH			230			// ���
#define MY_TABLE_HEIGHT			200			// �߶�
#define MY_ELEM_WIDTH			50			// Ԫ�ؿ��
#define MY_ELEM_HEIGHT			30			// Ԫ�ظ߶�
#define MY_COLS_WIDTH			4			// �м��
#define MY_ROWS_HEIGHT			6			// �м��
#define MY_TF_WIDTH				10			// ��ͬ�����м��
#define MY_TF_HEIGHT			50			// �м��
// �����ʽ
#define MYCREATEFRONT(x, y, weight)	CreateFont(-x /*�߶�*/, -y /*���*/, 0, 0, weight/*��ϸ*/, \
	FALSE/*б��?*/, FALSE/*�»���?*/, FALSE/*ɾ����?*/, DEFAULT_CHARSET, \
	OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, \
	FF_DONTCARE, TEXT("������"))

#define MY_TABLE_RES_DEFAULT	TEXT("")
#define MY_DEFAULT_SEED			99

#define GAME_OPT_UP				'w'
#define GAME_OPT_DOWN				's'
#define GAME_OPT_LEFT				'a'
#define GAME_OPT_RIGHT			'd'

class GameSeedCreator : public ControlOption
{
	// ͨ�� ControlOption �̳�
	virtual int createOpt(char* dst, int maxlen) override;
};


class GameTable_2048 :public WinContent
{
public:
	// ��ʼ����Ϸ��
	// @ parameter
	// @ id: ��Ϸ�̱��
	// @ x, y: ��Ϸ�̵�����
	// @ hWnd, hInst: �����ڡ����
	// @ startid: HMENU ��ʼ ID
	GameTable_2048(int id, int x, int y, HWND hWnd, int startid, HINSTANCE hInst, int seed);

	void show();		// ��ʾ��Ϸ��
	void hide();		// ������Ϸ��
	void setSeed(int _seed) { seed = _seed; }	// ���� 2048 ������

	void highlight() { ::SendMessage(htableFrame, WM_SETTEXT, NULL, (LPARAM)L"YOU"); }

	void showTitle(const char* result, int len);	// ��״̬�����

	void display(const char* msg, int len);		// ִ�в�������ʾ

private:
	HWND htableFrame;
	HWND htxt_res;
	HWND helems[GAME_SIZE][GAME_SIZE] = { {0} };
	Game_2048 game;
	int seed;
	int res;
};

#endif
