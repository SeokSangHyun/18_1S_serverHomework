#include "FrameWork.h"



FrameWork::FrameWork(HINSTANCE instance, HWND hwnd)
{
	main_hInstance = instance;
	main_hwnd = hwnd;
}
FrameWork::~FrameWork()
{
}


void FrameWork::Initialize()
{

	int retval;
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)		return;

	g_mysocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(SERVER_PORT);
	ServerAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int Result = WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	WSAAsyncSelect(g_mysocket, main_hwnd, WM_SOCKET, FD_CLOSE | FD_READ);

	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = BUF_SIZE;
}
void FrameWork::Release()
{
	closesocket(g_mysocket);
	WSACleanup();
}

void FrameWork::Draw(HDC * memdc)
{
}

//
/*		////////////////////////////////////////////////////////		*/	
/*		////////////////////////////////////////////////////////		*/	
//


void FrameWork::ReadPacket(SOCKET sock)
{
	DWORD iobyte, ioflag = 0;
	int ret = WSARecv(sock, &recv_wsabuf, 1, &iobyte, &ioflag, NULL, NULL);
	if (ret) {
		int err_code = WSAGetLastError();
		printf("Recv Error [%d]\n", err_code);
	}

	BYTE *ptr = reinterpret_cast<BYTE *>(recv_buffer);
	while (0 != iobyte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (iobyte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);

			ProcessPacket(packet_buffer);

			ptr += in_packet_size - saved_packet_size;
			iobyte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, iobyte);
			saved_packet_size += iobyte;
			iobyte = 0;
		}
	}
}

void FrameWork::KeyboardProcess(WPARAM wparam)
{
	switch (wparam)
	{
	case VK_RIGHT:
		if (g_ppacket[g_serialNum - 1].type)
			g_data.data.dir = CharDir::right;
		break;
	case VK_LEFT:
		if (g_ppacket[g_serialNum - 1].type)
			g_data.data.dir = CharDir::left;
		break;
	case VK_UP:
		if (g_ppacket[g_serialNum - 1].type)
			g_data.data.dir = CharDir::up;
		break;
	case VK_DOWN:
		if (g_ppacket[g_serialNum - 1].type)
			g_data.data.dir = CharDir::down;
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////








