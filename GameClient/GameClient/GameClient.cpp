// GameClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "stdafx.h"
#include "GameClient.h"


#define MAX_LOADSTRING 100
#define SERVERPORT 9000

using namespace std;

int len;
char serverIp[30];
RECT **ChessBoard;
HBITMAP g_BackBitMap;
Packet g_packet;

char g_divide;
char max_user = 0;
char g_serialNum = 0;
Packet_t g_ppacket[20];
SOCKET sock;
SOCKADDR_IN serveraddr;
Packet_t g_data;


// 시간 관련 전역 변수
int					iFPS = 0;
DWORD				g_dwPrevTime = 0;
float				g_fTime_Loop = 0.f;
float				g_fTime_FPS = 0.f;
int					g_iTime = 0;
float				g_fDeleteTime;


// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
HWND g_hWnd;

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
#ifdef _DEBUG
	//#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")	//멀티바이트
	#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")	//유니코드
#endif
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: 여기에 코드를 입력합니다.
	ZeroMemory(g_ppacket, sizeof(Packet_t)*20);
	ZeroMemory(serverIp, sizeof(serverIp));
	
	// 전역 문자열을 초기화합니다.
	cout << "점속 할 IP Address를 적으시오. : ";
	cin >> serverIp;
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GAMECLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	//
	// 윈속 초기화
	int retval;
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)		return 1;

	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance(hInstance, nCmdShow))
	{
		//초기화
		return FALSE;
	}
	//


	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMECLIENT));

	MSG msg;
	CreateThread(NULL, 0, MainThread, NULL, 0, NULL);

	// 기본 메시지 루프입니다.
	while (1)
	{
		DWORD dwCurrTime = timeGetTime();
		DWORD dwElapsedTime = dwCurrTime - g_dwPrevTime;
		float fElapsedTimeInSecond = dwElapsedTime / 1000.f;
		g_dwPrevTime = dwCurrTime;

		g_fTime_Loop += fElapsedTimeInSecond;
		g_fTime_FPS += fElapsedTimeInSecond;
		g_fDeleteTime = fElapsedTimeInSecond;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else if (msg.message == WM_QUIT)
			{
				exit(1);
			}

		}

		if (g_fTime_FPS >= 1.f)
		{
			iFPS = 0;
			g_fTime_FPS = 0.f;
		}
	}

	FreeConsole();
	closesocket(sock);
	WSACleanup();
	return (int)msg.wParam;
}


// 정보 대화 상자의 메시지 처리기입니다.
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

//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PAINTSTRUCT ps;
	static HDC hdc, MemDC, BackDC;
	static HBITMAP hPreBit;
	static CImage bit_character;

	switch (message)
	{
	case WM_CREATE:
		SetTimer(hWnd, 1, 100, NULL);
		bit_character.Load(L"pone.bmp");

		break;
	case WM_TIMER:
		switch (wParam)
		{
		case 1:
		InvalidateRect(hWnd, NULL, false);
		break;
		}
		break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 메뉴 선택을 구문 분석합니다.
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			//exit(1);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_LBUTTONDOWN:
		//mouse.x = LOWORD(lParam);
		//mouse.y = HIWORD(lParam);
		break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_RIGHT:
			if (g_ppacket[g_serialNum-1].type)
				g_data.data.dir = CharDir::right;
			break;
		case VK_LEFT:
			if (g_ppacket[g_serialNum-1].type)
				g_data.data.dir = CharDir::left;
			break;
		case VK_UP:
			if (g_ppacket[g_serialNum-1].type)
				g_data.data.dir = CharDir::up;
			break;
		case VK_DOWN:
			if (g_ppacket[g_serialNum-1].type)
				g_data.data.dir = CharDir::down;
			break;
		}
		InvalidateRect(hWnd, NULL, false);
		break;

	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		MemDC = CreateCompatibleDC(hdc);
		g_BackBitMap = CreateCompatibleBitmap(hdc, WIN_WIDTH, WIN_HEIGHT);
		hPreBit = (HBITMAP)SelectObject(MemDC, g_BackBitMap);

		//1. 판
		PrintBoard(&MemDC, g_divide, ChessBoard);
		//캐릭터
		for(int i = 0 ; i < g_divide ; ++i)
			if(g_ppacket[i].type)
				bit_character.TransparentBlt(MemDC, g_ppacket[i].data.x, g_ppacket[i].data.y,
					g_ppacket[i].data.size_x, g_ppacket[i].data.size_y, RGB(0, 0, 0));

		//
		BitBlt(hdc, 0,0,WIN_WIDTH, WIN_HEIGHT, MemDC, 0,0, SRCCOPY);

		DeleteObject(g_BackBitMap);
		DeleteDC(MemDC);
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		KillTimer(hWnd, 0);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
