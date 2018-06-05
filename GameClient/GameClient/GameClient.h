#pragma once

#include "resource.h"
#include "stdafx.h"


//
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
//
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
	return TRUE;
}



void BuildSettting(RECT rc, int div, RECT **board)
{
	float div_x = (rc.right - rc.left) / div;
	float div_y = (rc.bottom - rc.top) / div;

	POINT temp;
	temp = { rand() % div, rand() % div };
	for (int i = 0; i < div; ++i)
	{
		for (int j = 0; j < div; ++j)
		{
			board[i][j].left = j * div_x;
			board[i][j].right = j * div_x + div_x;

			board[i][j].top = i * div_y;
			board[i][j].bottom = i * div_y + div_y;
		}
	}
}



/*
*/
void PrintBoard(HDC *memdc, int div, RECT **board)
{
	HBRUSH whiteBrush, blackBrush;
	HBRUSH myBrush;

	//가로
	blackBrush = CreateSolidBrush(RGB(0, 0, 0));
	whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
	for (int i = 0; i < div; ++i)
	{
		for (int j = 0; j < div; ++j)
		{
			if ((i + j) % 2 ? true : false) myBrush = (HBRUSH)SelectObject(*memdc, whiteBrush);
			else							myBrush = (HBRUSH)SelectObject(*memdc, blackBrush);

			Rectangle(*memdc, board[i][j].left, board[i][j].top, board[i][j].right, board[i][j].bottom);
			//MoveToEx(*hdc, board[i][j].left, board[i][j].top, NULL);
			//LineTo(*hdc, board[i][j].right, board[i][j].top);
			SelectObject(*memdc, myBrush);
		}
	}
	DeleteObject(whiteBrush);
	DeleteObject(blackBrush);
}




DWORD WINAPI MainThread(LPVOID arg)
{
	int retval;
	// socket()
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit((char*)"socket()");

	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(serverIp);
	serveraddr.sin_port = htons(SERVERPORT);

	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit((char*)"connect()");
	//
	RECT rc; // 실제 윈도우 크기 , 이미지 크기
	GetClientRect(g_hWnd, &rc);

	recvn(sock, (char*)&g_serialNum, sizeof(char), 0);
	g_data.type = g_serialNum;
	if (!g_serialNum) {
		closesocket(sock);
		return 1;
	}
	send(sock, (char*)&rc, sizeof(RECT), 0);


	recvn(sock, (char*)&g_divide, sizeof(char), 0);
	recvn(sock, (char*)&max_user, sizeof(char), 0);
	recvn(sock, (char*)&g_ppacket, sizeof(Packet_t)*max_user, 0);
	ChessBoard = new RECT*[g_divide];
	for (int i = 0; i < g_divide; ++i)
		ChessBoard[i] = new RECT[g_divide];
	BuildSettting(rc, g_divide, ChessBoard);

	// 서버와 데이터 통신
	while (1)
	{
		if (g_fTime_Loop >= 1.f / 1000)
		{
			retval = send(sock, (char*)&g_data.type, sizeof(char), 0);
			retval = send(sock, (char*)&g_data.data, sizeof(Data), 0);
			retval = recvn(sock, (char*)&g_ppacket, sizeof(Packet_t)*max_user, 0);

			g_data.data.dir = g_ppacket[g_serialNum - 1].data.dir;
			g_data.data.room[0] = g_ppacket[g_serialNum - 1].data.room[0];
			g_data.data.room[1] = g_ppacket[g_serialNum - 1].data.room[1];
			g_data.data.size_x = g_ppacket[g_serialNum - 1].data.size_x;
			g_data.data.size_y = g_ppacket[g_serialNum - 1].data.size_y;
			g_data.data.x = g_ppacket[g_serialNum - 1].data.x;
			g_data.data.y = g_ppacket[g_serialNum - 1].data.y;

			if (g_data.type == DataType::notLink || retval == -1)
				break;

			++iFPS;
			g_fTime_Loop = 0.f;
		}
	}

	closesocket(sock);
}

