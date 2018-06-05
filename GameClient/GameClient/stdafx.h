// stdafx.h : 자주 사용하지만 자주 변경되지는 않는
// 표준 시스템 포함 파일 또는 프로젝트 관련 포함 파일이
// 들어 있는 포함 파일입니다.
//

#pragma once

#pragma comment(lib, "ws2_32")
#include "targetver.h"
#include <winsock2.h>
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
// Windows 헤더 파일:
#include <windows.h>
#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include "..\..\protocol.h"

// C 런타임 헤더 파일입니다.
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <random>
#include <time.h>
#include <atlimage.h>


// TODO: 프로그램에 필요한 추가 헤더는 여기에서 참조합니다.

#define WIN_WIDTH 500
#define WIN_HEIGHT 500

enum DataType { notLink, Link };
enum CharDir { stay = 10, left, right, up, down };
struct Packet
{
	float x;
	float y;

	short size_x;
	short size_y;

	char room[2];
	char dir;
};

#pragma pack(push, 1)
struct Data
{
	float x;
	float y;

	short size_x;
	short size_y;

	char room[2];
	char dir;
};
struct Packet_t
{
	char size;
	char type;
	Data data;
};
#pragma pack(pop)



// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
static void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
static int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, buf, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}
