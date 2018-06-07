#pragma once

#include "CFramework.h"


/*   ==========   */
void ThreadWorker()
{
	int retval = 1;
	while (1)
	{
		unsigned long data_size;
		unsigned long long key;
		WSAOVERLAPPED *p_over;

		BOOL is_success = GetQueuedCompletionStatus(g_iocp, &data_size, &key, &p_over, INFINITE);
		if (is_success == 0) { //에러처리
			cout << "Error in GQCS - key [" << key << "]" << endl;
			DisCountPlayer(key);
			continue;
		}
		if (data_size == 0) { //접속종료 처리
			DisCountPlayer(key);
			continue;
		}

		ExOver *o = reinterpret_cast<ExOver *>(p_over);
		if (o->is_recv == true)
		{
			int work_size = data_size;
			char *ptr = o->io_buffer;

			while (0 < work_size)
			{
				int p_size;//패킷 전체 크기
				if (0 != g_clients[key].packet_size) {
					p_size = g_clients[key].packet_size;
				}
				else {
					p_size = ptr[0];
					g_clients[key].packet_size = p_size;
				}

				int remain = p_size - g_clients[key].prev_size;
				if (remain <= work_size) {

					//pakcet build
					memcpy(g_clients[key].prev_packet + g_clients[key].prev_size,
						ptr, remain);// 이거 해석 한번 해바바

					Util::ProcessPacket(static_cast<int>(key), g_clients[key].prev_packet);
					g_clients[key].prev_size = 0;
					g_clients[key].packet_size = 0;
					work_size -= remain;
					ptr += remain;
				}
				else
				{
					memcpy(g_clients[key].prev_packet + g_clients[key].prev_size,
						ptr,
						work_size);
					g_clients[key].prev_size += work_size;
					work_size -= work_size;
					ptr += work_size;

				}
			}
			unsigned long rflag = 0;
			ZeroMemory(&o->wsaover, sizeof(WSAOVERLAPPED));
			WSARecv(g_clients[key].s, &o->wsabuf, 1, NULL, &rflag, &o->wsaover, NULL);
		}
		else
		{
			delete o;

		}
	}

}

void ThreadAccept()
{
	int retval = 0;
	auto g_socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN server_addr;
	ZeroMemory(&server_addr, sizeof(SOCKADDR_IN));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(SERVERPORT);
	//
	::bind(g_socket, (SOCKADDR *)&server_addr, sizeof(SOCKADDR_IN));
	listen(g_socket, SOMAXCONN);

	while (1)
	{
		SOCKADDR_IN client_addr;
		ZeroMemory(&client_addr, sizeof(SOCKADDR_IN));
		client_addr.sin_family = AF_INET;
		client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		client_addr.sin_port = htons(SERVERPORT);
		int addrlen = sizeof(SOCKADDR_IN);

		auto new_sock = WSAAccept(g_socket, reinterpret_cast<sockaddr*>(&client_addr), &addrlen, NULL, NULL);
		if (new_sock == INVALID_SOCKET)		err_display((char*)"accept()");

		int clientNum = -1;
		for (int i = 0; i < MAX_USER; ++i) {
			if (NumUser[i] == false) {
				clientNum = i + 1;
				NumUser[i] = true;
				break;
			}
		}
		if (clientNum == -1) {
			continue;
		}
		g_clients[clientNum].s = new_sock;
		g_clients[clientNum].x = 4;
		g_clients[clientNum].y = 4;
		ZeroMemory(&g_clients[clientNum].exover.wsaover, sizeof(WSAOVERLAPPED));

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_sock), g_iocp, clientNum, 0);
		g_clients[clientNum].in_use = true;

		unsigned long flag = 0;
		int ret = WSARecv(new_sock, &g_clients[clientNum].exover.wsabuf,
			1, NULL, &flag, &g_clients[clientNum].exover.wsaover, NULL);	//	recv 오류가 안뜨는 문제가 있음
		if (0 != ret)
		{
			int err_num = WSAGetLastError();
			if (WSA_IO_PENDING == err_num)
				err_display("Recv in AcceptThread");
		}

		/*   ==== 시야처리 ====   */
		sc_packet_put_player p;
		p.id = clientNum;
		p.size = sizeof(sc_packet_put_player);
		p.type = SC_PUT_PLAYER;
		p.x = g_clients[clientNum].x;
		p.y = g_clients[clientNum].y;

		//나의 접속을 모든 플레이어에게 알림.

	}
}

