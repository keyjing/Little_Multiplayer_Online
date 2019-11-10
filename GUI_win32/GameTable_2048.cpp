#include "GameTable_2048.h"
#include <atlconv.h>
#include<cstdlib>
#include<ctime>

static int rank = 1;

GameTable_2048::GameTable_2048(int id, int x, int y, HWND hWnd, int startid, HINSTANCE hInst, int _seed)
	:game(_seed), seed(_seed), res(GAME_RES_CONTINUE)
{
	char buf[20] = { 0 };
	sprintf_s(buf, 20, "玩家 %d", id + 1);
	USES_CONVERSION;	//宏
	WCHAR* p = A2W(buf);	//需要转化为宽字节

	// 创建 Frame
	htableFrame = CreateWindowW(L"static", p, WS_CHILD | WS_DLGFRAME /*| WS_VISIBLE*/,
		x, y, MY_TABLE_WIDTH, MY_TABLE_HEIGHT, hWnd, (HMENU)startid++, hInst, nullptr);

	// 添加信息输出 LABLE
	htxt_res = CreateWindowW(L"static", MY_TABLE_RES_DEFAULT, WS_CHILD | /*WS_VISIBLE |*/ SS_CENTER | SS_CENTERIMAGE,
		0, MY_TABLE_HEIGHT / 2 - 15,
		MY_TABLE_WIDTH, 30, htableFrame, (HMENU)startid++, hInst, nullptr);

	// 标题字体
	static HFONT hfont_title = MYCREATEFRONT(16, 8, FW_BOLD);
	::SendMessage(htableFrame, WM_SETFONT, (WPARAM)hfont_title, NULL);
	::SendMessage(htxt_res, WM_SETFONT, (WPARAM)hfont_title, NULL);

	// 添加表格元素
	int height_ocp = MY_ELEM_HEIGHT + MY_ROWS_HEIGHT;	// 实际占据高度
	int width_ocp = MY_ELEM_WIDTH + MY_COLS_WIDTH;		// 实际占据宽度
	int x0 = 4;		// 第一个元素的坐标
	int y0 = 40;
	// 数字字体
	static HFONT hfont = //MYCREATEFRONT(16, 8, FW_LIGHT);
		CreateFont(-14 /*高度*/, -7 /*宽度*/, 0, 0, FW_LIGHT/*粗细*/, \
			FALSE/*斜体?*/, FALSE/*下划线?*/, FALSE/*删除线?*/, DEFAULT_CHARSET, \
			OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, \
			FF_DONTCARE, TEXT("微软雅黑"));
	for (int i = 0; i < GAME_SIZE; ++i)
	{
		for (int j = 0; j < GAME_SIZE; ++j)
		{
			sprintf_s(buf, 20, "%d", game.get(i, j));
			p = A2W(buf);
			helems[i][j] = CreateWindowW(L"static", p,
				WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,	// 字体居中
				x0 + width_ocp * j, y0 + height_ocp * i,
				MY_ELEM_WIDTH, MY_ELEM_HEIGHT,
				htableFrame, (HMENU)startid++, hInst, nullptr);
			::SendMessage(helems[i][j], WM_SETFONT, (WPARAM)hfont, NULL);
		}
	}
}

void GameTable_2048::show()
{
	::ShowWindow(htableFrame, SW_SHOW);
}

void GameTable_2048::hide()
{
	::ShowWindow(htableFrame, SW_HIDE);
}

void GameTable_2048::showTitle(const char* result, int len)
{
	char* buf = new char[len + 1];
	::memcpy(buf, result, sizeof(char) * len);
	buf[len] = '\0';
	USES_CONVERSION;
	WCHAR* p = A2W(buf);
	::SendMessage(htxt_res, WM_SETTEXT, NULL, (LPARAM)p);
}

void GameTable_2048::display(const char* msg, int len)
{
	if (res != GAME_RES_CONTINUE) return;	// 已结束
	
	static int count = 0;
	
	char buf[50] = { 0 };
	WCHAR* p; 
	USES_CONVERSION;
	for (int i = 0; i < len; i++)
	{
		++count;
		switch (msg[i])
		{
		case POST_OPT_ALL:
			seed = msg[i + 1];
			return;
		case GAME_OPT_UP:
			res = game.up(seed);
			seed = seed << 1;
			break;
		case GAME_OPT_DOWN:
			res = game.down(seed);
			seed = seed + 1;
			break;
		case GAME_OPT_LEFT:
			res = game.left(seed);
			seed = seed + 2;
			break;
		case GAME_OPT_RIGHT:
			res = game.right(seed);
			seed = seed >> 1;
			break;
		default:
			continue;
		}
		for (int x = 0; x < GAME_SIZE; ++x)
		{
			for (int y = 0; y < GAME_SIZE; ++y)
			{
				sprintf_s(buf, 50, "%d", game.get(x, y));
				p = A2W(buf);
				::SendMessage(helems[x][y], WM_SETTEXT, NULL, (LPARAM)p);
			}
		}
		switch (res)
		{
		case GAME_RES_SUCCESS:
			sprintf_s(buf, 50, "WIN! Rank %d Conut: %d", rank++, count);
			p = A2W(buf);
			::SendMessage(htxt_res, WM_SETTEXT, NULL, (LPARAM)p);
			break;
		case GAME_RES_FAILED:
			sprintf_s(buf, 50, "FAILED! Conut: %d", count);
			p = A2W(buf);
			::SendMessage(htxt_res, WM_SETTEXT, NULL, (LPARAM)p);
			break;
		default:
			continue;
		}
		for (int i = 0; i < GAME_SIZE; ++i)
			for (int j = 0; j < GAME_SIZE; ++j)
				::ShowWindow(helems[i][j], SW_HIDE);
		::ShowWindow(htxt_res, SW_SHOW);
	}
}

int GameSeedCreator::createOpt(char* dst, int maxlen)
{
	if (maxlen <= 0) return 0;
	srand((int)time(0));
	dst[0] = rand() % 50 + 50;
	return 1;
}
