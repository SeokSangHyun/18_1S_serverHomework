#include "CFramework.h"

using namespace std;

extern std::array <Client, MAX_USER> g_clients;



void CFramework::SendPacket(int id, void *ptr)
{
	unsigned char *packet = reinterpret_cast<unsigned char *>(ptr);
	EXOVER *s_over = new EXOVER;
	memcpy(s_over->m_iobuf, packet, packet[0]);
	s_over->event_type = true;
	s_over->m_wsabuf.buf = s_over->m_iobuf;
	s_over->m_wsabuf.len = s_over->m_iobuf[0];
	ZeroMemory(&s_over->m_over, sizeof(WSAOVERLAPPED));
	int res = WSASend(g_clients[id].m_s, &s_over->m_wsabuf, 1, NULL, 0,
		&s_over->m_over, NULL);
	if (0 != res) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) error_display("Send Error! ", err_no);
	}
}

void CFramework::ProcessPacket(int id, char *packet)
{
	int x = g_clients[id].m_x;
	int y = g_clients[id].m_y;
	wstring str = { L"EXEC Get_Id" };
	wstring temp;

	switch (packet[1])
	{
	/*case CS_LOGIN:
		temp = L" " + to_wstring(packet[2]);
		str += temp;
		DataBaseSQL(str.data(), &x, &y);
		g_clients[id].Id = packet[2];
		break;*/
	default:
		cout << "Unkown Packet Type from Client [" << id << "]\n";
		return;
	}
	g_clients[id].m_x = x;
	g_clients[id].m_y = y;

	sc_packet_pos pos_packet;

	pos_packet.id = id;
	pos_packet.size = sizeof(sc_packet_pos);
	pos_packet.type = SC_POS;
	pos_packet.x = x;
	pos_packet.y = y;

	unordered_set<int> new_vl;
	for (int i = 0; i < MAX_USER; ++i) {
		if (i == id) continue;
		if (false == g_clients[i].m_isconnected) continue;
		if (true == CanSee(id, i)) new_vl.insert(i);
	}

	SendPacket(id, &pos_packet);

	// new_vl에는 있는데 old_vl에 없는 경우
	for (auto ob : new_vl) {
		g_clients[id].m_mvl.lock();
		if (0 == g_clients[id].m_viewlist.count(ob)) {
			g_clients[id].m_viewlist.insert(ob);
			g_clients[id].m_mvl.unlock();
			SendPutObjectPacket(id, ob);


			g_clients[ob].m_mvl.lock();
			if (0 == g_clients[ob].m_viewlist.count(id)) {
				g_clients[ob].m_viewlist.insert(id);
				g_clients[ob].m_mvl.unlock();
				SendPutObjectPacket(ob, id);
			}
			else {
				g_clients[ob].m_mvl.unlock();
				SendPacket(ob, &pos_packet);
			}
		}
		else {
			g_clients[ob].m_mvl.lock();
			if (0 != g_clients[ob].m_viewlist.count(id)) {
				g_clients[ob].m_mvl.unlock();
				SendPacket(ob, &pos_packet);
			}
			else {
				g_clients[ob].m_viewlist.insert(id);
				g_clients[ob].m_mvl.unlock();
				SendPutObjectPacket(ob, id);
			}
		}

	}

	// new_vl에는 없는데 old_vl에 있는 경우
	vector <int> to_remove;
	g_clients[id].m_mvl.lock();
	unordered_set<int> vl_copy = g_clients[id].m_viewlist;
	g_clients[id].m_mvl.unlock();
	for (auto ob : vl_copy) {
		if (0 == new_vl.count(ob)) {
			to_remove.push_back(ob);

			g_clients[ob].m_mvl.lock();
			if (0 != g_clients[ob].m_viewlist.count(id)) {
				g_clients[ob].m_viewlist.erase(id);
				g_clients[ob].m_mvl.unlock();
				SendRemoveObjectPacket(ob, id);
			}
			else {
				g_clients[ob].m_mvl.unlock();
			}
		}
	}

	g_clients[id].m_mvl.lock();
	for (auto ob : to_remove) g_clients[id].m_viewlist.erase(ob);
	g_clients[id].m_mvl.unlock();
	for (auto ob : to_remove) {
		SendRemoveObjectPacket(id, ob);
	}
}




void CFramework::DisconnectPlayer(int id)
{
	wstring str = { L"EXEC Set_Id" };
	wstring temp;
	temp = L" " + to_wstring(g_clients[id].Id);
	str += temp;
	temp = L", " + to_wstring(g_clients[id].m_x);
	str += temp;
	temp = L", " + to_wstring(g_clients[id].m_y);
	str += temp;
	//DataBaseSQL(str.data(), &g_clients[id].m_x, &g_clients[id].m_y);

	sc_packet_remove_player p;
	p.id = id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_PLAYER;
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == g_clients[i].m_isconnected) continue;
		if (i == id) continue;

		g_clients[i].m_mvl.lock();
		if (0 != g_clients[i].m_viewlist.count(id)) {
			g_clients[i].m_viewlist.erase(id);
			g_clients[i].m_mvl.unlock();
			SendPacket(i, &p);
		}
		else {
			g_clients[i].m_mvl.unlock();
		}
	}
	closesocket(g_clients[id].m_s);
	g_clients[id].m_mvl.lock();
	g_clients[id].m_viewlist.clear();
	g_clients[id].m_mvl.unlock();
	g_clients[id].m_isconnected = false;
}

bool CFramework::CanSee(int a, int b)
{
	int dist_x = g_clients[a].m_x - g_clients[b].m_x;
	int dist_y = g_clients[a].m_y - g_clients[b].m_y;
	int dist = dist_x * dist_x + dist_y * dist_y;
	return (VIEW_RADIUS * VIEW_RADIUS >= dist);
}




void CFramework::StartRecv(int id)
{
	unsigned long r_flag = 0;
	ZeroMemory(&g_clients[id].m_rxover.m_over, sizeof(WSAOVERLAPPED));
	int ret = WSARecv(g_clients[id].m_s, &g_clients[id].m_rxover.m_wsabuf, 1,
		NULL, &r_flag, &g_clients[id].m_rxover.m_over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no) error_display("Recv Error", err_no);
	}
}

void CFramework::SendPutObjectPacket(int client, int object)
{
	sc_packet_put_player p;
	p.id = object;
	p.size = sizeof(p);
	p.type = SC_PUT_PLAYER;
	p.x = g_clients[object].m_x;
	p.y = g_clients[object].m_y;
	SendPacket(client, &p);
}

void CFramework::SendRemoveObjectPacket(int client, int object)
{
	sc_packet_remove_player p;
	p.id = object;
	p.size = sizeof(p);
	p.type = SC_REMOVE_PLAYER;
	SendPacket(client, &p);
}




