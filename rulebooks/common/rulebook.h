/*
 * The rulebook interface of GameMaster 1.0.0, as its Developer's Kit documents
 * it: the game event record and its events. Every ParlorNet game rulebook uses
 * it through rb.c.
 */
#ifndef RULEBOOK_H
#define RULEBOOK_H

#include <Types.h>

enum {
	ge_InitRuleBook, ge_FinishRuleBook,
	ge_NewGame, ge_OldGame, ge_Swap, ge_Restart, ge_Close,
	ge_Activate, ge_Deactivate, ge_UpdateMenus, ge_Menu,
	ge_MouseDown, ge_KeyDown, ge_Idle, ge_SleepQuery,
	ge_ConnectionMade, ge_MessageReceived, ge_ConnectionLost,
	ge_Ask, ge_Answer, ge_Error, ge_SendMessage,
	ge_Flatten
};

/* The game event record, laid out as Pascal lays it out (342 bytes). */
typedef struct GameEvent {
	unsigned char event;        /* 0 */
	unsigned char pad0;
	Handle game;                /* 2 */
	Handle globals;             /* 6 */
	Point where;                /* 10 */
	short modifiers;            /* 14 */
	unsigned char message[256]; /* 16 */
	unsigned char but1[8];      /* 272 */
	unsigned char but2[8];      /* 280 */
	short int1, int2;           /* 288, 290 */
	long long1;                 /* 292 */
	Boolean modified;           /* 296 */
	Boolean myturn;             /* 297 */
	short version;              /* 298 */
	Boolean dialoging;          /* 300 */
	unsigned char pad1;
	long dumspace[10];          /* 302 */
} GameEvent;

#endif
