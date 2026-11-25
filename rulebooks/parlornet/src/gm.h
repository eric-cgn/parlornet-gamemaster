/*
 * GameMaster 1.0.0 as a rulebook sees it.
 *
 * The rulebook interface (the game event record and its events) is the one
 * GameMaster's Developer's Kit documents. The rest is GameMaster's own code,
 * read from the program: its connection layer (CODE 2), which every network
 * call from the rest of the program goes through by the jump table, and the
 * two routines of CODE 3 the lobby uses to start a game. The rulebook checks
 * CODE 2 and CODE 3 against the CRC-32s below before it uses any of this.
 */
#ifndef GM_H
#define GM_H

#include <Types.h>

/* ---- the rulebook interface ---------------------------------------------------------------------- */

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

/* ---- the connection layer (CODE 2) ---------------------------------------------------------------- */

/* A connection: 0x118 bytes, starting with the Pascal string "GMMG". Kind 1 is MacTCP, 2 is AppleTalk
   (ADSP); the rulebook's own connections are kind 3. */
typedef struct GMConn {
	unsigned char magic[6];     /* 0: "\pGMMG" */
	unsigned char name[256];    /* 6: the other side, as shown */
	Ptr data;                   /* 0x106: the game window it belongs to, or 0 for a call being answered */
	unsigned char kind;         /* 0x10A */
	unsigned char pad;
	long link;                  /* 0x10C */
	long stream;                /* 0x110 */
	long extra;                 /* 0x114 */
} GMConn;

enum { kGMConnSize = 0x118, kKindTCP = 1, kKindADSP = 2, kKindParlor = 3 };

/* An event of the connection layer, as GETGMCEVENT returns it. */
typedef struct GMCEvent {
	unsigned char kind;         /* 0: below */
	unsigned char pad;
	Ptr data;                   /* 2: the connection's window (0: the call answerer's) */
	long count;                 /* 6 */
	GMConn *conn;               /* 0xA */
	unsigned char name[256];    /* 0xE */
} GMCEvent;

enum { kEvConnected = 1, kEvClosed = 2, kEvData = 3, kEvFailed = 4, kEvLost = 5 };

/* A game window is a dialog record that GameMaster extends with its own fields. */
enum {
	kWinRulebook = 0xB0,        /* the rulebook's node */
	kWinConn = 0xBC,            /* its connection */
	kNodeID = 0x140             /* in a rulebook node (a copy of its GMRZ): the ID string */
};

/* The jump-table entries (offsets from A5 of each JMP) and where they go in their segment. */
enum {
	kJTSendData = 0x45A, kJTCollectData = 0x462, kJTPlaceCall = 0x46A, kJTGetEvent = 0x472,
	kJTClose = 0x47A, kJTSetData = 0x482, kJTOpenActiveSpecial = 0x492,
	kJTCreateList = 0x2A, kJTReturnHead = 0x3A, kJTMoveToNext = 0x42, kJTFetch = 0x72, kJTIsTail = 0x7A
};

enum {
	kC2SendData = 0x31EC, kC2Size = 15522,
	kC3CreateList = 0x174, kC3BringUpNewGame = 0x1074, kC3MenuConnect = 0x1456, kC3Size = 15864
};

#define kC2CRC 0xB9FF521CUL
#define kC3CRC 0xC6720B21UL

/* A5-relative globals of GameMaster's that the lobby uses. */
enum { kA5Rulebooks = -0xC08, kA5NewWhere = -0xB8C };

#endif
