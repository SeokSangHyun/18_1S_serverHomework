#include<Windows.h>

#define MAX_BUFF_SIZE   4000
#define MAX_PACKET_SIZE  255

// npc°¡ ¸¹À¸¹Ç·Î ÁöÇüÀ» ³ÐÇôÁÜ
#define BOARD_WIDTH   600
#define BOARD_HEIGHT  400
//#define BOARD_WIDTH   100
//#define BOARD_HEIGHT  100

#define VIEW_RADIUS   10

#define MAX_USER 10	

#define NPC_START  1000
//#define NUM_OF_NPC  10000
// ¸Ê ³ÐÈ÷´Ï±î Àû¾îº¸ÀÌ´Ï npc¼ýÀÚ¸¦ ´Ã·ÁÁÜ
#define NUM_OF_NPC  2100

#define MY_SERVER_PORT  4000

#define MAX_STR_SIZE  100

#define MAP_SIZE 300

#define CS_UP    1
#define CS_DOWN  2
#define CS_LEFT  3
#define CS_RIGHT    4
#define CS_CHAT		5
#define CS_SKILL	6
#define CS_TP	7	
#define CS_STORE	8

#define SC_POS           1
#define SC_PUT_PLAYER    2
#define SC_REMOVE_PLAYER 3
#define SC_CHAT			 4
#define SC_STATE		5
#define SC_STORE		6
#define SC_LOGIN_FAILED 7
#pragma pack (push, 1)

enum AREA {
	BG = 20, VERTICALROAD = 1, HORIZEN_ROAD = 2, S1_LINE = 4, VILLIGE = 15,
	CEN_LINE = 14, VILLIGE_LINE = 19, BOSS_LINE = 18
};

struct cs_packet_up {
	BYTE size;
	BYTE type;
	BYTE zone;
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

struct cs_packet_chat {
	BYTE size;
	BYTE type;
	WCHAR message[MAX_STR_SIZE];
};

struct cs_packet_skill {
	BYTE size;
	BYTE type;
	BYTE value;
	BYTE damage;
};

struct cs_packet_store {
	int max_hp;
	int exp_sum;
	int attack;
};

struct sc_packet_pos {
	BYTE size;
	BYTE type;
	WORD id;
	int x;
	int y;
};

struct sc_packet_put_player {
	BYTE size;
	BYTE type;
	WORD id;
	int x;
	int y;
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

struct sc_pakcet_discon {
	BYTE size;
	BYTE type;
	WORD id;
};

struct sc_packet_state {
	BYTE size;
	BYTE type;
	WORD id;
	
	unsigned long long hp;
	int max_hp;
	
	BYTE level;
	
	unsigned long long exp;
	int expsum;

	BYTE attack;
};

#pragma pack (pop)



