#include <WinSock2.h>
#include <winsock.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <queue>
#include <fstream>
#include <random>
#include <ctime>


#include <sqlext.h>

#define NAME_LEN 50
#define PHONE_LEN 20
#define POS_LEN 400

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

#include "protocol.h"

HANDLE g_hiocp;
SOCKET g_ssocket;

enum OPTYPE { OP_SEND, OP_RECV, OP_DO_AI, OP_PLAYER_MOVE_NOTIFY, OP_RESPAWN, OP_REVIVAL };
enum NPC_MOVE { NONE, LEFT, RIGHT, UP, DOWN };

std::chrono::high_resolution_clock::time_point g_time;
using namespace std;
using namespace chrono;

int g_map[MAP_SIZE][MAP_SIZE];

struct OverlappedEx {
	WSAOVERLAPPED over;
	WSABUF wsabuf;
	unsigned char IOCP_buf[MAX_BUFF_SIZE];
	OPTYPE event_type;
	int event_target;
};

struct CLIENT {
	int x;
	int y;
	char id[10];
	int initi_x;
	int initi_y;

	std::chrono::high_resolution_clock::time_point last_move_time;

	bool is_active;
	bool connect;

	lua_State *L;

	SOCKET client_socket;
	OverlappedEx recv_over;
	unsigned char packet_buf[MAX_PACKET_SIZE];
	int prev_packet_data;
	int curr_packet_size;
	std::unordered_set<int> view_list;
	std::mutex vl_lock;

	int bye;
	bool move;
	int count;

	int hp;
	int level;
	int exp;
	int attack;
	int maxhp;

	bool trac;
	int trac_client;
	int turn;
	int monster_level;
	bool die;
	int exp_sum;
	high_resolution_clock::time_point revival_time;
	high_resolution_clock::time_point active_time;
};

enum Event_Type { E_MOVE, E_RESPAWN, E_REVIVAL };

struct Timer_Event {
	int object_id;
	high_resolution_clock::time_point exec_time;
	Event_Type event;
};

class comparison {
	bool reverse;
public:
	comparison() {}
	bool operator() (const Timer_Event first, const Timer_Event second) const {
		return first.exec_time > second.exec_time;
	}
};

priority_queue <Timer_Event, vector<Timer_Event>, comparison> timer_queue;
mutex tq_lock;

CLIENT g_clients[NUM_OF_NPC];

SQLHENV henv;
SQLHDBC hdbc;
SQLHSTMT hstmt = 0;
SQLRETURN retcode;
SQLWCHAR ID[NAME_LEN];
int xPos, yPos, iExp, iExps, iAtt, ihp, iLv;
SQLLEN LID = 0, LxPos = 0, LyPos = 0, L_lv, L_hp, L_exp, L_exps,L_att;

SQLWCHAR dbID[NAME_LEN];
int db_x_pos, db_y_pos, db_lv, db_exp, db_exps,db_attack, db_hp;
SQLLEN S_ID = 0, S_xPos = 0, S_yPos = 0, S_lv, S_hp,S_exps,S_att,S_exp;


wchar_t g_sqldata[255];
char g_savebuf[255];

void SendChatPacket(int client, int object, wchar_t *mess);

void show_error() {
	cout << "error\n";
}
void error_display(char *msg, int err_no)
{
	WCHAR *lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	std::cout << msg;
	std::wcout << L"����" << lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
	while (true);
}

bool Is_Close(int from, int to)
{
	return (g_clients[from].x - g_clients[to].x)
		* (g_clients[from].x - g_clients[to].x)
		+ (g_clients[from].y - g_clients[to].y)
		* (g_clients[from].y - g_clients[to].y) <= VIEW_RADIUS *VIEW_RADIUS;
}

void Initialize_DB()
{
	std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
			}
		}
	}
}

bool Is_Attcked(int from, int to)
{
	if (g_clients[from].x - 1 == g_clients[to].x && g_clients[from].y == g_clients[to].y) return true;
	if (g_clients[from].x + 1 == g_clients[to].x && g_clients[from].y == g_clients[to].y) return true;
	if (g_clients[from].x == g_clients[to].x && g_clients[from].y - 1 == g_clients[to].y) return true;
	if (g_clients[from].x == g_clients[to].x && g_clients[from].y + 1 == g_clients[to].y) return true;
	return false;
}

bool Is_Buy(int from, int x, int y)
{
	if (g_clients[from].x == x && g_clients[from].y == y) return true;
	return false;
}

bool Is_NPC(int id)
{
	return (id >= NPC_START) && (id < NUM_OF_NPC);
}

int API_get_x(lua_State *L)
{
	int oid = lua_tonumber(L, -1);
	lua_pop(L, 2);
	lua_pushnumber(L, g_clients[oid].x);
	return 1;
}

int API_get_y(lua_State *L)
{
	int oid = lua_tonumber(L, -1);
	lua_pop(L, 2);
	lua_pushnumber(L, g_clients[oid].y);
	return 1;
}

int API_bye(lua_State *L)
{
	int oid = lua_tonumber(L, -1);
	lua_pop(L, 2);
	lua_pushnumber(L, g_clients[oid].count);

	if (g_clients[oid].count > 2)
	{
		g_clients[oid].count = 0;
		g_clients[oid].bye = NONE;
	}
	return 1;
}

int API_SendMessage(lua_State *L)
{
	int client_id = lua_tonumber(L, -3);
	int oid = lua_tonumber(L, -2);

	char *mess = (char *)lua_tostring(L, -1);
	size_t wlen, len = strlen(mess) + 1;
	wchar_t wmess[MAX_STR_SIZE];

	lua_pop(L, 3);

	len = (MAX_STR_SIZE - 1 < len) ? MAX_STR_SIZE - 1 : len;

	mbstowcs_s(&wlen, wmess, len, mess, _TRUNCATE);
	wmess[MAX_STR_SIZE - 1] = (wchar_t)0;

	SendChatPacket(oid, client_id, wmess);
	return 0;
}

void SetPos(int beginX, int endX, int beginY, int endY, int startNum, int endNUM)
{
	std::default_random_engine dre(unsigned int(NULL));
	std::uniform_int_distribution<> limitX(beginX, endX);
	std::uniform_int_distribution<> limitY(beginY, endY);


	for (int i = startNum; i < endNUM; ++i)
	{
		g_clients[i].x = limitX(dre);
		g_clients[i].y = limitY(dre);
		g_clients[i].initi_x = g_clients[i].x;
		g_clients[i].initi_y = g_clients[i].y;
	}
}

void Initialize_NPC()
{
	//������� ��� ���� �ƿ�  
	SetPos(108, 148, 60, 230, 1000, 2000);
	SetPos(167, 298, 30, 60, 2000, 2050);
	SetPos(1, 85, 239, 269, 2050, NUM_OF_NPC);

	for (int i = NPC_START; i < NUM_OF_NPC; ++i) {

		//g_clients[i].x = rand() % 300;
		//g_clients[i].y = rand() % 300;
		g_clients[i].last_move_time = std::chrono::high_resolution_clock::now();
		g_clients[i].connect = false;
		g_clients[i].is_active = false;
		g_clients[i].bye = NONE;
		g_clients[i].move = false;
		g_clients[i].count = 0;
		g_clients[i].hp = 100;
		g_clients[i].level = 1;
		g_clients[i].exp = 0;
		g_clients[i].trac = false;
		g_clients[i].trac_client = -1;

		if (i < 2000)
			g_clients[i].monster_level = 1;
		else if (i < 2050)
			g_clients[i].monster_level = 2;
		else if (i < NUM_OF_NPC)
		{
			g_clients[i].trac = true;
			g_clients[i].monster_level = 3;
		}

		lua_State *L = luaL_newstate();
		luaL_openlibs(L);

		luaL_loadfile(L, "mons.lua");

		if (0 != lua_pcall(L, 0, 0, 0))
		{
			printf("LUA Error : %s\n", lua_tostring(L, -1));
			lua_close(L);
		}

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 1, 0);
		lua_pop(L, 1);

		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_bye", API_bye);

		g_clients[i].L = L;

		Timer_Event t = { i, high_resolution_clock::now() + 5s, E_RESPAWN };
		tq_lock.lock();
		timer_queue.push(t);
		tq_lock.unlock();
	}
}

void SendPacket(int cl, void *packet)
{
	int psize = reinterpret_cast<unsigned char *>(packet)[0];
	int ptype = reinterpret_cast<unsigned char *>(packet)[1];
	OverlappedEx *over = new OverlappedEx;
	over->event_type = OP_SEND;
	memcpy(over->IOCP_buf, packet, psize);
	ZeroMemory(&over->over, sizeof(over->over));
	over->wsabuf.buf = reinterpret_cast<CHAR *>(over->IOCP_buf);
	over->wsabuf.len = psize;
	int ret = WSASend(g_clients[cl].client_socket, &over->wsabuf, 1, NULL, 0,
		&over->over, NULL);
	if (0 != ret) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			error_display("Error in SendPacket:", err_no);
	}
	//std::cout << "Send Packet [" << ptype << "] To Client : " << cl << std::endl;
}

void SendDisconnectedPacket(int client, int object)
{
	sc_pakcet_discon packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_LOGIN_FAILED;
	SendPacket(client, &packet);
}

void WakeUpNPC(int id)
{
	if (true == g_clients[id].is_active) return;
	g_clients[id].is_active = true;
	Timer_Event event = { id, high_resolution_clock::now() + 1s ,E_MOVE };
	tq_lock.lock();
	timer_queue.push(event);
	tq_lock.unlock();
}

void Initialize_Server()
{
	Initialize_NPC();

	std::wcout.imbue(std::locale("korean"));
	WSADATA   wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	g_hiocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);

	g_ssocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN ServerAddr;
	ZeroMemory(&ServerAddr, sizeof(SOCKADDR_IN));
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(MY_SERVER_PORT);
	ServerAddr.sin_addr.s_addr = INADDR_ANY;

	::bind(g_ssocket, reinterpret_cast<sockaddr *>(&ServerAddr), sizeof(ServerAddr));
	listen(g_ssocket, 5);

	for (int i = 0; i < MAX_USER; ++i) {
		g_clients[i].connect = false;
		g_clients[i].L = nullptr;
	}

	ifstream in("map.txt");
	for (int i = 0; i < MAP_SIZE; ++i)
		for (int j = 0; j < MAP_SIZE; ++j)
			in >> g_map[j][i];
}


void SendPutPlayerPacket(int client, int object)
{
	sc_packet_put_player packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_PUT_PLAYER;
	packet.x = g_clients[object].x;
	packet.y = g_clients[object].y;
	SendPacket(client, &packet);
}

void SendPositionPacket(int client, int object)
{
	sc_packet_pos packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = g_clients[object].x;
	packet.y = g_clients[object].y;
	SendPacket(client, &packet);
}

void SendRemovePlayerPacket(int client, int object)
{
	sc_packet_pos packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_REMOVE_PLAYER;

	SendPacket(client, &packet);
}

void SendStatePacket(int client, int object)
{
	sc_packet_state packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_STATE;
	packet.hp = g_clients[object].hp;
	packet.attack = g_clients[object].attack;
	packet.max_hp = g_clients[object].maxhp;
	packet.expsum = g_clients[object].exp_sum;
	packet.level = g_clients[object].level;
	packet.exp = g_clients[object].exp;
	SendPacket(client, &packet);
}

void Check_Monster(int ci)
{

}

void Check_Player_Info(int ci)
{


}

void Check_Player_State()
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (g_clients[i].connect == false) continue;
		Check_Player_Info(i);
		SendStatePacket(i, i);
	}
}

int CheckMove(int id)
{
	g_clients[id].bye = rand() % 4 + 1;

	while (1) {
		switch (g_clients[id].bye) {
		case LEFT:
			if (g_clients[id].x > 3) {
				return LEFT;
				break;
			}
			else
				g_clients[id].bye = rand() % 4 + 1;
		case RIGHT:
			if (g_clients[id].x - 3 > BOARD_WIDTH - 2) {
				return RIGHT;
				break;
			}
			else
				g_clients[id].bye = rand() % 4 + 1;
			break;
		case UP:
			if (g_clients[id].y > 3) {
				return UP;
				break;
			}
			else
				g_clients[id].bye = rand() % 4 + 1;
		case DOWN:
			if (g_clients[id].y - 3 > BOARD_HEIGHT - 2) {
				return DOWN;
				break;
			}
			else
				g_clients[id].bye = rand() % 4 + 1;
		}
	}
}

void SendChatPacket(int client, int object, wchar_t *mess)
{
	sc_packet_chat packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	if (MAX_STR_SIZE <= wcslen(mess)) {
		cout << "Message too long at SendChatPacket()\n";
		return;
	}
	wcscpy(packet.message, mess);

	if (g_clients[object].move)
	{
		g_clients[object].vl_lock.lock();
		g_clients[object].move = false;
		g_clients[object].bye = NONE;
		g_clients[object].count = 0;
		g_clients[object].vl_lock.unlock();
	}
	else if (!g_clients[object].move)
	{
		g_clients[object].vl_lock.lock();
		CheckMove(object);
		g_clients[object].move = true;
		g_clients[object].vl_lock.unlock();
	}

	SendPacket(client, &packet);
}

void NPC_HP(int ci, int obj, unsigned char packet[])
{
	cs_packet_skill *my_packet = reinterpret_cast<cs_packet_skill*>(packet);
	if (g_clients[obj].hp > 0 && g_clients[obj].is_active)
	{
		g_clients[obj].hp -= g_clients[ci].attack;
		g_clients[obj].trac = true;
		if (g_clients[obj].hp <= 0)
		{
			g_clients[ci].exp += 20 * g_clients[obj].monster_level;
			g_clients[ci].exp_sum += g_clients[ci].exp;
			if (g_clients[ci].exp >= 100 * g_clients[ci].level)
			{
				g_clients[ci].exp -= 100;
				g_clients[ci].level += 1;
			}
			SendRemovePlayerPacket(ci, obj);
		}
	}
	SendStatePacket(ci, ci);
}

void NPC_Respawn(int npc)
{
	if (g_clients[npc].hp <= 0) {
		//cout << npc << " resp" << g_clients[npc].x << " " << g_clients[npc].initi_x << endl;
		g_clients[npc].connect = true;
		g_clients[npc].x = g_clients[npc].initi_x;
		g_clients[npc].y = g_clients[npc].initi_y;
		g_clients[npc].is_active = true;
		g_clients[npc].bye = NONE;
		g_clients[npc].move = false;
		g_clients[npc].count = 0;
		g_clients[npc].hp = 100;
		g_clients[npc].level = 1;
		g_clients[npc].exp = 0;
		g_clients[npc].trac_client = -1;
		if (2050 < npc &&npc < NUM_OF_NPC)
			g_clients[npc].trac = true;
		else
			g_clients[npc].trac = false;
		for (int i = 0; i < MAX_USER; ++i)
			if (g_clients[i].connect)
				SendPutPlayerPacket(i, npc);
	}
	Timer_Event t = { npc, high_resolution_clock::now() + 5s, E_RESPAWN };
	tq_lock.lock();
	timer_queue.push(t);
	tq_lock.unlock();
}

void Check_Battle(int ci, unsigned char packet[])
{
	cs_packet_skill *my_packet = reinterpret_cast<cs_packet_skill*>(packet);
	for (int i = NPC_START; i < NUM_OF_NPC; ++i)
		if (g_clients[i].is_active)
			if (Is_Attcked(ci, i))
				NPC_HP(ci, i, packet);
}

void Player_Respawn(int ci)
{
	cout << boolalpha << g_clients[ci].die << endl;
	if (g_clients[ci].die) {
		cout << "PR" << endl;
		g_clients[ci].die = false;
		g_clients[ci].hp = 100;
		g_clients[ci].x = 4;
		g_clients[ci].y = 4;
		g_clients[ci].exp = 0;
		SendPutPlayerPacket(ci, ci);
		for (int i = 0; i < MAX_USER; ++i)
			if (g_clients[i].connect) {
				SendPutPlayerPacket(ci, i);
			}
	}
	//Timer_Event t = { ci, high_resolution_clock::now() + 2s, E_REVIVAL };
	//tq_lock.lock();
	//timer_queue.push(t);
	//tq_lock.unlock();
}

void Buy_State(int ci, unsigned char packet[])
{
	cs_packet_store *my_packet = reinterpret_cast<cs_packet_store*>(packet);
	cout << g_clients[ci].exp_sum << ", " << g_clients[ci].maxhp << endl;
	if (g_clients[ci].exp_sum != 0)
	{
		//HP
		if (Is_Buy(ci, 165, 153))
			if (g_clients[ci].exp_sum > 10) {
				g_clients[ci].exp_sum -= 10;
				g_clients[ci].maxhp += 10;
				g_clients[ci].hp = g_clients[ci].maxhp;
			}
		//AT
		if (Is_Buy(ci, 180, 153))
			if (g_clients[ci].exp_sum > 10) {
				g_clients[ci].exp_sum -= 10;
				g_clients[ci].attack += 10;
			}
		SendStatePacket(ci, ci);
		for (int i = 0; i < MAX_USER; ++i) {
			if (g_clients[i].connect)
				SendStatePacket(i, ci);
		}
	}
}

void ProcessPacket(int ci, unsigned char packet[])
{
	cs_packet_up *my_packet = reinterpret_cast<cs_packet_up*>(packet);

	if (g_clients[ci].die) return;

	int mapx = g_clients[ci].x, mapy = g_clients[ci].y;
	switch (packet[1])
	{

	case CS_UP:
		if (BG != g_map[mapx][mapy - 1])
			if (g_clients[ci].y > 0) g_clients[ci].y--; break;
	case CS_DOWN:
		if (BG != g_map[mapx][mapy + 1])
			if (g_clients[ci].y < MAP_SIZE - 1) g_clients[ci].y++; break;
	case CS_LEFT:
		if (BG != g_map[mapx - 1][mapy])
			if (g_clients[ci].x > 0) g_clients[ci].x--; break;
	case CS_RIGHT:
		if (BG != g_map[mapx + 1][mapy])
			if (g_clients[ci].x < MAP_SIZE - 1) g_clients[ci].x++; break;
	case CS_SKILL:
		Check_Battle(ci, packet);
		break;
	case CS_TP:
		//150 150 50 242
		if (my_packet->zone == 1) {
			g_clients[ci].x = 130;
			g_clients[ci].y = 50;
		}
		if (my_packet->zone == 2) {
			g_clients[ci].x = 150;
			g_clients[ci].y = 150;
		}
		if (my_packet->zone == 3) {
			g_clients[ci].x = 88;
			g_clients[ci].y = 250;
		}
		if (my_packet->zone == 4) {
			g_clients[ci].x = 250;
			g_clients[ci].y = 250;
		}
		break;
	case CS_STORE:
		Buy_State(ci, packet);
		break;
	default: std::cout << "Unknown Packet Type from Client : " << ci << std::endl;
		while (true);
	}

	SendPositionPacket(ci, ci);
	std::unordered_set<int> new_view_list;
	for (int i = 0; i < MAX_USER; ++i)
		if (true == g_clients[i].connect)
			if (i != ci)
				if (true == Is_Close(ci, i))
					new_view_list.insert(i);

	for (int i = NPC_START; i < NUM_OF_NPC; ++i)
	{
		if (true == Is_Close(ci, i)) new_view_list.insert(i);
		if (Is_Close(ci, i) && Is_Attcked(ci, i)) continue;
	}

	std::unordered_set<int> vlc;
	g_clients[ci].vl_lock.lock();
	vlc = g_clients[ci].view_list;
	g_clients[ci].vl_lock.unlock();

	for (auto target : new_view_list)
		if (0 == vlc.count(target)) {
			SendPutPlayerPacket(ci, target);
			vlc.insert(target);
			if (true == Is_NPC(target)) {
				WakeUpNPC(target);
				continue;
			}
			g_clients[target].vl_lock.lock();
			if (0 != g_clients[target].view_list.count(ci)) {
				g_clients[target].vl_lock.unlock();
				SendPositionPacket(target, ci);
			}
			else
			{
				g_clients[target].view_list.insert(ci);
				g_clients[target].vl_lock.unlock();
				SendPutPlayerPacket(target, ci);
			}
		}
		else {
			if (true == Is_NPC(target))continue;
			g_clients[target].vl_lock.lock();
			if (0 != g_clients[target].view_list.count(ci)) {
				g_clients[target].vl_lock.unlock();
				SendPositionPacket(target, ci);
			}
			else {
				g_clients[target].view_list.insert(ci);
				g_clients[target].vl_lock.unlock();
				SendPutPlayerPacket(target, ci);
			}
		}

		std::unordered_set<int> removed_id_list;
		for (auto target : vlc)
			if (0 == new_view_list.count(target)) {
				SendRemovePlayerPacket(ci, target);
				removed_id_list.insert(target);
				if (true == Is_NPC(target)) continue;
				g_clients[target].vl_lock.lock();
				if (0 != g_clients[target].view_list.count(ci)) {
					g_clients[target].view_list.erase(ci);
					g_clients[target].vl_lock.unlock();
					SendRemovePlayerPacket(target, ci);
				}
				else g_clients[target].vl_lock.unlock();
			}

		g_clients[ci].vl_lock.lock();
		for (auto p : vlc)
			g_clients[ci].view_list.insert(p);
		for (auto p : removed_id_list)
			g_clients[ci].view_list.erase(p);
		g_clients[ci].vl_lock.unlock();

		for (auto npc : new_view_list) {
			if (false == Is_NPC(npc)) continue;
			OverlappedEx *over_ex = new OverlappedEx;
			over_ex->event_target = ci;
			over_ex->event_type = OP_PLAYER_MOVE_NOTIFY;
			PostQueuedCompletionStatus(g_hiocp, 1, npc, &over_ex->over);
		}



}

void DisconnectClient(int ci)
{
	closesocket(g_clients[ci].client_socket);
	g_clients[ci].connect = false;
	std::unordered_set<int> lvl;

	g_clients[ci].vl_lock.lock();
	lvl = g_clients[ci].view_list;
	g_clients[ci].vl_lock.unlock();

	for (auto target : lvl)
	{
		g_clients[target].vl_lock.lock();
		if (0 != g_clients[target].view_list.count(ci)) {
			g_clients[target].vl_lock.unlock();
			SendRemovePlayerPacket(target, ci);
		}
		else g_clients[target].vl_lock.unlock();
	}
	g_clients[ci].vl_lock.lock();
	g_clients[ci].view_list.clear();
	g_clients[ci].vl_lock.unlock();
}

void NPC_Attack(int id)
{
	for (int i = 0; i < MAX_USER; ++i)
	{
		if (g_clients[i].connect && Is_Attcked(i, id)) {
			if (g_clients[i].hp > 0)
				g_clients[i].hp -= 10 * g_clients[id].monster_level;

			if (g_clients[i].hp >= 100)
				g_clients[i].hp = 0;

			SendStatePacket(i, i);
			if (g_clients[i].hp <= 0) {
				for (int j = 0; j < MAX_USER; ++j)
					if (g_clients[j].connect) {
						g_clients[j].die = true;
						Timer_Event t = { i, high_resolution_clock::now() + 2s, E_REVIVAL };
						tq_lock.lock();
						timer_queue.push(t);
						tq_lock.unlock();
						SendRemovePlayerPacket(j, i);
					}
			}
		}
	}
}

void NPC_Random_Move(int id)
{
	//	for (int i = 0; i < 10000; ++i) sum = sum + i;

	for (int i = 0; i < MAX_USER; ++i)
		if (g_clients[i].connect && Is_Close(id, i)) {
			g_clients[id].trac_client = i;
			break;
		}

	int x = g_clients[id].x;
	int y = g_clients[id].y;
	int ci = g_clients[id].trac_client;

	if (-1 != g_clients[id].trac_client && !Is_Attcked(ci, id) && g_clients[id].trac) {

		if (0 == g_clients[id].turn) {
			if (g_clients[ci].x > x) x++;
			else  x--;
			g_clients[id].turn = 1;
		}
		else {
			if (g_clients[ci].y > y) y++;
			else  y--;
			g_clients[id].turn = 0;
		}

	}

	else if (g_clients[id].trac)
	{
		switch ((rand() % 4) + 1) {
		case LEFT: if (x > 0 && BG != g_map[x - 1][y] && VILLIGE_LINE != g_map[x - 1][y] && S1_LINE != g_map[x - 1][y])x--; break;
		case UP: if (y > 0 && BG != g_map[x][y - 1]) y--; break;
		case RIGHT: if (x < BOARD_WIDTH - 2 && BG != g_map[x + 1][y] && VILLIGE_LINE != g_map[x + 1][y] && S1_LINE != g_map[x + 1][y]) x++; break;
		case DOWN: if (y < BOARD_HEIGHT - 2 && BG != g_map[x][y + 1]) y++; break;
		}
	}

	g_clients[id].x = x;
	g_clients[id].y = y;


	NPC_Attack(id);

	for (int i = 0; i < MAX_USER; ++i) {
		if (true == g_clients[i].connect) {
			g_clients[i].vl_lock.lock();
			if (0 != g_clients[i].view_list.count(id)) {
				if (true == Is_Close(i, id)) {
					g_clients[i].vl_lock.unlock();
					SendPositionPacket(i, id);
				}
				else {
					g_clients[i].view_list.erase(id);
					g_clients[i].vl_lock.unlock();
					SendRemovePlayerPacket(i, id);
				}
			}
			else {
				if (true == Is_Close(i, id)) {
					g_clients[i].view_list.insert(id);
					g_clients[i].vl_lock.unlock();
					SendPutPlayerPacket(i, id);
				}
				else {
					g_clients[i].vl_lock.unlock();
				}
			}
		}
	}

	for (int i = 0; i < MAX_USER; ++i) {
		if ((true == g_clients[i].connect) && (Is_Close(i, id))) {
			Timer_Event t = { id, high_resolution_clock::now() + 1s, E_MOVE };
			tq_lock.lock();
			timer_queue.push(t);
			tq_lock.unlock();
			return;
		}
	}
	g_clients[id].is_active = false;
}


void SavePositionToDB(int ci) {
	// Connect to data source  
	retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015182002", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);

	// Allocate statement handle  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
		//�̸� �� hp ��ġ ���ݷ� �Ѱ���ġ x,y
		sprintf(g_savebuf, "EXEC dbo.save_state %s, %d, %d, %d,%d,%d,%d,%d",
			g_clients[ci].id, g_clients[ci].level, g_clients[ci].hp, g_clients[ci].exp,
			g_clients[ci].attack, g_clients[ci].exp_sum, g_clients[ci].x, g_clients[ci].y);

		MultiByteToWideChar(CP_UTF8, 0, g_savebuf, strlen(g_savebuf), g_sqldata, sizeof g_sqldata / sizeof *g_sqldata);
		g_sqldata[strlen(g_savebuf)] = '\0';

		retcode = SQLExecDirect(hstmt, g_sqldata, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

			// Bind columns 1, 2, and 3  
			//int db_x_pos, db_y_pos, db_lv, db_exp, db_dxps,db_attack;
			retcode = SQLBindCol(hstmt, 1, SQL_WCHAR, dbID, NAME_LEN, &S_ID);
			retcode = SQLBindCol(hstmt, 2, SQL_INTEGER, &db_lv, POS_LEN, &S_lv);
			retcode = SQLBindCol(hstmt, 3, SQL_INTEGER, &db_hp, POS_LEN, &S_hp);
			retcode = SQLBindCol(hstmt, 4, SQL_INTEGER, &db_exp, POS_LEN, &S_exp);
			retcode = SQLBindCol(hstmt, 5, SQL_INTEGER, &db_attack, POS_LEN, &S_att);
			retcode = SQLBindCol(hstmt, 6, SQL_INTEGER, &db_exps, POS_LEN, &S_exps);
			retcode = SQLBindCol(hstmt, 7, SQL_INTEGER, &db_x_pos, POS_LEN, &S_xPos);
			retcode = SQLBindCol(hstmt, 8, SQL_INTEGER, &db_y_pos, POS_LEN, &S_yPos);

			// Fetch and print each row of data. On an error, display a message and exit.  

			retcode = SQLFetch(hstmt);
			if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
				show_error();

			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				printf("ID : %s\n", dbID);
			}
		}
	}
	SQLDisconnect(hdbc);
}


void DB_thread()
{
	DWORD StartTime = GetTickCount();
	while (true) {
		if (GetTickCount() - StartTime > 2500) {
			for (int i = 0; i < MAX_USER; ++i) {
				if (false == g_clients[i].connect)
					continue;
				SavePositionToDB(i);
			}
			StartTime = GetTickCount();
		}
	}
}

void Worker_Thread()
{
	while (true) {
		DWORD io_size;
		unsigned long long ci;
		OverlappedEx *over;
		BOOL ret = GetQueuedCompletionStatus(g_hiocp, &io_size, &ci,
			reinterpret_cast<LPWSAOVERLAPPED *>(&over), INFINITE);
		//		std::cout << "GQCS :";
		if (FALSE == ret) {
			int err_no = WSAGetLastError();
			if (64 == err_no)
				DisconnectClient(ci);
			error_display("GQCS : ", WSAGetLastError());
			//			std::cout << "Error in GQCS\n";
		}
		if (0 == io_size) {
			DisconnectClient(ci);
			continue;
		}
		if (OP_RECV == over->event_type) {
			//std::cout << "RECV from Client :" << ci;
			//std::cout << "  IO_SIZE : " << io_size << std::endl;
			unsigned char *buf = g_clients[ci].recv_over.IOCP_buf;
			unsigned psize = g_clients[ci].curr_packet_size;
			unsigned pr_size = g_clients[ci].prev_packet_data;
			while (io_size > 0) {
				if (0 == psize) psize = buf[0];
				if (io_size + pr_size >= psize) {
					unsigned char packet[MAX_PACKET_SIZE];
					memcpy(packet, g_clients[ci].packet_buf, pr_size);
					memcpy(packet + pr_size, buf, psize - pr_size);
					ProcessPacket(static_cast<int>(ci), packet);
					io_size -= psize - pr_size;
					buf += psize - pr_size;
					psize = 0; pr_size = 0;
				}
				else {
					memcpy(g_clients[ci].packet_buf + pr_size, buf, io_size);
					pr_size += io_size;
					io_size = 0;
				}
			}
			g_clients[ci].curr_packet_size = psize;
			g_clients[ci].prev_packet_data = pr_size;
			DWORD recv_flag = 0;
			WSARecv(g_clients[ci].client_socket,
				&g_clients[ci].recv_over.wsabuf, 1,
				NULL, &recv_flag, &g_clients[ci].recv_over.over, NULL);
		}
		else if (OP_SEND == over->event_type) {
			if (io_size != over->wsabuf.len) {
				std::cout << "Send Incomplete Error!\n";
				closesocket(g_clients[ci].client_socket);
				g_clients[ci].connect = false;
			}
			delete over;
		}
		else if (OP_DO_AI == over->event_type) {
			NPC_Random_Move(ci);
			delete over;
		}
		else if (OP_PLAYER_MOVE_NOTIFY == over->event_type)
		{
			lua_State * L = g_clients[ci].L;
			lua_getglobal(L, "player_move_notify");
			lua_pushnumber(L, over->event_target);

			if (0 != lua_pcall(L, 1, 0, 0))
			{
				printf("LUA Error : player_move_notify; %s\n", lua_tostring(L, -1));
				lua_close(L);
			}

			delete over;
		}
		else if (OP_RESPAWN == over->event_type)
		{
			//std::cout << " RESP" << std::endl;
			NPC_Respawn(ci);
			delete over;
		}
		else if (OP_REVIVAL == over->event_type) {
			Player_Respawn(ci);
			delete over;
		}
		else {
			std::cout << "Unknown GQCS event!\n";
			while (true);
		}
	}
}

void Accept_Thread()
{
	while (true) {
		SOCKADDR_IN ClientAddr;
		ZeroMemory(&ClientAddr, sizeof(SOCKADDR_IN));
		ClientAddr.sin_family = AF_INET;
		ClientAddr.sin_port = htons(MY_SERVER_PORT);
		ClientAddr.sin_addr.s_addr = INADDR_ANY;
		int addr_size = sizeof(ClientAddr);
		SOCKET new_client = WSAAccept(g_ssocket,
			reinterpret_cast<sockaddr *>(&ClientAddr), &addr_size, NULL, NULL);

		int new_id = -1;
		for (int i = 0; i < MAX_USER; ++i)
			if (g_clients[i].connect == false) { new_id = i; break; }
		if (-1 == new_id) { std::cout << "MAX USER OVERFLOW!\n"; closesocket(new_client);  continue; }

		////////
		char client_id[10];
		char id_buf[10];
		int retval = recv(new_client, id_buf, sizeof(id_buf), 0);
		if (retval == SOCKET_ERROR) {
			cout << "recv Error\n";
		}
		id_buf[retval - 1] = '\0';
		strcpy(client_id, id_buf);
		cout << client_id << endl;

		g_clients[new_id].connect = true;
		g_clients[new_id].client_socket = new_client;
		g_clients[new_id].curr_packet_size = 0;
		g_clients[new_id].prev_packet_data = 0;
		ZeroMemory(&g_clients[new_id].recv_over, sizeof(g_clients[new_id].recv_over));
		g_clients[new_id].recv_over.event_type = OP_RECV;
		g_clients[new_id].recv_over.wsabuf.buf =
			reinterpret_cast<CHAR *>(g_clients[new_id].recv_over.IOCP_buf);
		g_clients[new_id].recv_over.wsabuf.len = sizeof(g_clients[new_id].recv_over.IOCP_buf);
//		g_clients[new_id].x = 4;
//		g_clients[new_id].y = 4;
//		g_clients[new_id].level = 1;
//		g_clients[new_id].hp = 100;
		g_clients[new_id].die = false;
//		g_clients[new_id].attack = 50;
		g_clients[new_id].maxhp = 100;
		//////////
		// Connect to data source  
		retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015182002", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
		// Allocate statement handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
			sprintf(g_savebuf, "EXEC dbo.load_state %s", client_id);
			MultiByteToWideChar(CP_UTF8, 0, g_savebuf, strlen(g_savebuf), g_sqldata, sizeof g_sqldata / sizeof *g_sqldata);
			g_sqldata[strlen(g_savebuf)] = '\0';
			//Execute a SQL statement
			retcode = SQLExecDirect(hstmt, g_sqldata, SQL_NTS);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				// Bind columns 1, 2, and 3  
		//�̸� �� hp ��ġ ���ݷ� �Ѱ���ġ x,y
				retcode = SQLBindCol(hstmt, 1, SQL_WCHAR, ID, NAME_LEN, &LID);
				retcode = SQLBindCol(hstmt, 2, SQL_INTEGER, &iLv, POS_LEN, &L_lv);
				retcode = SQLBindCol(hstmt, 3, SQL_INTEGER, &ihp, POS_LEN, &L_hp);
				retcode = SQLBindCol(hstmt, 4, SQL_INTEGER, &iExp, POS_LEN, &L_exp);
				retcode = SQLBindCol(hstmt, 5, SQL_INTEGER, &iAtt, POS_LEN, &L_att);
				retcode = SQLBindCol(hstmt, 6, SQL_INTEGER, &iExps, POS_LEN, &L_exps);
				retcode = SQLBindCol(hstmt, 7, SQL_INTEGER, &xPos, POS_LEN, &LxPos);
				retcode = SQLBindCol(hstmt, 8, SQL_INTEGER, &yPos, POS_LEN, &LyPos);
				// Fetch and print each row of data. On an error, display a message and exit.  
				retcode = SQLFetch(hstmt);
				if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
					show_error();
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					wcout << "���� : " << ID << endl;
				}
				else {
					cout << "No Data \n";
					SendDisconnectedPacket(new_id, new_id);
					g_clients[new_id].connect = false;
					closesocket(new_client);
					SQLDisconnect(hdbc);
					continue;
				}
			}
			// Process data  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLCancel(hstmt);
				SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
			}
			SQLDisconnect(hdbc);
		}
		g_clients[new_id].x = xPos;
		g_clients[new_id].y = yPos;
		g_clients[new_id].attack = iAtt;
		g_clients[new_id].hp = ihp;
		g_clients[new_id].exp = iExp;
		g_clients[new_id].exp_sum = iExps;
		g_clients[new_id].level = iLv;
		strcpy(g_clients[new_id].id, client_id);




		DWORD recv_flag = 0;
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), g_hiocp, new_id, 0);
		WSARecv(new_client, &g_clients[new_id].recv_over.wsabuf, 1,
			NULL, &recv_flag, &g_clients[new_id].recv_over.over, NULL);


		SendPutPlayerPacket(new_id, new_id);

		std::unordered_set<int> local_view_list;

		for (int i = 0; i < MAX_USER; ++i)
		{
			if (g_clients[i].connect == true)
			{
				if (new_id != i)
				{
					if (true == Is_Close(i, new_id)) {
						SendPutPlayerPacket(new_id, i);
						local_view_list.insert(i);
						SendPutPlayerPacket(i, new_id);
						g_clients[new_id].view_list.insert(i);
						g_clients[i].vl_lock.lock();
						g_clients[i].view_list.insert(new_id);
						g_clients[i].vl_lock.unlock();
					}
				}
			}

			for (int i = NPC_START; i < NUM_OF_NPC; ++i) {
				if (true == Is_Close(new_id, i))
				{
					WakeUpNPC(i);
					local_view_list.insert(i);
					SendPutPlayerPacket(new_id, i);
				}
			}

			g_clients[new_id].vl_lock.lock();
			for (auto p : local_view_list) g_clients[new_id].view_list.insert(p);
			g_clients[new_id].vl_lock.unlock();
		}
	}
}

void NPC_AI_Thread()
{
	for (;;) {
		auto now = high_resolution_clock::now();
		for (int i = NPC_START; i < NUM_OF_NPC; ++i) {
			for (int j = 0; j < MAX_USER; ++j) {
				if (Is_Close(j, i)) {
					OverlappedEx *over = new OverlappedEx;
					over->event_type = OP_DO_AI;
					PostQueuedCompletionStatus(g_hiocp, 1, i, &over->over);
					break;
				}
			}
		}
		auto du = high_resolution_clock::now() - now;
		int du_s = duration_cast<milliseconds>(du).count();
		if (1000 > du_s) Sleep(1000 - du_s);
	}
}

void Timer_Thread()
{
	for (;;) {
		Sleep(10);
		for (;;) {
			if (high_resolution_clock::now() - g_time >= 1s)
			{
				Check_Player_State();
				g_time = high_resolution_clock::now();
			}
			tq_lock.lock();
			if (0 == timer_queue.size()) {
				tq_lock.unlock();
				break;
			}
			Timer_Event t = timer_queue.top();
			if (t.exec_time > high_resolution_clock::now()) {
				tq_lock.unlock();
				break;
			}
			timer_queue.pop();
			tq_lock.unlock();
			OverlappedEx *over = new OverlappedEx;
			if (E_MOVE == t.event)
				over->event_type = OP_DO_AI;
			if (E_RESPAWN == t.event)
				over->event_type = OP_RESPAWN;
			if (E_REVIVAL == t.event)
				over->event_type = OP_REVIVAL;
			PostQueuedCompletionStatus(g_hiocp, 1, t.object_id, &over->over);
		}
	}
}

void Shutdown_Server()
{
	closesocket(g_ssocket);
	CloseHandle(g_hiocp);
	WSACleanup();
}

int main()
{
	std::vector <std::thread *> worker_threads;
	g_time = high_resolution_clock::now();
	Initialize_Server();
	Initialize_DB();
	for (int i = 0; i < 6; ++i)
		worker_threads.push_back(new std::thread{ Worker_Thread });

	//std::thread ai_thread{ NPC_AI_Thread };
	std::thread db_thread{ DB_thread };

	std::thread timer_thread{ Timer_Thread };

	std::thread accept_thread{ Accept_Thread };

	accept_thread.join();

	timer_thread.join();
	for (auto pth : worker_threads) {
		pth->join();
		delete pth;
	}
	Shutdown_Server();
}