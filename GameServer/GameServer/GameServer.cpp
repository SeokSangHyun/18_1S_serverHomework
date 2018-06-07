

#include "GameServer.h"



struct EXOVER {
	WSAOVERLAPPED m_over;
	char m_iobuf[MAX_BUFF_SIZE];
	WSABUF m_wsabuf;
	char event_type;
	int event_target;
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

// 1번째 방법
//array <NPC*, NUM_OF_NPC> g_NPCs;
//
std::array <Client, MAX_USER> g_clients;



void WakeUpNPC(int npc_id)
{
	//if( true ==  CAS( &m_isactive, false, true)) add_timer();
	if (false == g_clients[npc_id].m_isactive) {
		g_clients[npc_id].m_isactive = true;
		add_timer(npc_id, EVT_MOVE, high_resolution_clock::now() + 1s);
	}
}

void timer_thread()
{
	while (true) {
		this_thread::sleep_for(10us);
		while (false == timer_queue.empty()) {
			if (timer_queue.top().wakeup_t > high_resolution_clock::now())
				break;
			EVENT ev = timer_queue.top();
			timer_queue.pop();
			ProcessEvent(ev);
		}
	}
}



void error_display(const char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"  에러" << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
	while (true);
}

void ErrorDisplay(const char * location)
{
	error_display(location, WSAGetLastError());
}

bool IsNPC(int id)
{
	return (id >= NPC_START) && (id <= NUM_OF_NPC);

}
bool CanSee(int a, int b)
{
	int dist_x = g_clients[a].m_x - g_clients[b].m_x;
	int dist_y = g_clients[a].m_y - g_clients[b].m_y;
	int dist = dist_x * dist_x + dist_y * dist_y;
	return (VIEW_RADIUS * VIEW_RADIUS >= dist);
}
void SendPacket(int id, void *ptr)
{
	unsigned char *packet = reinterpret_cast<unsigned char *>(ptr);
	EXOVER *s_over = new EXOVER;
	memcpy(s_over->m_iobuf, packet, packet[0]);
	s_over->event_type = EVT_SEND;
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
void SendChatPacket(int to, int chatter, wchar_t *mess)
{
	sc_packet_chat pos_packet;
	pos_packet.id = chatter; //누각 얘기를 하는가
	pos_packet.size = sizeof(sc_packet_chat);
	pos_packet.type = SC_CHAT;
	wcsncpy_s(pos_packet.message, mess, MAX_STR_SIZE);

	SendPacket(to, reinterpret_cast<char*>(&pos_packet));
}

int CAPI_send_message(lua_State *L)
{
	char *mess = (char*)lua_tostring(L, -1);
	int chatter = (int)lua_tointeger(L, -2);
	int player = (int)lua_tointeger(L, -3);
	lua_pop(L, 4);

	//mess 유니코드 변환 필요
	wchar_t wmess[MAX_STR_SIZE];
	size_t len = strlen(mess);
	if (len >= MAX_STR_SIZE) len = MAX_STR_SIZE - 1;
	size_t wlen;
	mbstowcs_s(&wlen, wmess, len, mess, _TRUNCATE);
	wmess[MAX_STR_SIZE - 1] = (wchar_t)0;
	SendChatPacket(player, chatter, wmess);
	return 0;
}
int CAPI_get_x(lua_State *L)
{

	int player = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = g_clients[player].m_x;
	lua_pushnumber(L, x);
	return 1;
}
int CAPI_get_y(lua_State *L)
{

	int player = (int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = g_clients[player].m_y;
	lua_pushnumber(L, x);
	return 1;
}


void initialize()
{
	for (auto &cl : g_clients) {
		cl.L = luaL_newstate();
		luaL_openlibs(cl.L);
		luaL_loadfile(cl.L, "monster.lua");
		lua_pcall(cl.L, 0, 0, 0);
	}

	for (int i = NPC_START; i < NUM_OF_NPC; ++i) {
		lua_State *L = luaL_newstate();
		L = luaL_newstate();
		luaL_openlibs(L);
		g_clients[i].m_x = rand() % BOARD_WIDTH;
		g_clients[i].m_y = rand() % BOARD_HEIGHT;
		g_clients[i].m_isactive = false;


		luaL_loadfile(L, "monster.lua");
		lua_pcall(L, 0, 0, 0);
		lua_getglobal(L, "set_myid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);

		lua_register(L, "API_send_message", CAPI_send_message);
		lua_register(L, "API_get_x", CAPI_get_x);
		lua_register(L, "API_get_y", CAPI_get_y);

		g_clients[i].L = L;
	}

	gh_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0); // 의미없는 파라메터, 마지막은 알아서 쓰레드를 만들어준다.
	std::wcout.imbue(std::locale("korean"));

	WSADATA	wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
}

void StartRecv(int id)
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


void SendPutObjectPacket(int client, int object)
{
	sc_packet_put_player p;
	p.id = object;
	p.size = sizeof(p);
	p.type = SC_PUT_PLAYER;
	p.x = g_clients[object].m_x;
	p.y = g_clients[object].m_y;
	SendPacket(client, &p);
}


void SendRemoveObjectPacket(int client, int object)
{
	sc_packet_remove_player p;
	p.id = object;
	p.size = sizeof(p);
	p.type = SC_REMOVE_PLAYER;
	SendPacket(client, &p);
}

void ProcessPacket(int id, char *packet)
{
	int x = g_clients[id].m_x;
	int y = g_clients[id].m_y;
	wstring str = { L"EXEC Get_Id" };
	wstring temp;

	switch (packet[1])
	{
	case CS_UP: if (y > 0) y--; break;
	case CS_DOWN: if (y < BOARD_HEIGHT - 1) y++; break;
	case CS_LEFT: if (x > 0) x--; break;
	case CS_RIGHT: if (x < BOARD_WIDTH - 1) x++; break;
	case CS_LOGIN:
		temp = L" " + to_wstring(packet[2]);
		str += temp;
		DataBaseSQL(str.data(), &x, &y);
		g_clients[id].Id = packet[2];
		break;
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
	for (int i = NPC_START; i < NUM_OF_NPC; ++i) {
		if (true == CanSee(id, i)) continue;

		new_vl.insert(i);
		//시야내에 있는 플레이어가 이동했다는 이벤트 발생
		EXOVER *exover = new EXOVER;
		exover->event_type = EVT_PLAYER_MOVE;
		exover->event_target = id;
		PostQueuedCompletionStatus(gh_iocp, 1, i, &exover->m_over);

	}

	SendPacket(id, &pos_packet);

	// new_vl에는 있는데 old_vl에 없는 경우
	for (auto ob : new_vl) {
		g_clients[id].m_mvl.lock();
		if (0 == g_clients[id].m_viewlist.count(ob)) {
			g_clients[id].m_viewlist.insert(ob);
			g_clients[id].m_mvl.unlock();
			SendPutObjectPacket(id, ob);

			if (true == IsNPC(ob)) {
				WakeUpNPC(ob);
				continue;
			}

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
			// new_vl에도 있고 old_vl에도 있는 경우
			g_clients[id].m_mvl.unlock();
			if (true == IsNPC(ob)) {
				WakeUpNPC(ob);
				continue;
			}
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

			if (true == IsNPC(ob)) continue;
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




void DisconnectPlayer(int id)
{
	wstring str = { L"EXEC Set_Id" };
	wstring temp;
	temp = L" " + to_wstring(g_clients[id].Id);
	str += temp;
	temp = L", " + to_wstring(g_clients[id].m_x);
	str += temp;
	temp = L", " + to_wstring(g_clients[id].m_y);
	str += temp;
	DataBaseSQL(str.data(), &g_clients[id].m_x, &g_clients[id].m_y);

	sc_packet_remove_player p;
	p.id = id;
	p.size = sizeof(p);
	p.type = SC_REMOVE_PLAYER;
	for (int i = 0; i < MAX_USER; ++i) {
		if (false == g_clients[i].m_isconnected) continue;
		if (i == id) continue;
		if (true == IsNPC(i)) break;
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

void MoveNPC(int i)
{
	unordered_set <int> old_vl;
	for (int id = 0; id < MAX_USER; ++id) {
		if (false == g_clients[id].m_isconnected) continue;
		if (true == CanSee(i, id)) old_vl.insert(id);
	}
	switch (rand() % 4)
	{
	case 0: if (g_clients[i].m_x > 0) g_clients[i].m_x--; break;
	case 1: if (g_clients[i].m_x < BOARD_WIDTH - 1) g_clients[i].m_x++; break;
	case 2: if (g_clients[i].m_y > 0) g_clients[i].m_y--; break;
	case 3: if (g_clients[i].m_y < BOARD_HEIGHT - 1) g_clients[i].m_y++; break;
	}
	// 움직인 후에 나를 보고 있는 애들
	unordered_set <int> new_vl;
	for (int id = 0; id < MAX_USER; ++id) {
		if (false == g_clients[id].m_isconnected) continue;
		if (true == CanSee(i, id)) new_vl.insert(id);
	}

	sc_packet_pos pos_p;
	pos_p.id = i;
	pos_p.size = sizeof(pos_p);
	pos_p.type = SC_POS;
	pos_p.x = g_clients[i].m_x;
	pos_p.y = g_clients[i].m_y;

	// 멀어진 플레이어에서 시야 삭제 ( 이미 나를 보고 있었던 애들에 대해서, 내가 움직인 다음에도 보고 있느냐? )
	for (auto id : old_vl) {
		if (0 == new_vl.count(id)) {		// 보고있지 않다면 remove를 해주어야 한다.
			g_clients[id].m_mvl.lock();
			if (g_clients[id].m_viewlist.count(i)) {
				g_clients[id].m_viewlist.erase(i);
				g_clients[id].m_mvl.unlock();
				SendRemoveObjectPacket(id, i);
			}
			else {
				g_clients[id].m_mvl.unlock();
			}
		}
		else {
			// 계속 보고 있다면 move 패킷 보내줌
			g_clients[id].m_mvl.lock();
			if (0 != g_clients[id].m_viewlist.count(i)) {
				g_clients[id].m_mvl.unlock();
				SendPacket(id, &pos_p);
			}
			else {
				g_clients[id].m_viewlist.insert(i);
				g_clients[id].m_mvl.unlock();
				SendPutObjectPacket(id, i);
			}
		}
	}
	// 새로 만난 플레이어에게 시야 추가
	for (auto id : new_vl) {
		if (0 == old_vl.count(id)) {
			g_clients[id].m_mvl.lock();
			if (0 == g_clients[id].m_viewlist.count(i)) {
				g_clients[id].m_viewlist.insert(i);
				g_clients[id].m_mvl.unlock();
				SendPutObjectPacket(id, i);
			}
			else {
				g_clients[id].m_mvl.unlock();
				SendPacket(id, &pos_p);
			}
		}
	}

	if (false == new_vl.empty())
		add_timer(i, EVT_MOVE, high_resolution_clock::now() + 1s);
	else
		g_clients[i].m_isactive = false;
}


void worker_thread()
{
	while (true)
	{
		unsigned long io_size;
		unsigned long long iocp_key; // 64 비트 integer , 우리가 64비트로 컴파일해서 64비트
		WSAOVERLAPPED *over;
		BOOL ret = GetQueuedCompletionStatus(gh_iocp, &io_size, &iocp_key, &over, INFINITE);
		int key = static_cast<int>(iocp_key);
		cout << "WT::Network I/O with Client [" << key << "]\n";
		if (FALSE == ret) {
			cout << "Error in GQCS\n";
			DisconnectPlayer(key);
			continue;
		}
		if (0 == io_size) {
			DisconnectPlayer(key);
			continue;
		}

		EXOVER *p_over = reinterpret_cast<EXOVER *>(over);
		if (EVT_RECV == p_over->event_type) {
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
					ProcessPacket(key, g_clients[key].m_packet);
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
			StartRecv(key);
		}
		else if (EVT_SEND == p_over->event_type) {  // Send 후처리
			cout << "WT:A packet was sent to Client[" << key << "]\n";
			delete p_over;
		}
		else if (EVT_MOVE == p_over->event_type) {
			MoveNPC(static_cast<int>(iocp_key));
		}
		else if (EVT_PLAYER_MOVE == p_over->event_type)
		{
			EXOVER *o = reinterpret_cast<EXOVER *>(p_over);
			int player = o->event_target;
			lua_getglobal(g_clients[key].L, "event_player_move");
			lua_pushnumber(g_clients[key].L, player);
			lua_pcall(g_clients[key].L, 1, 0, 0);
			delete o;
		}
		else {
			cout << "Unkonwn Event Type detected in worker thread!\n";
		}
	}
}

void accept_thread()	//새로 접속해 오는 클라이언트를 IOCP로 넘기는 역할
{
	SOCKET s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN bind_addr;
	ZeroMemory(&bind_addr, sizeof(SOCKADDR_IN));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(MY_SERVER_PORT);
	bind_addr.sin_addr.s_addr = INADDR_ANY;	// 0.0.0.0  아무대서나 오는 것을 다 받겠다.

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
		c_addr.sin_port = htons(MY_SERVER_PORT);
		c_addr.sin_addr.s_addr = INADDR_ANY;	// 0.0.0.0  아무대서나 오는 것을 다 받겠다.
		int addr_size = sizeof(sockaddr);

		SOCKET cs = WSAAccept(s, reinterpret_cast<sockaddr *>(&c_addr), &addr_size, NULL, NULL);
		if (INVALID_SOCKET == cs) {
			ErrorDisplay("In Accept Thread:WSAAccept()");
			continue;
		}
		cout << "New Client Connected!\n";

		////////////////////////////
		//StartRecv(LOGIN);
		//BOOL ret = GetQueuedCompletionStatus(gh_iocp, &io_size, &iocp_key, &over, INFINITE);

		int id = -1;
		for (int i = 0; i < MAX_USER; ++i)
			if (false == g_clients[i].m_isconnected) {
				id = i;
				break;
			}
		if (-1 == id) {
			cout << "MAX USER Exceeded\n";
			continue;
		}


		g_clients[id].m_x = 0;
		g_clients[id].m_y = 0;

		cout << "ID of new Client is [" << id << "]";
		g_clients[id].m_s = cs;
		g_clients[id].m_packet_size = 0;
		g_clients[id].m_prev_packet_size = 0;
		g_clients[id].m_viewlist.clear();

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(cs), gh_iocp, id, 0);
		g_clients[id].m_isconnected = true;
		StartRecv(id);

		sc_packet_put_player p;
		p.id = id;
		p.size = sizeof(p);
		p.type = SC_PUT_PLAYER;
		p.x = g_clients[id].m_x;
		p.y = g_clients[id].m_y;

		SendPacket(id, &p);

		// 나의 접속을 기존 플레이어들에게 알려준다.
		for (int i = 0; i<MAX_USER; ++i)
			if (true == g_clients[i].m_isconnected) {
				if (false == CanSee(i, id)) continue;
				if (i == id) continue;
				g_clients[i].m_mvl.lock();
				g_clients[i].m_viewlist.insert(id);
				g_clients[i].m_mvl.unlock();
				SendPacket(i, &p);
			}

		// 나에게 이미 접속해 있는 플레이어들의 정보를 알려준다.
		for (int i = 0; i < MAX_USER; ++i) {
			if (false == g_clients[i].m_isconnected) continue;
			if (i == id) continue;
			if (false == CanSee(i, id)) continue;
			p.id = i;
			p.x = g_clients[i].m_x;
			p.y = g_clients[i].m_y;
			g_clients[id].m_mvl.lock();
			g_clients[id].m_viewlist.insert(i);
			g_clients[id].m_mvl.unlock();
			SendPacket(id, &p);
		}
		// 주위에 있는 NPC 정보를 알려 준다.
		for (int i = NPC_START; i < NUM_OF_NPC; ++i) {
			if (false == CanSee(i, id)) continue;
			p.id = i;
			p.x = g_clients[i].m_x;
			p.y = g_clients[i].m_y;
			g_clients[id].m_mvl.lock();
			g_clients[id].m_viewlist.insert(i);
			g_clients[id].m_mvl.unlock();
			WakeUpNPC(i);
			SendPacket(id, &p);
		}


	}
}

void NPC_ai_thread()
{
	while (true) {
		Sleep(1000);
		for (int i = NPC_START; i < NUM_OF_NPC; ++i) {

			//if (false == g_clients[i].m_awaken) continue; // 깨어나지 않은 애들은 움직이지 말자 누가 꺠웠냐와 누가 false를 만들었느냐. 모든 NPC상대로 검색. 바람직하지 않음.
			// 깨어있지 않는 NPC 자체는 건드리지를 말아야 함 , 캐시 오버헤드.
			// 검색이 아니라 타이머를 통해 구현

			// 움직이기 전에 나를 보고 있는 애들
			unordered_set <int> old_vl;
			for (int id = 0; id < MAX_USER; ++id) {
				if (false == g_clients[id].m_isconnected) continue;
				if (true == CanSee(i, id)) old_vl.insert(id);
			}
			switch (rand() % 4)
			{
			case 0: if (g_clients[i].m_x > 0) g_clients[i].m_x--; break;
			case 1: if (g_clients[i].m_x < BOARD_WIDTH - 1) g_clients[i].m_x++; break;
			case 2: if (g_clients[i].m_y > 0) g_clients[i].m_y--; break;
			case 3: if (g_clients[i].m_y < BOARD_HEIGHT - 1) g_clients[i].m_y++; break;
			}
			// 움직인 후에 나를 보고 있는 애들
			unordered_set <int> new_vl;
			for (int id = 0; id < MAX_USER; ++id) {
				if (false == g_clients[id].m_isconnected) continue;
				if (true == CanSee(i, id)) new_vl.insert(id);
			}

			sc_packet_pos pos_p;
			pos_p.id = i;
			pos_p.size = sizeof(pos_p);
			pos_p.type = SC_POS;
			pos_p.x = g_clients[i].m_x;
			pos_p.y = g_clients[i].m_y;

			// 멀어진 플레이어에서 시야 삭제 ( 이미 나를 보고 있었던 애들에 대해서, 내가 움직인 다음에도 보고 있느냐? )
			for (auto id : old_vl) {
				if (0 == new_vl.count(i)) {		// 보고있지 않다면 remove를 해주어야 한다.
					g_clients[id].m_mvl.lock();
					if (g_clients[id].m_viewlist.count(i)) {
						g_clients[id].m_viewlist.erase(i);
						g_clients[id].m_mvl.unlock();
						SendRemoveObjectPacket(id, i);
					}
					else {
						g_clients[id].m_mvl.unlock();
					}
				}
				else {
					// 계속 보고 있다면 move 패킷 보내줌
					g_clients[id].m_mvl.lock();
					if (0 != g_clients[id].m_viewlist.count(i)) {
						g_clients[id].m_mvl.unlock();
						SendPacket(id, &pos_p);
					}
					else {
						g_clients[id].m_viewlist.insert(i);
						g_clients[id].m_mvl.unlock();
						SendPutObjectPacket(id, i);
					}
				}
			}
			// 새로 만난 플레이어에게 시야 추가
			for (auto id : new_vl) {
				if (0 == old_vl.count(id)) {
					g_clients[id].m_mvl.lock();
					if (0 == g_clients[id].m_viewlist.count(i)) {
						g_clients[id].m_viewlist.insert(i);
						g_clients[id].m_mvl.unlock();
						SendPutObjectPacket(id, i);
					}
					else {
						g_clients[id].m_mvl.unlock();
						SendPacket(id, &pos_p);
					}
				}
			}

		}
	}
}
int main()
{
	vector <thread> w_threads;
	initialize();
	//CreateWorkerThreads();	// 쓰레드 조인까지 이 안에서 해주어야 한다. 전역변수 해서 관리를 해야 함. 전역변수 만드는 것은
	// 좋은 방법이 아님.
	for (int i = 0; i < 4; ++i) w_threads.push_back(thread{ worker_thread }); // 4인 이유는 쿼드코어 CPU 라서
																			  //CreateAcceptThreads();
	thread a_thread{ accept_thread };
	//thread ai_thread{ NPC_ai_thread };
	thread t_thread{ timer_thread };
	for (auto& th : w_threads) th.join();
	a_thread.join();
	//ai_thread.join();
	t_thread.join();
}