// GameClient.cpp: 응용 프로그램의 진입점을 정의합니다.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "FrameWork.h"
#include "GameClient.h"

#define MAX_LOADSTRING 100
using namespace std;


FrameWork *g_framework;
HINSTANCE hInst;                                // 현재 인스턴스입니다.
HWND g_hWnd;
//	//	//
HDC g_hdc, MemDC;


RECT **ChessBoard;
HBITMAP g_BackBitMap, hPreBit;
Packet g_packet;



// 전역 변수:
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.




ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMECLIENT));
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMECLIENT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GAMECLIENT);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);

}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, WIN_WIDTH, WIN_HEIGHT, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	g_hWnd = hWnd;

	ChessBoard = new RECT*[BOARD_COUNT];
	for (int i = 0; i < BOARD_COUNT; ++i)
		ChessBoard[i] = new RECT[BOARD_COUNT];

	g_framework = new FrameWork(hInstance, g_hWnd);
	g_framework->Initialize();

	return TRUE;
}





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
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_GAMECLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);


	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance(hInstance, nCmdShow))
	{
		//초기화
		return FALSE;
	}
	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMECLIENT));
	MSG msg;
	BuildSettting(ChessBoard);



	// 기본 메시지 루프입니다.
	while (1)
	{

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
		Render();
	}
	//////


	g_framework->Release();
	delete g_framework;
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PAINTSTRUCT ps;
	switch (message)
	{
	case WM_CREATE:

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
			exit(1);
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
		
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			closesocket((SOCKET)wParam);
			exit(-1);
			break;
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_READ:
			//ReadPacket((SOCKET)wParam);
			break;
		case FD_CLOSE:
			closesocket((SOCKET)wParam);
			exit(-1);
			break;
		}
	}
		break;
	case WM_KEYDOWN:
		g_framework->KeyboardDownProcess(wParam);
		g_framework->BackGroundProcess(ChessBoard);
		break;
	case WM_KEYUP:
		g_framework->KeyboardUpProcess(wParam);

		break;
	case WM_PAINT:
	{
		BeginPaint(hWnd, &ps);
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

void Render()
{
	g_hdc = GetDC(g_hWnd);
	MemDC = CreateCompatibleDC(g_hdc);
	g_BackBitMap = CreateCompatibleBitmap(g_hdc, WIN_WIDTH, WIN_HEIGHT);
	hPreBit = (HBITMAP)SelectObject(MemDC, g_BackBitMap);

	//1. 판
	PrintBoard(&MemDC, ChessBoard);
	//
	g_framework->Draw(&MemDC);


	BitBlt(g_hdc, 0, 0, WIN_WIDTH, WIN_HEIGHT, MemDC, 0, 0, SRCCOPY);

	DeleteObject(g_BackBitMap);
	DeleteDC(MemDC);
	ReleaseDC(g_hWnd, g_hdc);

}

