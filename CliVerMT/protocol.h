#pragma once

#define	WM_SOCKET	WM_USER + 1

#define MY_SERVER_PORT  4000

#define MAX_BUFF_SIZE   4000
#define BUF_SIZE 4000

#define MAX_PACKET_SIZE  255

#define BOARD_WIDTH   5100
#define BOARD_HEIGHT  5100

#define MAX_USER 10
#define NPC_START  10
#define MAX_OBJECT_INDEX 30

#define VIEW_RADIUS 50

#define MAX_STR_SIZE  100

#define CS_UP		1
#define CS_DOWN		2
#define CS_LEFT		3
#define CS_RIGHT    4
#define CS_TELEPORT 5
#define CS_ATTACK	6
#define CS_SIGHT	7
#define CS_CHAT		8
#define CS_ANIM		9

#define SC_POS           1
#define SC_PUT_PLAYER    2
#define SC_REMOVE_PLAYER 3
#define SC_CHAT			4
#define SC_SCENE		5
#define SC_ANIM			6

enum PlantType {
	FieldTree,
	FieldFlower,
	FieldGrass,
	TownTree,
	TownFlower,
	TownGrass
};
enum FBXType {
	Spider,
	Human
};
enum SCENE
{
	TOWN = 0,
	FIELD = 1
};

enum player_animaition
{
	player_idle, front_run, back_run, left_run, right_run 
};

#define PI				3.141592
#define RADIAN			( PI / 180.0 )
#define DEGREE			( 180.0 / PI )
#define RAD2DEG(Rad)	( Rad * DEGREE )

#pragma pack (push, 1)
struct cs_packet_move {
	BYTE size;
	BYTE type;
	char dir_x;
	char dir_z;
};

struct cs_packet_sight {
	BYTE size;
	BYTE type;
	char dir_x;
	char dir_z;
};

struct cs_packet_attack
{
	BYTE size;
	BYTE type;
	WORD id;
};

struct cs_packet_chat {
	BYTE size;
	BYTE type;
	WCHAR message[MAX_STR_SIZE];
};

struct sc_packet_pos {
	BYTE size;
	BYTE type;
	WORD id;
	float x;
	float y;
	char sight_x;
	char sight_z;
};

struct sc_packet_put_player {
	BYTE size;
	BYTE type;
	WORD id;
	float x;
	float y;
	char sight_x;
	char sight_z;
};

struct sc_packet_remove_player {
	BYTE size;
	BYTE type;
	WORD id;
};

struct sc_packet_scene_player
{
	BYTE size;
	BYTE type;
	WORD id;
	BYTE scene;
};

struct sc_packet_chat {
	BYTE size;
	BYTE type;
	WORD id;
	WCHAR message[MAX_STR_SIZE];
};

struct cs_packet_animation {
	BYTE size;
	BYTE type;
	WORD id;
	char anim_num;
};
#pragma pack (pop)
