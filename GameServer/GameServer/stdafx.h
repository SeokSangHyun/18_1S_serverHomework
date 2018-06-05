#pragma once

#include <iostream>
#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <thread>

#include <Mmsystem.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "..\..\protocol.h"
#include "targetver.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32")

#define BUFSIZE    512
#define DIVIDE 50
#define BOARD_WIDTH   800
#define BOARD_HEIGHT  500


enum DataType {notLink, Link};
enum CharDir {stay = 10, left, right, up, down };

//
struct ExOver {
	WSAOVERLAPPED wsaover;
	bool is_recv;
	WSABUF wsabuf;
	char io_buffer[MAX_BUFF_SIZE];
};
struct CLIENT {
	SOCKET s;
	bool in_use;
	char x, y;

	ExOver exover;
	int packet_size;
	int prev_size;//지금까지 담은 패킷크기.
	char prev_packet[MAX_PACKET_SIZE];

	//
	std::unordered_set<int> viewlist;
};
extern CLIENT g_clients[MAX_USER];


//////////

static void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}
static void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", (LPCWSTR)msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
