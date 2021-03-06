
#include "stdafx.h"
#include "GameServer.h"

using namespace std;


std::array <Client, MAX_USER> g_clients;
CFramework *g_framework;


HANDLE g_iocp;


void ErrorDisplay(const char * location)
{
	error_display(location, WSAGetLastError());
}




/*/////////////////////////////////////////////////////////////////////*/
/*/////////////////////////////////////////////////////////////////////*/
int main()
{
	g_framework = new CFramework;
	vector <thread> w_threads;
	initialize();


	for (int i = 0; i < 4; ++i) w_threads.push_back(thread{ WorkerThread });													  
	thread a_thread{ AcceptThread };


	for (auto& th : w_threads) th.join();
	a_thread.join();
}
/////////////////////////////////////////////////////////////////////////



void initialize()
{
	g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0); // 의미없는 파라메터, 마지막은 알아서 쓰레드를 만들어준다.
	std::wcout.imbue(std::locale("korean"));

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
}

// Accept
void AcceptThread()	//새로 접속해 오는 클라이언트를 IOCP로 넘기는 역할
{
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN bind_addr;
	ZeroMemory(&bind_addr, sizeof(SOCKADDR_IN));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(SERVERPORT);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	::bind(s, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
	listen(s, 1000);


	unsigned long io_size;
	unsigned long long iocp_key; // 64 비트 integer , 우리가 64비트로 컴파일해서 64비트
	WSAOVERLAPPED *over;
	Client dumy_serClint;
	while (true)
	{
		SOCKADDR_IN c_addr;
		ZeroMemory(&c_addr, sizeof(SOCKADDR_IN));
		c_addr.sin_family = AF_INET;
		c_addr.sin_port = htons(SERVERPORT);
		c_addr.sin_addr.s_addr = INADDR_ANY;
		int addr_size = sizeof(sockaddr);

		SOCKET cs = WSAAccept(s, reinterpret_cast<sockaddr *>(&c_addr), &addr_size, NULL, NULL);
		if (INVALID_SOCKET == cs) {
			ErrorDisplay("In Accept Thread:WSAAccept()");
			continue;
		}
		cout << "New Client Connected!\n";

		////////////////////////////
		int id = -1;
		for(int i = 0 ; i < MAX_USER ; ++i)
			if (false == g_clients[i].m_isconnected) {
				id = i;
				break;
			}
		if (-1 == id) {
			cout << "MAX USER Exceeded\n";
			continue;
		}

		g_clients[id].m_s = cs;

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(cs), g_iocp, id, 0);
		g_clients[id].m_isconnected = true;
		g_framework->StartRecv(id);


		sc_packet_put_player p;
		p.id = id;
		p.size = sizeof(p);
		p.type = SC_PUT_PLAYER;
		p.x = g_clients[id].m_x;
		p.y = g_clients[id].m_y;

		g_framework->SendPacket(id, &p);

		// 나의 접속을 기존 플레이어들에게 알려준다.
		for (int i = 0; i<MAX_USER; ++i)
			if (true == g_clients[i].m_isconnected) {
				if (false == g_framework->CanSee(i, id)) continue;
				if (i == id) continue;
				g_clients[i].m_mvl.lock();
				g_clients[i].m_viewlist.insert(id);
				g_clients[i].m_mvl.unlock();
				g_framework->SendPacket(i, &p);
			}

		// 나에게 이미 접속해 있는 플레이어들의 정보를 알려준다.
		for (int i = 0; i < MAX_USER; ++i) {
			if (false == g_clients[i].m_isconnected) continue;
			if (i == id) continue;
			if (false == g_framework->CanSee(i, id)) continue;
			p.id = i;
			p.x = g_clients[i].m_x;
			p.y = g_clients[i].m_y;
			g_clients[id].m_mvl.lock();
			g_clients[id].m_viewlist.insert(i);
			g_clients[id].m_mvl.unlock();
			g_framework->SendPacket(id, &p);
		}


	}
}


// WorkerThread
void WorkerThread()
{
	while (true)
	{
		unsigned long io_size;
		unsigned long long iocp_key; // 64 비트 integer , 우리가 64비트로 컴파일해서 64비트
		WSAOVERLAPPED *over;
		BOOL ret = GetQueuedCompletionStatus(g_iocp, &io_size, &iocp_key, &over, INFINITE);
		int key = static_cast<int>(iocp_key);
		cout << "WT::Network I/O with Client [" << key << "]\n";
		if (FALSE == ret) {
			cout << "Error in GQCS\n";
			g_framework->DisconnectPlayer(key);
			continue;
		}
		if (0 == io_size) {
			g_framework->DisconnectPlayer(key);
			continue;
		}

		EXOVER *p_over = reinterpret_cast<EXOVER *>(over);
		if (true == p_over->event_type) {
			cout << "WT:Packet From Client [" << key << "]\n";
			int work_size = io_size;
			char *wptr = p_over->m_iobuf;
			while (0 < work_size) {
				int p_size;
				if (0 != g_clients[key].m_packet_size)
					p_size = g_clients[key].m_packet_size;
				else {
					p_size = wptr[0];
					g_clients[key].m_packet_size = p_size;
				}
				int need_size = p_size - g_clients[key].m_prev_packet_size;
				if (need_size <= work_size) {
					memcpy(g_clients[key].m_packet
						+ g_clients[key].m_prev_packet_size, wptr, need_size);
					g_framework->ProcessPacket(key, g_clients[key].m_packet);
					g_clients[key].m_prev_packet_size = 0;
					g_clients[key].m_packet_size = 0;
					work_size -= need_size;
					wptr += need_size;
				}
				else {
					memcpy(g_clients[key].m_packet + g_clients[key].m_prev_packet_size, wptr, work_size);
					g_clients[key].m_prev_packet_size += work_size;
					work_size = -work_size;
					wptr += work_size;
				}
			}
			g_framework->StartRecv(key);
		}
		else if (false == p_over->event_type) {  // Send 후처리
			cout << "WT:A packet was sent to Client[" << key << "]\n";
			delete p_over;
		}
		else {
			cout << "Unkonwn Event Type detected in worker thread!\n";
		}
	}
}






