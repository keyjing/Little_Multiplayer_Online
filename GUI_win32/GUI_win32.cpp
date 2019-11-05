// GUI_win32.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "GUI_win32.h"
#include "../Little_Multiplayer_Online/Multicast.h"
#include "../Little_Multiplayer_Online/Server.h"
#include "../Little_Multiplayer_Online/Client.h"
#include "../Little_Multiplayer_Online/Multicast.cpp"
#include "../Little_Multiplayer_Online/Server.cpp"
#include "../Little_Multiplayer_Online/Client.cpp"

#include<atlconv.h>

#define MY_MAIN_TITLE			"2048小游戏"

#define MAX_LOADSTRING			100
#define MY_GAME_SIZE			4			// 游戏盘格式: 4x4
#define MY_WIN_WIDTH			1000		// 窗口宽度
#define MY_WIN_HEIGHT			600			// 窗口高度
#define MY_MAX_PLAYERS			8
// 游戏盘格式
#define MY_TABLE_WIDTH			300			// 宽度
#define MY_TABLE_HEIGHT			300			// 高度
#define MY_ELEM_WIDTH			50			// 元素宽度
#define MY_ELEM_HEIGHT			30			// 元素高度
#define MY_COLS_WIDTH			4			// 列间隔
#define MY_ROWS_HEIGHT			6			// 行间隔
// Frame格式
#define MY_TABLE_FRAME_WIDTH	230
#define MY_TABLE_FRAME_HEIGHT	200
#define MY_TF_WIDTH				10
#define MY_TF_HEIGHT			50
// 字体格式
#define MYCREATEFRONT(x, y, weight)	CreateFont(-x /*高度*/, -y /*宽度*/, 0, 0, weight/*粗细*/, \
	FALSE/*斜体?*/, FALSE/*下划线?*/, FALSE/*删除线?*/, DEFAULT_CHARSET, \
	OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, \
	FF_DONTCARE, TEXT("新宋体"))

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// TODO: 全局数据
int players = 0;

// TODO: 全局控件
HWND hmainFrame;		// 主界面
HWND hservFrame;		// 创建服务器界面
HWND htxt_name;
HWND htxt_players;
HWND hclientFrame;		// 客户端界面
HWND hlist_serv_name;

HWND htabFrame[8];									// 游戏盘所在 Frame
HWND hGameTable[8][MY_GAME_SIZE][MY_GAME_SIZE];		// 游戏盘控件， 8人的 4X4
HWND hGameRes[8];									// 结果

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// TODO: 添加自己函数的向前声明:
void		initGameTable(HWND, int);
void		initMainFrame(HWND);
void		initServFrame(HWND);
void		initClientFrame(HWND);

// 原始的静态文本消息响应及自定义的消息响应
WNDPROC OriginStaticProc;
LRESULT CALLBACK MyStaticProc(HWND, UINT, WPARAM, LPARAM);

void		freshListBox(void);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	// 宏作用：避免编译器关于未引用参数的警告
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。
	// 替换掉原本静态文本的消息响应
	WNDCLASS ws;
	::GetClassInfo(hInst, L"static", &ws);
	OriginStaticProc = ws.lpfnWndProc;
	ws.lpfnWndProc = MyStaticProc;
	RegisterClass(&ws);

    // 初始化全局字符串
	/* LoadStringW(窗口句柄, 资源ID, 缓冲区地址, 缓冲区大小) -> int
	*		函数作用：将资源复制到缓冲区中
	*/
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GUIWIN32, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GUIWIN32));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
			if (LOWORD(msg.wParam) == IDB_CREATE_ROOM)
				MessageBox(NULL, L"BB", L"BB", NULL);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

	// CS_HREDRAW: 移动或调整使得宽度变化时重绘; CS_VREDRAW: ..高度变化时重绘;
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GUIWIN32));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= CreateSolidBrush(RGB(60, 179, 113));//(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GUIWIN32);
	//wcex.lpszMenuName	= nullptr;		// 取消菜单
	wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

	/*	CreateWindowW(窗口类名, 窗口标题, 窗口样式, X, Y, 宽度, 高度,
	*		父窗口句柄, 主菜单句柄, 实例窗口句柄, 附加参数) -> 窗口句柄
	*	窗口样式参考：	https://blog.csdn.net/qq_27361833/article/details/80338185
	*/
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
	   WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX,		// 无拉伸、无最大化
	   CW_USEDEFAULT, 0,	// (X, Y)
	   MY_WIN_WIDTH, MY_WIN_HEIGHT,	// 宽度，高度
	   nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		// TODO:
		initMainFrame(hWnd);
		initServFrame(hWnd);
		initClientFrame(hWnd);
		initGameTable(hWnd, 1);
		break;
	//case WM_CTLCOLORSTATIC:
	//	return (INT_PTR)GetStockObject(NULL_BRUSH);
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
				//for (int i = 0; i < 8; ++i)
				//	ShowWindow(htabFrame[i], bl ? SW_HIDE : SW_SHOW);
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// TODO: 添加函数，需要在前面添加向前声明

// 自定义的静态区域消息响应，对 Frame 中按钮的点击进行响应
LRESULT CALLBACK MyStaticProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			// 分析菜单选择:
			switch (wmId)
			{
			case IDB_CREATE_ROOM:
				ShowWindow(hmainFrame, SW_HIDE);
				ShowWindow(hservFrame, SW_SHOW);
				break;
			case IDB_JOIN_ROOM:
				ShowWindow(hmainFrame, SW_HIDE);
				ShowWindow(hclientFrame, SW_SHOW);
				break;
			case IDB_SERV_OK:
			{	
				// TODO: 获取编辑框文本
				int len = ::SendMessage(htxt_name, WM_GETTEXTLENGTH, NULL, NULL);
				WCHAR buf[256] = { 0 };
				::SendMessage(htxt_name, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
				//::MessageBox(NULL, buf, L"NAME", NULL);

				int len2 = ::SendMessage(htxt_players, WM_GETTEXTLENGTH, NULL, NULL);
				//if (len2 >= 2) {
				//	::MessageBox(hWnd, L"人数限制：1 ~ 8", L"超出范围", NULL);
				//	break;
				//}
				WCHAR buf2[256] = { 0 };
				::SendMessage(htxt_players, WM_GETTEXT, (WPARAM)len2 + 1, (LPARAM)buf2);
				USES_CONVERSION;
				char* p = W2A(buf2);
				players = 0;
				for (int i = 0; p[i] >= '0' && p[i] <= '9'; ++i)
					players = players * 10 + (p[i] - '0');
				char buf3[256] = { 0 };
				sprintf_s(buf3, "玩家数： %d", players);
				WCHAR* wp = A2W(buf3);
				if (players <= 0 || players > 8) {
					::MessageBox(hWnd, L"人数限制：1 ~ 8", L"超出范围", NULL);
					break;
				}
				::MessageBox(hWnd, wp, L"MESSAGE", NULL);

				ShowWindow(hservFrame, SW_HIDE);
				for (int i = 0; i < MY_MAX_PLAYERS; ++i)
					ShowWindow(htabFrame[i], SW_SHOW);
				break;
			}
			case IDB_CLIE_OK:
				{	
					// 获取列表框中的选中索引， -1 为没选， 选中索引从 0 开始
					int index = ::SendMessage(hlist_serv_name, LB_GETCURSEL, NULL, NULL);
					char buf[256] = { 0 };
					sprintf_s(buf, "选中了 %d", index);
					WCHAR wch[256] = { 0 };
					USES_CONVERSION;
					WCHAR* p = A2W(buf);
					wcscpy_s(wch, p);
					::MessageBox(hWnd, wch, L"消息", NULL);
					break;
				}
			case IDB_CLIE_FRESH:
				// TODO:
				freshListBox();
				break;
			case IDB_CLIE_CANCEL:
			case IDB_SERV_CANCEL:
				ShowWindow(hservFrame, SW_HIDE);
				ShowWindow(hclientFrame, SW_HIDE);
				ShowWindow(hmainFrame, SW_SHOW);
				break;
			default:
				// 交由原本的响应函数处理
				return OriginStaticProc(hWnd, message, wParam, lParam);
			}
		}
		break;
	default:
		// 交由原本的响应函数处理
		return OriginStaticProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*		进行表格元素布局
* @parameter
* @             hWnd: 父窗口句柄
* @ htab, rows, cols: 表格元素句柄数组、行数、列数
* @          startid: 起始 HMENU ID，有50个可用
*/
void initTabFrame(HWND hWnd, HWND htab[][MY_GAME_SIZE], int startid)
{
	int height_ocp = MY_ELEM_HEIGHT + MY_ROWS_HEIGHT;	// 实际占据高度
	int width_ocp = MY_ELEM_WIDTH + MY_COLS_WIDTH;		// 实际占据宽度
	int x = 2;
	int y = 40;
	static HFONT hfont = //MYCREATEFRONT(16, 8, FW_LIGHT);
		CreateFont(-14 /*高度*/, -7 /*宽度*/, 0, 0, FW_LIGHT/*粗细*/, \
			FALSE/*斜体?*/, FALSE/*下划线?*/, FALSE/*删除线?*/, DEFAULT_CHARSET, \
			OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, \
			FF_DONTCARE, TEXT("微软雅黑"));
	for (int i = 0; i < MY_GAME_SIZE; ++i)
	{
		int id = startid + i * MY_GAME_SIZE;;
		for (int j = 0; j < MY_GAME_SIZE; ++j)
		{
			htab[i][j] = CreateWindowW(L"static", L"2048",
				WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,	// 字体居中
				x + width_ocp * j, y + height_ocp * i,
				MY_ELEM_WIDTH, MY_ELEM_HEIGHT,
				hWnd, HMENU(id + j), hInst, nullptr);
			::SendMessage(htab[i][j], WM_SETFONT, (WPARAM)hfont, NULL);
		}
	}
}

void initGameTable(HWND hWnd, int players)
{
	// 字体
	static HFONT hfont = MYCREATEFRONT(16, 8, FW_BOLD);
	// 标题，需要从 char 转化为 WCHAR
	char title[256] = { 0 };
	WCHAR wch[256];
	for (int i = 0; i < MY_MAX_PLAYERS; ++i)
	{
		// ASNI 转化为 Unicode
		sprintf_s(title, "玩家 %d", i + 1);
		USES_CONVERSION;//宏
		WCHAR* p = A2W(title);
		wcscpy_s(wch, p);
		// 创建 Frame
		htabFrame[i] = CreateWindowW(L"static", wch, WS_CHILD | WS_DLGFRAME /*| WS_VISIBLE*/,
			15 + (i % 4) * (MY_TABLE_FRAME_WIDTH + MY_TF_WIDTH), 60 + (i / 4) * (MY_TABLE_FRAME_HEIGHT + MY_TF_HEIGHT),
			MY_TABLE_FRAME_WIDTH, MY_TABLE_FRAME_HEIGHT,
			hWnd, HMENU(IDF_MY_TAB_0 + i * 10), hInst, nullptr);
		// 添加表格元素
		initTabFrame(htabFrame[i], hGameTable[i], IDT_MY_TABLE_0 + 50 * i);
		// 添加信息输出 LABLE
		hGameRes[i] = CreateWindowW(L"static", L"GAME OVER!", WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
			100, 0, 100, 30, htabFrame[i], HMENU(IDF_MY_TAB_0 + i * 10 + 1), hInst, nullptr);
		// 设置字体
		::SendMessage(htabFrame[i], WM_SETFONT, (WPARAM)hfont, NULL);
		::SendMessage(hGameRes[i], WM_SETFONT, (WPARAM)hfont, NULL);
	}

	::SendMessage(htabFrame[3], WM_SETTEXT, 0, (LPARAM)(LPCTSTR)L"YOU");
}

void initMainFrame(HWND hWnd)
{
	hmainFrame = CreateWindowW(L"static", NULL, WS_CHILD | WS_BORDER | WS_VISIBLE,
		MY_WIN_WIDTH / 2 - 210, MY_WIN_HEIGHT / 2 - 230, 400, 400,
		hWnd, (HMENU)IDF_MAIN_FRAME, hInst, nullptr);
	HWND htitle = CreateWindowW(L"static", TEXT(MY_MAIN_TITLE), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		100, 50, 200, 60, hmainFrame, HMENU(IDF_MAIN_FRAME + 1), hInst, nullptr);
	HWND hbtn_create = CreateWindowW(L"button", L"创建新房间", WS_CHILD | WS_VISIBLE,
		150, 180, 100, 30, hmainFrame, (HMENU)IDB_CREATE_ROOM, hInst, nullptr);
	HWND hbtn_join = CreateWindowW(L"button", L"进入房间", WS_CHILD | WS_VISIBLE,
		150, 250, 100, 30, hmainFrame, (HMENU)IDB_JOIN_ROOM, hInst, nullptr);
	// 字体
	static HFONT hfont = MYCREATEFRONT(28, 14, FW_BOLD);
	::SendMessage(htitle, WM_SETFONT, (WPARAM)hfont, NULL);
	// 字体2
	static HFONT hfont2 = MYCREATEFRONT(16, 8, FW_LIGHT);
	::SendMessage(hbtn_create, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hbtn_join, WM_SETFONT, (WPARAM)hfont2, NULL);
}

void initServFrame(HWND hWnd)
{
	hservFrame = CreateWindowW(L"static", NULL, WS_CHILD | WS_BORDER /*| WS_VISIBLE*/,
		MY_WIN_WIDTH / 2 - 210, MY_WIN_HEIGHT / 2 - 230, 400, 400,
		hWnd, (HMENU)IDF_SERVER_FRAME, hInst, nullptr);
	HWND htitle = CreateWindowW(L"static", TEXT("创建新房间"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		100, 50, 200, 60, hservFrame, HMENU(IDF_SERVER_FRAME + 1), hInst, nullptr);
	HWND hlab_name = CreateWindowW(L"static", TEXT("输入房间名："), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		120, 120, 160, 30, hservFrame, HMENU(IDF_SERVER_FRAME + 2), hInst, nullptr);
	htxt_name = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		120, 150, 160, 30, hservFrame, (HMENU)IDT_SERV_NAME, hInst, NULL);
	HWND hlab_players = CreateWindowW(L"static", TEXT("输入玩家数：1-8"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		120, 200, 160, 30, hservFrame, HMENU(IDF_SERVER_FRAME + 3), hInst, nullptr);
	htxt_players = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
		120, 230, 160, 30, hservFrame, (HMENU)IDT_SERV_PLAYERS, hInst, NULL);
	HWND hbtn_serv_ok = CreateWindowW(L"button", L"确认", WS_CHILD | WS_VISIBLE,
		100, 300, 50, 30, hservFrame, (HMENU)IDB_SERV_OK, hInst, nullptr);
	HWND hbtn_serv_cancel = CreateWindowW(L"button", L"取消", WS_CHILD | WS_VISIBLE,
		250, 300, 50, 30, hservFrame, (HMENU)IDB_SERV_CANCEL, hInst, nullptr);
	// 字体
	static HFONT hfont = MYCREATEFRONT(28, 14, FW_BOLD);
	::SendMessage(htitle, WM_SETFONT, (WPARAM)hfont, NULL);
	// 字体2
	static HFONT hfont2 = MYCREATEFRONT(14, 7, FW_LIGHT);
	::SendMessage(hlab_name, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_name, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hlab_players, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_players, WM_SETFONT, (WPARAM)hfont2, NULL);
	// 字体3 4
	static HFONT hfont3 = MYCREATEFRONT(16, 8, FW_BOLD);
	static HFONT hfont4 = MYCREATEFRONT(16, 8, FW_LIGHT);
	::SendMessage(hbtn_serv_ok, WM_SETFONT, (WPARAM)hfont3, NULL);
	::SendMessage(hbtn_serv_cancel, WM_SETFONT, (WPARAM)hfont4, NULL);
}

void initClientFrame(HWND hWnd)
{
	hclientFrame = CreateWindowW(L"static", NULL, WS_CHILD | WS_BORDER /*| WS_VISIBLE*/,
		MY_WIN_WIDTH / 2 - 210, MY_WIN_HEIGHT / 2 - 230, 400, 400,
		hWnd, (HMENU)IDF_CLIENT_FRAME, hInst, nullptr);
	HWND htitle = CreateWindowW(L"static", TEXT("加入房间"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		100, 50, 200, 60, hclientFrame, HMENU(IDF_CLIENT_FRAME + 1), hInst, nullptr);
	// 列表框
	hlist_serv_name = CreateWindowW(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
		80, 120, 240, 170, hclientFrame, (HMENU)IDCB_CLIE_BOX, hInst, nullptr);
	char ch[256] = "Hello World!";
	//::SendMessage(hlist_serv_name, LB_ADDSTRING, NULL, (LPARAM)L"选项 1");
	//::SendMessage(hlist_serv_name, LB_ADDSTRING, NULL, (LPARAM)L"选项 2");
	//::SendMessage(hlist_serv_name, LB_ADDSTRING, NULL, (LPARAM)L"选项 3");
	freshListBox();

	HWND hbtn_clie_ok = CreateWindowW(L"button", L"确认", WS_CHILD | WS_VISIBLE,
		75, 300, 50, 30, hclientFrame, (HMENU)IDB_CLIE_OK, hInst, nullptr);
	HWND hbtn_clie_fresh = CreateWindowW(L"button", L"刷新", WS_CHILD | WS_VISIBLE,
		175, 300, 50, 30, hclientFrame, (HMENU)IDB_CLIE_FRESH, hInst, nullptr);
	HWND hbtn_clie_cancel = CreateWindowW(L"button", L"取消", WS_CHILD | WS_VISIBLE,
		275, 300, 50, 30, hclientFrame, (HMENU)IDB_CLIE_CANCEL, hInst, nullptr);
	// 字体
	static HFONT hfont = MYCREATEFRONT(28, 14, FW_BOLD);
	::SendMessage(htitle, WM_SETFONT, (WPARAM)hfont, NULL);
	// 字体2
	static HFONT hfont2 = MYCREATEFRONT(18, 9, FW_LIGHT);
	::SendMessage(hlist_serv_name, WM_SETFONT, (WPARAM)hfont2, NULL);
	// 字体3 4
	static HFONT hfont3 = MYCREATEFRONT(16, 8, FW_BOLD);
	static HFONT hfont4 = MYCREATEFRONT(16, 8, FW_LIGHT);
	::SendMessage(hbtn_clie_ok, WM_SETFONT, (WPARAM)hfont3, NULL);
	::SendMessage(hbtn_clie_fresh, WM_SETFONT, (WPARAM)hfont4, NULL);
	::SendMessage(hbtn_clie_cancel, WM_SETFONT, (WPARAM)hfont4, NULL);
}

void freshListBox(void)
{
	::SendMessage(hlist_serv_name, LB_RESETCONTENT, NULL, NULL);
	static char list[10][256] = { {0} };
	static int n = 0;
	n = (n + 1) % 6;
	USES_CONVERSION;
	for (int i = 0; i < n+5; ++i)
	{
		sprintf_s(list[i], "选项 %d", i);
		WCHAR* p = A2W(list[i]);
		::SendMessage(hlist_serv_name, LB_ADDSTRING, NULL, (LPARAM)p);
	}
}