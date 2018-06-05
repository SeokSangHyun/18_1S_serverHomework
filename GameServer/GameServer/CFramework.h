#pragma once

#include "stdafx.h"



//클라이언트( cl )에 packet의 정보를 보냄.
static void SendPacket(int cl, void *packet) {
	ExOver *o = new ExOver;
	char *p = reinterpret_cast<char *>(packet);
	memcpy(o->io_buffer, packet, p[0]);
	o->is_recv = false;
	o->wsabuf.buf = o->io_buffer;
	o->wsabuf.len = p[0];
	ZeroMemory(&o->wsaover, sizeof(WSAOVERLAPPED));

	int ret = WSASend(g_clients[cl].s, &o->wsabuf, 1, NULL, 0, &o->wsaover, NULL);

	if (0 != ret)
	{
		int err_num = WSAGetLastError();
		if (WSA_IO_PENDING != err_num)
			err_display("Error in SendPacket : ");
	}
}

static void DisCountPlayer(int key)
{
	closesocket(g_clients[key].s);
	g_clients[key].in_use = false;
	std::cout << "client [" << key << "] DisCount\n";

	sc_packet_remove_player p;
	p.id = key;
	p.size = sizeof(p);
	p.type = SC_REMOVE_PLAYER;

	for (auto id : g_clients[key].viewlist) {
		if (g_clients[id].in_use == true) {
			if (g_clients[id].viewlist.count(key)) {
				g_clients[id].viewlist.erase(key);
				SendPacket(id, &p);
			}
		}
	}
	g_clients[key].viewlist.clear();
}


namespace Util
{
	void LindedSetData(char serial_num, SOCKET &sock);
	void ProcessPacket(char serial_num, char *data);
	void KeyInput(char serial_num, char *packet);
}