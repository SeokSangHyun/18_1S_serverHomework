#pragma once

#define MAX_USER 1000
#define MAX_BUFF_SIZE 4000
#define MAX_PACKET_SIZE 255
#define SERVERPORT 9000

#define MAX_STR_SIZE  100
#define DEVIDE

#define CS_UP    1
#define CS_DOWN  2
#define CS_LEFT  3
#define CS_RIGHT    4
#define CS_CHAT		5

#define SC_POS           1
#define SC_PUT_PLAYER    2
#define SC_REMOVE_PLAYER 3
#define SC_CHAT			4

struct Packet
{
	float x;
	float y;

	short size_x;
	short size_y;

	char room[2];
	char dir;
};

/////
#pragma pack(push, 1)
struct Data
{
	float x;
	float y;

	short size_x;
	short size_y;

	char room[2];
	char dir;
};
struct Packet_t
{
	char size;
	char type;
	Data data;
};

/*   ==========   */
struct cs_packet_up {
	BYTE size;
	BYTE type;
};

struct cs_packet_down {
	BYTE size;
	BYTE type;
};

struct cs_packet_left {
	BYTE size;
	BYTE type;
};

struct cs_packet_right {
	BYTE size;
	BYTE type;
};

////
struct cs_packet_chat {
	BYTE size;
	BYTE type;
	WCHAR message[MAX_STR_SIZE];
};
////

struct sc_packet_pos {
	BYTE size;
	BYTE type;
	WORD id;
	BYTE x;
	BYTE y;
};

struct sc_packet_put_player {
	BYTE size;
	BYTE type;
	WORD id;
	BYTE x;
	BYTE y;
};
struct sc_packet_remove_player {
	BYTE size;
	BYTE type;
	WORD id;
};

struct sc_packet_chat {
	BYTE size;
	BYTE type;
	WORD id;
	WCHAR message[MAX_STR_SIZE];
};
#pragma pack(pop)
/////