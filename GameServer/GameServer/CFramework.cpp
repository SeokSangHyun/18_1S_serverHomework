#include "CFramework.h"

CLIENT g_clients[MAX_USER];


void Util::LindedSetData(char num, SOCKET &sock)
{
	/*
	RECT rect;
	serialNum = num;
	send(sock, (char*)&serialNum, sizeof(char), 0);
	recvn(sock, (char*)&rect, sizeof(RECT), 0);

	m_clientPacket[serialNum - 1].size = sizeof(Data);
	m_clientPacket[serialNum - 1].type = num;
	m_clientPacket[serialNum - 1].data.size_x = (rect.right - rect.left) / DIVIDE;
	m_clientPacket[serialNum - 1].data.size_y = (rect.bottom - rect.top) / DIVIDE;
	m_clientPacket[serialNum - 1].data.dir = CharDir::stay;

	POINT temp;
	temp = { rand() % DIVIDE, rand() % DIVIDE };
	for (int i = 0; i < DIVIDE; ++i)
	{
		for (int j = 0; j < DIVIDE; ++j)
		{
			if (temp.x == j && temp.y == i)
			{
				m_clientPacket[serialNum - 1].data.x = j * m_clientPacket[serialNum - 1].data.size_x;
				m_clientPacket[serialNum - 1].data.y = i * m_clientPacket[serialNum - 1].data.size_y;
							   
				m_clientPacket[serialNum - 1].data.room[0] = j;
				m_clientPacket[serialNum - 1].data.room[1] = i;

			}
		}
	}

	char ntemp = DIVIDE;
	send(sock, (char*)&ntemp, sizeof(char), 0);
	ntemp = MAX_USER;
	send(sock, (char*)&ntemp, sizeof(char), 0);

	send(sock, (char*)&m_clientPacket, sizeof(Packet_t)*MAX_USER, 0);
	*/
}

void Util::ProcessPacket(char serial_num, char *packet)
{
	KeyInput(serial_num, packet);
}

void Util::KeyInput(char serial_num, char *packet)
{
	int x = g_clients[serial_num].x;
	int y = g_clients[serial_num].y;
	switch (packet[1])
	{
	case CS_UP: if (y > 0) y--; break;
	case CS_DOWN: if (y < BOARD_HEIGHT - 1) y++; break;
	case CS_LEFT: if (x > 0) x--; break;
	case CS_RIGHT: if (x < BOARD_WIDTH - 1) x++; break;
	default: 
		std::cout << "Unkown Packet Type from Client [" << serial_num << "]\n";
		return;
	}
	g_clients[serial_num].x = x;
	g_clients[serial_num].y = y;

	sc_packet_pos sp;
	sp.id = serial_num;
	sp.size = sizeof(sc_packet_pos);
	sp.type = SC_POS;
	sp.x = g_clients[serial_num].x;
	sp.y = g_clients[serial_num].y;

	/*
	unordered_set<int> new_view_list;
	for (int i = 0; i < MAX_USER; ++i) {
		if (i == cl) continue;
		if (g_client[i].in_use == false) continue;
		if (CanSee(cl, i) == false) continue;
		new_view_list.insert(i);
	}
	for (auto id : new_view_list) {
		//새로 ViewList에 들어오는 객체 처리
		if (0 == g_client[cl].viewlist.count(id)) {
			SendPutObject(cl, id);
			g_client[cl].viewlist.insert(id);
			if (0 == g_client[id].viewlist.count(cl)) {
				SendPutObject(id, cl);
				g_client[id].viewlist.insert(cl);
			}
			else {
				SendPacket(id, &sp);
			}
		}
		//ViewList에 계속 남아있는 객체 처리
		//id 가 이미 list에 있음
		else {
			if (g_client[id].viewlist.count(cl)) {
				g_client[id].viewlist.insert(cl);
				SendPutObject(id, cl);
			}
			else {
				SendPacket(id, &sp);
			}
		}
	}

	//ViewList에서 나가는 객체 처리
	for (auto id : g_client[cl].viewlist) {
		if (0 != new_view_list.count(id)) {
			g_client[cl].viewlist.erase(id);
			SendRemoveObject(cl, id);

			if (0 != g_client[id].viewlist.count(cl)) {
				g_client[id].viewlist.erase(cl);
				SendRemoveObject(id, cl);
			}
		}
	}
	*/

	for (int i = 0; i < MAX_USER; ++i)
		if (g_clients[i].in_use)
			SendPacket(i, &sp);
}
