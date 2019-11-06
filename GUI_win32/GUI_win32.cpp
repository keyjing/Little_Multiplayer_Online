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

#define DEFAULT_SERV_PORT		"1234"
#define MAX_FOUND_TIME			3000

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
Server* sp = nullptr;
Client* cp = nullptr;

struct FoundServerResult fdservs;

//int players = 0;

// TODO: 全局控件
HWND hmainFrame;		// 主界面

HWND hservFrame;		// 创建服务器界面
HWND htxt_name;			// 服务器名
HWND htxt_players;		// 人数
HWND htxt_port;			// 端口
HWND htxt_mc_ip;		// 多播地址
HWND htxt_mc_port;		// 多播端口

HWND hclientFrame;			// 客户端界面
HWND hlist_serv_name;		// 服务器列表
HWND htxt_found_mc_ip;		// 查找多播地址
HWND htxt_found_mc_port;	// 查找多播端口

HWND htabFrame[8];									// 游戏盘所在 Frame
HWND hGameTable[8][MY_GAME_SIZE][MY_GAME_SIZE];		// 游戏盘控件， 8人的 4X4
HWND hGameRes[8];									// 结果

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// TODO: 添加自己函数的向前声明:
void	initGameTable(HWND, int);		// 创建游戏界面
void	initMainFrame(HWND);			// 创建主界面
void	initServFrame(HWND);			// 创建服务器界面
void	initClientFrame(HWND);			// 创建客户端界面
void	freshListBox(HWND);				// 刷新客户端界面的选择列表
void	GetServFrameMsg(HWND);			// 获取服务端界面的信息，并进行处理
void	GetClientFrameMsg(HWND);		// 获取客户端界面的信息，并进行处理
void	wcharToArrayChar(char*, const WCHAR*);
int		wcharToInt(const WCHAR*);

// 原始的静态文本消息响应及自定义的消息响应
WNDPROC	OriginStaticProc;									// 静态区域控件原本的响应函数	
LRESULT CALLBACK MyStaticProc(HWND, UINT, WPARAM, LPARAM);	// 自定义静态区域控件响应函数

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
				sp = Server::getInstance();
				ShowWindow(hmainFrame, SW_HIDE);
				ShowWindow(hservFrame, SW_SHOW);
				break;
			case IDB_JOIN_ROOM:
				cp = Client::getInstance();
				ShowWindow(hmainFrame, SW_HIDE);
				ShowWindow(hclientFrame, SW_SHOW);
				freshListBox(hWnd);
				break;
			case IDB_SERV_OK:
				GetServFrameMsg(hWnd);
				break;
			case IDB_CLIE_OK:
				GetClientFrameMsg(hWnd);
				break;
			case IDB_CLIE_FRESH:
				// TODO:
				freshListBox(hWnd);
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
	int default_id = IDF_SERVER_FRAME;
	hservFrame = CreateWindowW(L"static", NULL, WS_CHILD | WS_BORDER /*| WS_VISIBLE*/,
		MY_WIN_WIDTH / 2 - 210, MY_WIN_HEIGHT / 2 - 230, 400, 400,
		hWnd, (HMENU)default_id++, hInst, nullptr);
	// 标题
	HWND htitle = CreateWindowW(L"static", TEXT("创建新房间"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		100, 50, 200, 60, hservFrame, (HMENU)default_id++, hInst, nullptr);
	// 房间名
	HWND hlab_name = CreateWindowW(L"static",    TEXT("     输入房间名:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		20, 110, 130, 25, hservFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_name = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		160, 110, 190, 25, hservFrame, (HMENU)IDT_SERV_NAME, hInst, NULL);
	// 人数
	HWND hlab_players = CreateWindowW(L"static", TEXT("输入玩家数(1-8):"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		20, 150, 130, 25, hservFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_players = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
		160, 150, 190, 25, hservFrame, (HMENU)IDT_SERV_PLAYERS, hInst, NULL);
	// 端口
	HWND hlab_port = CreateWindowW(L"static",    TEXT("     服务器端口:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		20, 190, 130, 25, hservFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_port = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
		160, 190, 190, 25, hservFrame, (HMENU)IDT_SERV_PORT, hInst, NULL);
	// 多播地址
	HWND hlab_mc_ip = CreateWindowW(L"static",   TEXT("         多播IP:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		20, 230, 130, 25, hservFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_mc_ip = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		160, 230, 190, 25, hservFrame, (HMENU)IDT_SERV_MC_IP, hInst, NULL);
	// 多播端口
	HWND hlab_mc_port = CreateWindowW(L"static", TEXT("       多播端口:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		20, 270, 130, 25, hservFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_mc_port = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
		160, 270, 190, 25, hservFrame, (HMENU)IDT_SERV_MC_PORT, hInst, NULL);
	// 按钮
	HWND hbtn_serv_ok = CreateWindowW(L"button", L"确认", WS_CHILD | WS_VISIBLE,
		100, 320, 50, 25, hservFrame, (HMENU)IDB_SERV_OK, hInst, nullptr);
	HWND hbtn_serv_cancel = CreateWindowW(L"button", L"取消", WS_CHILD | WS_VISIBLE,
		250, 320, 50, 25, hservFrame, (HMENU)IDB_SERV_CANCEL, hInst, nullptr);
	// 字体
	static HFONT hfont = MYCREATEFRONT(28, 14, FW_BOLD);
	::SendMessage(htitle, WM_SETFONT, (WPARAM)hfont, NULL);
	// 字体2
	static HFONT hfont2 = MYCREATEFRONT(14, 7, FW_LIGHT);
	::SendMessage(hlab_name, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_name, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hlab_players, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_players, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hlab_port, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_port, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hlab_mc_ip, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_mc_ip, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hlab_mc_port, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_mc_port, WM_SETFONT, (WPARAM)hfont2, NULL);
	// 字体3 4
	static HFONT hfont3 = MYCREATEFRONT(16, 8, FW_BOLD);
	static HFONT hfont4 = MYCREATEFRONT(16, 8, FW_LIGHT);
	::SendMessage(hbtn_serv_ok, WM_SETFONT, (WPARAM)hfont3, NULL);
	::SendMessage(hbtn_serv_cancel, WM_SETFONT, (WPARAM)hfont4, NULL);
	// TODO: 设置默认值
	char buf[BUFSIZE] = { 0 };
	sprintf_s(buf, "%s", DEFAULT_SERV_PORT);
	USES_CONVERSION;
	WCHAR* p = A2W(buf);
	::SendMessage(htxt_port, WM_SETTEXT, NULL, (LPARAM)p);
	sprintf_s(buf, "%s", MC_DEFAULT_ADDR);
	p = A2W(buf);
	::SendMessage(htxt_mc_ip, WM_SETTEXT, NULL, (LPARAM)p);
	sprintf_s(buf, "%d", MC_DEFAULT_PORT);
	p = A2W(buf);
	::SendMessage(htxt_mc_port, WM_SETTEXT, NULL, (LPARAM)p);
}

void initClientFrame(HWND hWnd)
{
	int default_id = IDF_CLIENT_FRAME;
	hclientFrame = CreateWindowW(L"static", NULL, WS_CHILD | WS_BORDER /*| WS_VISIBLE*/,
		MY_WIN_WIDTH / 2 - 210, MY_WIN_HEIGHT / 2 - 230, 400, 400,
		hWnd, (HMENU)default_id++, hInst, nullptr);
	// 标题
	HWND htitle = CreateWindowW(L"static", TEXT("加入房间"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		100, 50, 200, 60, hclientFrame, (HMENU)default_id++, hInst, nullptr);

	// 多播地址
	HWND hlab_found_mc_ip = CreateWindowW(L"static",   TEXT("  多播IP:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		50, 110, 80, 25, hclientFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_found_mc_ip = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER,
		140, 110, 190, 25, hclientFrame, (HMENU)IDT_CLIE_MC_IP, hInst, NULL);
	// 多播端口
	HWND hlab_found_mc_port = CreateWindowW(L"static", TEXT("多播端口:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
		50, 140, 80, 25, hclientFrame, (HMENU)default_id++, hInst, nullptr);
	htxt_found_mc_port = CreateWindowW(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
		140, 140, 190, 25, hclientFrame, (HMENU)IDT_CLIE_MC_PORT, hInst, NULL);
	// 列表框
	hlist_serv_name = CreateWindowW(L"listbox", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL,
		80, 180, 240, 120, hclientFrame, (HMENU)IDCB_CLIE_BOX, hInst, nullptr);

	// 按钮
	HWND hbtn_clie_ok = CreateWindowW(L"button", L"确认", WS_CHILD | WS_VISIBLE,
		75, 320, 50, 30, hclientFrame, (HMENU)IDB_CLIE_OK, hInst, nullptr);
	HWND hbtn_clie_fresh = CreateWindowW(L"button", L"刷新", WS_CHILD | WS_VISIBLE,
		175, 320, 50, 30, hclientFrame, (HMENU)IDB_CLIE_FRESH, hInst, nullptr);
	HWND hbtn_clie_cancel = CreateWindowW(L"button", L"取消", WS_CHILD | WS_VISIBLE,
		275, 320, 50, 30, hclientFrame, (HMENU)IDB_CLIE_CANCEL, hInst, nullptr);
	// 字体
	static HFONT hfont = MYCREATEFRONT(28, 14, FW_BOLD);
	::SendMessage(htitle, WM_SETFONT, (WPARAM)hfont, NULL);
	// 字体2
	static HFONT hfont2 = MYCREATEFRONT(14, 7, FW_LIGHT);
	::SendMessage(hlab_found_mc_ip, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_found_mc_ip, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(hlab_found_mc_port, WM_SETFONT, (WPARAM)hfont2, NULL);
	::SendMessage(htxt_found_mc_port, WM_SETFONT, (WPARAM)hfont2, NULL);
	// 字体2
	static HFONT hfont3 = MYCREATEFRONT(18, 9, FW_LIGHT);
	::SendMessage(hlist_serv_name, WM_SETFONT, (WPARAM)hfont2, NULL);
	// 字体3 4
	static HFONT hfont4 = MYCREATEFRONT(16, 8, FW_BOLD);
	static HFONT hfont5 = MYCREATEFRONT(16, 8, FW_LIGHT);
	::SendMessage(hbtn_clie_ok, WM_SETFONT, (WPARAM)hfont4, NULL);
	::SendMessage(hbtn_clie_fresh, WM_SETFONT, (WPARAM)hfont5, NULL);
	::SendMessage(hbtn_clie_cancel, WM_SETFONT, (WPARAM)hfont5, NULL);
	// 设置默认值
	char buf[BUFSIZE] = { 0 };
	sprintf_s(buf, "%s", MC_DEFAULT_ADDR);
	USES_CONVERSION;
	WCHAR* p = A2W(buf);
	::SendMessage(htxt_found_mc_ip, WM_SETTEXT, NULL, (LPARAM)p);
	sprintf_s(buf, "%d", MC_DEFAULT_PORT);
	p = A2W(buf);
	::SendMessage(htxt_found_mc_port, WM_SETTEXT, NULL, (LPARAM)p);
}

void freshListBox(HWND hWnd)
{
	char mc_ip[IP_LENGTH] = { 0 };
	int mc_port = 0;
	WCHAR buf[BUFSIZE] = { 0 };
	// 获取多播IP
	int len = ::SendMessage(htxt_found_mc_ip, WM_GETTEXTLENGTH, NULL, NULL);
	::SendMessage(htxt_found_mc_ip, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	if (len <= 0 || len >= IP_LENGTH) {
		::MessageBox(hWnd, L"多播IP不符", L"超出范围", NULL);
		return;
	}
	wcharToArrayChar(mc_ip, buf);
	// TODO: 进一步检查多播 IP 是否正确

	// 获取多播端口
	len = ::SendMessage(htxt_found_mc_port, WM_GETTEXTLENGTH, NULL, NULL);
	::SendMessage(htxt_found_mc_port, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	mc_port = wcharToInt(buf);
	if (mc_port < 1024 || mc_port >= 65536) {
		::MessageBox(hWnd, L"端口限制：1024 ~ 65535", L"超出范围", NULL);
		return;
	}
	// TODO: 客户端查找服务器
	if (cp->findServer(mc_ip, mc_port, MAX_FOUND_SERVER, MAX_FOUND_TIME, fdservs) < 0) {
		::MessageBox(hWnd, L"FAILED", L"查找错误", NULL);
		return;
	}
	if (fdservs.found == 0) {
		::MessageBox(hWnd, L"没有找到房间", L"查找超时", NULL);
		return;
	}
	// TODO: 重设列表项
	::SendMessage(hlist_serv_name, LB_RESETCONTENT, NULL, NULL);
	char tmp[BUFSIZE] = { 0 };
	for (int i = 0; i < fdservs.found; ++i)
	{
		sprintf_s(tmp, "%s %s %d", fdservs.name[i], fdservs.ip[i], fdservs.port[i]);
		USES_CONVERSION;
		WCHAR* p = A2W(tmp);
		::SendMessage(hlist_serv_name, LB_ADDSTRING, NULL, (LPARAM)p);
	}
}

void GetServFrameMsg(HWND hWnd)
{
	char name[BUFSIZE] = { 0 };
	int players = 0;
	int port = 0;
	char mc_ip[IP_LENGTH] = { 0 };
	int mc_port = 0;
	WCHAR buf[256] = { 0 };
	// TODO: 获取编辑框文本
	// 获取房间名
	int len = ::SendMessage(htxt_name, WM_GETTEXTLENGTH, NULL, NULL);
	if (len <= 0) {
		::MessageBox(hWnd, L"输入房间名", L"提示", NULL);
		return;
	}
	::SendMessage(htxt_name, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	wcharToArrayChar(name, buf);
	// 获取人数
	len = ::SendMessage(htxt_players, WM_GETTEXTLENGTH, NULL, NULL);
	::SendMessage(htxt_players, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	players = wcharToInt(buf);
	if (players <= 0 || players > MY_MAX_PLAYERS) {
		::MessageBox(hWnd, L"人数限制：1 ~ 8", L"超出范围", NULL);
		return;
	}
	// 获取端口
	len = ::SendMessage(htxt_port, WM_GETTEXTLENGTH, NULL, NULL);
	::SendMessage(htxt_port, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	port = wcharToInt(buf);
	if (port < 1024 || port >= 65536) {
		::MessageBox(hWnd, L"端口限制：1024 ~ 65535", L"超出范围", NULL);
		return;
	}
	// 获取多播IP
	len = ::SendMessage(htxt_mc_ip, WM_GETTEXTLENGTH, NULL, NULL);
	::SendMessage(htxt_mc_ip, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	if (len <= 0 || len >= IP_LENGTH) {
		::MessageBox(hWnd, L"多播IP不符", L"超出范围", NULL);
		return;
	}
	wcharToArrayChar(mc_ip, buf);
	// TODO: 进一步检查多播 IP 是否正确

	// 获取多播端口
	len = ::SendMessage(htxt_mc_port, WM_GETTEXTLENGTH, NULL, NULL);
	::SendMessage(htxt_mc_port, WM_GETTEXT, (WPARAM)len + 1, (LPARAM)buf);
	mc_port = wcharToInt(buf);
	if (mc_port < 1024 || mc_port >= 65536) {
		::MessageBox(hWnd, L"端口限制：1024 ~ 65535", L"超出范围", NULL);
		return;
	}
#ifdef _DEBUG		// 显示各项信息
	char tmp[BUFSIZE] = { 0 };
	sprintf_s(tmp, "%s %d %d %s %d", name, players, port, mc_ip, mc_port);
	USES_CONVERSION;
	WCHAR* k = A2W(tmp);
	::MessageBox(hWnd, k, L"ALL PERPORTY", NULL);
#endif
	// TODO: 启动服务器



}

void GetClientFrameMsg(HWND hWnd)
{
	// 获取列表框中的选中索引， -1 为没选， 选中索引从 0 开始
	int index = ::SendMessage(hlist_serv_name, LB_GETCURSEL, NULL, NULL);
	if (index < 0) {
		::MessageBox(hWnd, L"先选择房间", L"提示", NULL);
		return;
	}
	// TODO: 启动客户端


}

void wcharToArrayChar(char* dst, const WCHAR* wch)
{
	USES_CONVERSION;
	char* p = W2A(wch);
	int len = strlen(p);
	::memcpy(dst, p, sizeof(char) * len);
	dst[len] = '\0';
}

int wcharToInt(const WCHAR* wch)
{
	int res = 0;
	char tmp[BUFSIZE] = { 0 };
	wcharToArrayChar(tmp, wch);
	for (int i = 0; tmp[i] >= '0' && tmp[i] <= '9'; ++i)
		res = res * 10 + (tmp[i] - '0');
	return res;
}
