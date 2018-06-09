#pragma once

#include "stdafx.h"

struct EXOVER {
	WSAOVERLAPPED m_over;
	char m_iobuf[MAX_BUFF_SIZE];
	WSABUF m_wsabuf;
	char event_type;
};


class Client {
public:
	SOCKET m_s;
	bool m_isconnected;
	int m_x;
	int m_y;
	bool m_isactive;
	WORD Id;

	unordered_set <int> m_viewlist;
	mutex m_mvl;

	EXOVER m_rxover;
	int m_packet_size;  // 지금 조립하고 있는 패킷의 크기
	int	m_prev_packet_size; // 지난번 recv에서 완성되지 않아서 저장해 놓은 패킷의 앞부분의 크기
	char m_packet[MAX_PACKET_SIZE];

	Client()
	{
		m_isconnected = false;
		m_x = 4;
		m_y = 4;

		ZeroMemory(&m_rxover.m_over, sizeof(WSAOVERLAPPED));
		m_rxover.m_wsabuf.buf = m_rxover.m_iobuf;
		m_rxover.m_wsabuf.len = sizeof(m_rxover.m_wsabuf.buf);
		m_rxover.event_type = false;
		m_prev_packet_size = 0;
	}
};

class CFramework
{
private:

public:
	void SendPacket(int id, void *ptr);
	void ProcessPacket(int id, char *ptr);

	bool CanSee(int a, int b);
	void DisconnectPlayer(int id);
public:
	void StartRecv(int id);
	
	void SendPutObjectPacket(int client, int object);
	void SendRemoveObjectPacket(int client, int object);
	
};