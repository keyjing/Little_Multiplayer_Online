#ifndef _GameTable_2048_h
#define _GameTable_2048_h

#include "WinGameControl.h"
#include "../Little_Multiplayer_Online//Game_2048.h"
#include "../Little_Multiplayer_Online/Server.h"

// 游戏盘格式
#define MY_TABLE_WIDTH			230			// 宽度
#define MY_TABLE_HEIGHT			200			// 高度
#define MY_ELEM_WIDTH			50			// 元素宽度
#define MY_ELEM_HEIGHT			30			// 元素高度
#define MY_COLS_WIDTH			4			// 列间隔
#define MY_ROWS_HEIGHT			6			// 行间隔
#define MY_TF_WIDTH				10			// 不同表格的列间隔
#define MY_TF_HEIGHT			50			// 行间隔
// 字体格式
#define MYCREATEFRONT(x, y, weight)	CreateFont(-x /*高度*/, -y /*宽度*/, 0, 0, weight/*粗细*/, \
	FALSE/*斜体?*/, FALSE/*下划线?*/, FALSE/*删除线?*/, DEFAULT_CHARSET, \
	OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, \
	FF_DONTCARE, TEXT("新宋体"))

#define MY_TABLE_RES_DEFAULT	TEXT("")
#define MY_DEFAULT_SEED			99

#define GAME_OPT_UP				'w'
#define GAME_OPT_DOWN				's'
#define GAME_OPT_LEFT				'a'
#define GAME_OPT_RIGHT			'd'

class GameSeedCreator : public ControlOption
{
	// 通过 ControlOption 继承
	virtual int createOpt(char* dst, int maxlen) override;
};


class GameTable_2048 :public WinContent
{
public:
	// 初始化游戏盘
	// @ parameter
	// @ id: 游戏盘标号
	// @ x, y: 游戏盘的坐标
	// @ hWnd, hInst: 父窗口、句柄
	// @ startid: HMENU 起始 ID
	GameTable_2048(int id, int x, int y, HWND hWnd, int startid, HINSTANCE hInst, int seed);

	void show();		// 显示游戏盘
	void hide();		// 隐藏游戏盘
	void setSeed(int _seed) { seed = _seed; }	// 设置 2048 的种子

	void highlight() { ::SendMessage(htableFrame, WM_SETTEXT, NULL, (LPARAM)L"YOU"); }

	void showTitle(const char* result, int len);	// 在状态栏输出

	void display(const char* msg, int len);		// 执行操作并显示

private:
	HWND htableFrame;
	HWND htxt_res;
	HWND helems[GAME_SIZE][GAME_SIZE] = { {0} };
	Game_2048 game;
	int seed;
	int res;
};

#endif
