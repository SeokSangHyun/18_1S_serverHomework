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
	inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr.s_addr);

	int Result = WSAConnect(g_mysocket, (sockaddr *)&ServerAddr, sizeof(ServerAddr), NULL, NULL, NULL, NULL);

	WSAAsyncSelect(g_mysocket, main_hwnd, WM_SOCKET, FD_CLOSE | FD_READ);

	send_wsabuf.buf = send_buffer;
	send_wsabuf.len = BUF_SIZE;
	recv_wsabuf.buf = recv_buffer;
	recv_wsabuf.len = BUF_SIZE;

	//
	for (int i = 0; i < MAX_USER; ++i)
	{
		other[i].id = i;
		if (i == id) continue;

		other[i].is_use = false;
		other[i].pos.x = rand() % BOARD_WIDTH;
		other[i].pos.y = rand() % BOARD_HEIGHT;
	}
}
void FrameWork::Release()
{
	closesocket(g_mysocket);
	WSACleanup();
}

void FrameWork::Draw(HDC * memdc)
{
	HBRUSH CharBrush;
	HBRUSH myBrush;

	CharBrush = CreateSolidBrush(RGB(20, 20, 20));
	myBrush = (HBRUSH)SelectObject(*memdc, CharBrush);
	

	for (int i = 0; i < MAX_USER; ++i) {
		if(other[i].is_use == false)
			Ellipse(*memdc, other[i].pos.x-OTHER_SIZE, other[i].pos.y- OTHER_SIZE,
				other[i].pos.x+ OTHER_SIZE, other[i].pos.y+ OTHER_SIZE);
	}
	Ellipse(*memdc, pos.x+MY_SIZE, pos.y+ MY_SIZE, pos.x- MY_SIZE, pos.y- MY_SIZE);


	SelectObject(*memdc, myBrush);
	DeleteObject(CharBrush);

	//for (int i = 0; i < g_divide; ++i)
	//		bit_character.TransparentBlt(MemDC, g_ppacket[i].data.x, g_ppacket[i].data.y,
	//			g_ppacket[i].data.size_x, g_ppacket[i].data.size_y, RGB(0, 0, 0));
}

//
/*		////////////////////////////////////////////////////////		*/	
/*		////////////////////////////////////////////////////////		*/	
//


void ProcessPacket(char *ptr)
{
	//static bool first_time = true;
	//switch (ptr[1])
	//{
	//case SC_PUT_PLAYER:
	//{
	//	sc_packet_put_player *my_packet = reinterpret_cast<sc_packet_put_player *>(ptr);
	//	int id = my_packet->id;
	//	if (first_time) {
	//		first_time = false;
	//		g_myid = id;
	//	}
	//	if (id == g_myid) {
	//		player.x = my_packet->x;
	//		player.y = my_packet->y;
	//		player.attr |= BOB_ATTR_VISIBLE;
	//	}
	//	else if (id < NPC_START) {
	//		skelaton[id].x = my_packet->x;
	//		skelaton[id].y = my_packet->y;
	//		skelaton[id].attr |= BOB_ATTR_VISIBLE;
	//	}
	//	else {
	//		npc[id - NPC_START].x = my_packet->x;
	//		npc[id - NPC_START].y = my_packet->y;
	//		npc[id - NPC_START].attr |= BOB_ATTR_VISIBLE;
	//	}
	//	break;
	//}
	//case SC_POS:
	//{
	//	sc_packet_pos *my_packet = reinterpret_cast<sc_packet_pos *>(ptr);
	//	int other_id = my_packet->id;
	//	if (other_id == g_myid) {
	//		g_left_x = my_packet->x - 8;
	//		g_top_y = my_packet->y - 8;
	//		player.x = my_packet->x;
	//		player.y = my_packet->y;
	//	}
	//	else if (other_id < NPC_START) {
	//		skelaton[other_id].x = my_packet->x;
	//		skelaton[other_id].y = my_packet->y;
	//	}
	//	else {
	//		npc[other_id - NPC_START].x = my_packet->x;
	//		npc[other_id - NPC_START].y = my_packet->y;
	//	}
	//	break;
	//}
	//
	//case SC_REMOVE_PLAYER:
	//{
	//	sc_packet_remove_player *my_packet = reinterpret_cast<sc_packet_remove_player *>(ptr);
	//	int other_id = my_packet->id;
	//	if (other_id == g_myid) {
	//		player.attr &= ~BOB_ATTR_VISIBLE;
	//	}
	//	else if (other_id < NPC_START) {
	//		skelaton[other_id].attr &= ~BOB_ATTR_VISIBLE;
	//	}
	//	else {
	//		npc[other_id - NPC_START].attr &= ~BOB_ATTR_VISIBLE;
	//	}
	//	break;
	//}
	//default:
	//	printf("Unknown PACKET type [%d]\n", ptr[1]);
	//}
}
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





void FrameWork::BackGroundProcess(RECT ** board)
{
	float minX, maxX;
	float minY, maxY;
	minX = WIN_WIDTH * 0.5f;		maxX = BOARD_WIDTH - WIN_WIDTH * 0.5f;
	minY = WIN_HEIGHT * 0.5f;		maxY = BOARD_HEIGHT - WIN_HEIGHT * 0.5f;

	switch (dir)
	{
	case CharDir::right:
		if (minX < pos.x) {
			if (board[0][BOARD_COUNT-1].right > WIN_WIDTH)
			{
				speed = 0;
				board_speed = 10;
				for (int i = 0; i < BOARD_COUNT; ++i) {
					for (int j = 0; j < BOARD_COUNT; ++j) {
						board[i][j].left -= board_speed;
						board[i][j].right -= board_speed;
					}
				}
				for (int i = 0; i < MAX_USER; ++i)
					if (other[i].is_use == false)
						other[i].pos.x -= board_speed;
			}
			else {
				speed = 10;
				board_speed = 0;
			}
		}
		else {
			speed = 10;
			board_speed = 0;
		}
		break;
	case CharDir::left:
		if (minX > pos.x) {
			if (board[0][0].left < 0)
			{
				speed = 0;
				board_speed = 10;
				for (int i = 0; i < BOARD_COUNT; ++i) {
					for (int j = 0; j < BOARD_COUNT; ++j) {
						board[i][j].left += board_speed;
						board[i][j].right += board_speed;
					}
				}
				for (int i = 0; i < MAX_USER; ++i)
					if (other[i].is_use == false)
						other[i].pos.x += board_speed;
			}
			else {
				speed = 10;
				board_speed = 0;
			}
		}
		else {
			speed = 10;
			board_speed = 0;
		}
		break;
	case CharDir::up:
		if (minY > pos.y) {
			if (board[0][0].top < 0)
			{
				speed = 0;
				board_speed = 10;
				for (int i = 0; i < BOARD_COUNT; ++i) {
					for (int j = 0; j < BOARD_COUNT; ++j) {
						board[i][j].top += board_speed;
						board[i][j].bottom += board_speed;
					}
				}
				for (int i = 0; i < MAX_USER; ++i)
					if (other[i].is_use == false)
						other[i].pos.y += board_speed;
			}
			else {
				speed = 10;
				board_speed = 0;
			}
		}
		else {
			speed = 10;
			board_speed = 0;
		}
		break;
	case CharDir::down:
		if (minY < pos.y) {
			if (board[BOARD_COUNT - 1][0].bottom > WIN_HEIGHT)
			{
				speed = 0;
				board_speed = 10;
				for (int i = 0; i < BOARD_COUNT; ++i) {
					for (int j = 0; j < BOARD_COUNT; ++j) {
						board[i][j].top -= board_speed;
						board[i][j].bottom -= board_speed;
					}
				}
				for (int i = 0; i < MAX_USER; ++i)
					if (other[i].is_use == false)
						other[i].pos.y -= board_speed;
			}
			else {
				speed = 10;
				board_speed = 0;
			}
		}
		else {
			speed = 10;
			board_speed = 0;
		}
		break;
	default:
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
void FrameWork::KeyboardDownProcess(WPARAM wparam)
{
	switch (wparam)
	{
	case VK_RIGHT:
		if(pos.x < WIN_WIDTH) pos.x+=speed;
		dir = CharDir::right;
		break;
	case VK_LEFT:
		if (pos.x > 0)pos.x-=speed;
		dir = CharDir::left;
		break;
	case VK_UP:
		if (pos.y > 0)pos.y-= speed;
		dir = CharDir::up;
		break;
	case VK_DOWN:
		if (pos.y < WIN_HEIGHT)pos.y+= speed;
		dir = CharDir::down;
		break;
	default:
		dir = CharDir::stay;
		break;
	}
}




