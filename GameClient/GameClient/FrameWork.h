#pragma once
#include "stdafx.h"

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <WS2tcpip.h>


#define SERVER_PORT 9000
#define BUF_SIZE	1024
#define	WM_SOCKET				WM_USER + 1



class FrameWork
{
	HINSTANCE main_hInstance;
	HWND main_hwnd;
	SOCKET g_mysocket;

	RECT **m_pBoard;

	WSABUF	send_wsabuf;
	char 	send_buffer[BUF_SIZE];
	WSABUF	recv_wsabuf;
	char	recv_buffer[BUF_SIZE];

	char	packet_buffer[BUF_SIZE];
	DWORD		in_packet_size = 0;
	int		saved_packet_size = 0;
	int		g_myid;

private:
	int speed = 10;
	int board_speed = 10;
	POINT pos;
	int dir;
	int id = 0;

	Client other[MAX_USER];

	////	////	////
public:
	FrameWork() {}
	FrameWork(HINSTANCE instance, HWND hwnd);
	~FrameWork();

	void Initialize();
	void Release();

	void Update();
	void Draw(HDC *memdc);

	//	서버처리
public:
	void ProcessPacket(char *);
	void ReadPacket(SOCKET sock);


	//	게임 로직
	void BackGroundProcess(RECT **board);
public:
	void KeyboardDownProcess(WPARAM wparam);
	void KeyboardUpProcess(WPARAM wparam) {}

};

