/*
 * The tunnel.
 *
 * GameMaster reaches the network only through its connection layer in CODE 2,
 * and the rest of the program calls that layer by the jump table. While the
 * lobby is open, seven of those entries point here. A connection of kind 3
 * (the rulebook's own) is carried over the parlor connection; every other
 * connection goes to GameMaster's own routine, so calls over MacTCP and
 * AppleTalk work as before.
 *
 * On the parlor connection a tunneled connection is a number, 1 to 8:
 *   to the parlor:   GMO c who     a game call (who: the opponent asked for)
 *                    GMS c k       the game connection for call k
 *                    GMD c hex     bytes
 *                    GMX c         closed
 *   from the parlor: GMK c         the parlor took connection c
 *                    GMD c hex     bytes
 *                    GMX c         closed
 * The parlor answers a tunneled call with the address 0.0.0.0 and the port
 * 0x504E0000 plus the call's number, so the game connection that GameMaster
 * then opens is recognized and tunneled too.
 */
#include <Memory.h>
#include <OSUtils.h>
#include <Events.h>
#include <Windows.h>
#include <Traps.h>
#include <string.h>

#include "gm.h"
#include "tcp.h"
#include "tunnel.h"

enum { kSlots = 8, kBufSize = 1024, kHexPerLine = 36 };

typedef struct {
	GMConn *gm;                 /* the record GameMaster holds; NULL when the slot is free */
	Ptr buf;                    /* bytes from the parlor not yet read by GameMaster */
	short len;
	Boolean acked;              /* GMK came */
	Boolean eof;                /* GMX came, or the parlor connection ended */
	Boolean saidConnected, saidClosed, dataTold;
	Boolean closedHere;         /* GMX sent */
} Slot;

static Slot gSlots[kSlots];
static Boolean gInstalled, gChecked, gGood;
static long gOrig[7];
static Ptr gC2, gC3;            /* where CODE 2 and CODE 3 start in memory */
static Boolean gArmed;
static char gArmedWho[16];
static unsigned char gStartID[64];
static char gStartWho[16];
static Boolean gStartPending;
static const unsigned char *gAskIDs[16];
static short gAskN;
static Boolean gAskPending;
static unsigned short gHave = 0xFFFF;

static Ptr A5(void)
{
	return *(Ptr *)0x904;       /* CurrentA5 */
}

/* ---- checking GameMaster ---------------------------------------------------------------------------- */

static unsigned long Crc32(const unsigned char *p, long n)
{
	unsigned long crc = 0xFFFFFFFFUL;
	short k;

	while (n-- > 0) {
		crc ^= *p++;
		for (k = 0; k < 8; k++)
			crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
	}
	return ~crc;
}

/* The segment that the jump-table entry at A5+off jumps into, at offset inSeg: NULL if that entry is not a
   loaded JMP. */
static Ptr SegmentOf(short off, long inSeg)
{
	Ptr e = A5() + off;

	if (*(unsigned short *)e != 0x4EF9)
		return NULL;
	return *(Ptr *)(e + 2) - inSeg - 4;
}

Boolean TunnelCheck(void)
{
	if (gChecked)
		return gGood;
	gChecked = true;
	gC2 = SegmentOf(kJTSendData, kC2SendData);
	gC3 = SegmentOf(kJTCreateList, kC3CreateList);
	gGood = gC2 && gC3 && Crc32((unsigned char *)gC2, kC2Size) == kC2CRC
		&& Crc32((unsigned char *)gC3, kC3Size) == kC3CRC;
	return gGood;
}

/* ---- the slots ------------------------------------------------------------------------------------------ */

static Slot *SlotOf(GMConn *c)
{
	short i;

	if (!c || c->kind != kKindParlor)
		return NULL;
	for (i = 0; i < kSlots; i++)
		if (gSlots[i].gm == c)
			return &gSlots[i];
	return NULL;
}

static short NumberOf(Slot *s)
{
	return (short)(s - gSlots) + 1;
}

static Slot *NewSlot(Ptr data)
{
	short i;
	Slot *s;
	GMConn *c;

	for (i = 0; i < kSlots && gSlots[i].gm; i++)
		;
	if (i == kSlots)
		return NULL;
	s = &gSlots[i];
	c = (GMConn *)NewPtrClear(kGMConnSize);
	s->buf = NewPtr(kBufSize);
	if (!c || !s->buf) {
		if (c)
			DisposePtr((Ptr)c);
		if (s->buf)
			DisposePtr(s->buf);
		s->buf = NULL;
		return NULL;
	}
	memcpy(c->magic, "\004GMMG", 5);
	memcpy(c->name, "\011ParlorNet", 10);
	c->data = data;
	c->kind = kKindParlor;
	s->gm = c;
	s->len = 0;
	s->acked = s->eof = s->saidConnected = s->saidClosed = s->dataTold = s->closedHere = false;
	return s;
}

static void FreeSlot(Slot *s)
{
	if (s->buf)
		DisposePtr(s->buf);
	memset(s, 0, sizeof *s);
}

short TunnelGames(void)
{
	short i, n = 0;

	for (i = 0; i < kSlots; i++)
		if (gSlots[i].gm && gSlots[i].gm->data)
			n++;
	return n;
}

/* ---- the parlor connection ---------------------------------------------------------------------------- */

static char gIn[160];
static short gInLen;
static Boolean gPumping;

OSErr SendLine(const char *text)
{
	char out[90];
	short n = strlen(text);

	if (n > 86)
		n = 86;
	memcpy(out, text, n);
	out[n++] = '\r';
	out[n++] = '\n';
	return TcpSend(out, n);
}

static void Num(char *p, short v)
{
	char t[8];
	short n = 0;

	do {
		t[n++] = '0' + v % 10;
		v /= 10;
	} while (v);
	while (n)
		*p++ = t[--n];
	*p = 0;
}

static void Send3(const char *verb, short a, const char *rest)
{
	char line[64];

	strcpy(line, verb);
	Num(line + strlen(line), a);
	if (rest) {
		strcat(line, " ");
		strcat(line, rest);
	}
	SendLine(line);
}

static short HexVal(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/* A tunnel line: GMK c, GMD c hex, GMX c. */
static void TunnelLine(char *line)
{
	short n = line[4] - '0', hi, lo;
	Slot *s;
	char *p;

	if (n < 1 || n > kSlots || (line[5] != ' ' && line[5] != 0))
		return;
	s = &gSlots[n - 1];
	if (!s->gm)
		return;
	if (line[2] == 'K')
		s->acked = true;
	else if (line[2] == 'X')
		s->eof = true;
	else if (line[2] == 'D') {
		for (p = line + 6; (hi = HexVal(p[0])) >= 0 && (lo = HexVal(p[1])) >= 0; p += 2)
			if (s->len < kBufSize)
				s->buf[s->len++] = (char)(hi * 16 + lo);
		s->dataTold = false;
	}
}

static void DoLine(char *line)
{
	if (line[0] == 'G' && line[1] == 'M' && (line[2] == 'K' || line[2] == 'D' || line[2] == 'X') && line[3] == ' ')
		TunnelLine(line);
	else
		LobbyLine(line);
}

void TunnelReset(void)
{
	short i;

	for (i = 0; i < kSlots; i++)
		if (gSlots[i].gm)
			gSlots[i].eof = true;
}

void Pump(void)
{
	char buf[256];
	long got;
	short i;

	if (gPumping || !TcpIsOpen())
		return;
	gPumping = true;
	got = TcpRecv(buf, sizeof buf);
	if (got < 0) {
		TcpClose();
		TunnelReset();
		gInLen = 0;
		LobbyLost();
	}
	for (i = 0; i < got; i++) {
		char c = buf[i];
		if (c == '\n' || c == '\r') {
			if (gInLen) {
				gIn[gInLen] = 0;
				gInLen = 0;
				DoLine(gIn);
			}
		} else if (gInLen < (short)sizeof gIn - 1)
			gIn[gInLen++] = c;
	}
	gPumping = false;
}

/* ---- the hooks ---------------------------------------------------------------------------------------- */

typedef pascal short (*SendDataProc)(GMConn *, Ptr, long);
typedef pascal Boolean (*GetEventProc)(GMCEvent *);
typedef pascal short (*PlaceCallProc)(Ptr, GMConn **, Boolean *, StringPtr);
typedef pascal void (*CloseProc)(GMConn **);
typedef pascal void (*SetDataProc)(GMConn *, Ptr);
typedef pascal short (*OpenActiveProc)(GMConn **, Ptr, Ptr);
typedef pascal void (*BringUpProc)(Ptr, Ptr);
typedef pascal void (*MenuConnectProc)(Ptr);
typedef pascal void (*ReturnHeadProc)(long, long *);
typedef pascal void (*MoveToNextProc)(long *);
typedef pascal void (*FetchProc)(long, long *);
typedef pascal Boolean (*IsTailProc)(long);

enum { hSend, hCollect, hPlace, hEvent, hClose, hSetData, hOpenActive };

static pascal short HSendData(GMConn *c, Ptr data, long n)
{
	static char out[1024];
	static const char hex[] = "0123456789abcdef";
	Slot *s = SlotOf(c);
	short used = 0, k, i;
	OSErr err = noErr;
	unsigned char *p = (unsigned char *)data;

	if (!s)
		return ((SendDataProc)gOrig[hSend])(c, data, n);
	if (s->eof || !TcpIsOpen())
		return -23008;
	while (n > 0 && err == noErr) {
		k = n > kHexPerLine ? kHexPerLine : (short)n;
		memcpy(out + used, "GMD ", 4);
		out[used + 4] = '0' + NumberOf(s);
		out[used + 5] = ' ';
		used += 6;
		for (i = 0; i < k; i++) {
			out[used++] = hex[*p >> 4];
			out[used++] = hex[*p++ & 15];
		}
		out[used++] = '\r';
		out[used++] = '\n';
		n -= k;
		if (used > (short)sizeof out - 90 || n == 0) {
			err = TcpSend(out, used);
			used = 0;
		}
	}
	return err;
}

static pascal short HCollectData(GMConn *c, Ptr data, long n)
{
	Slot *s = SlotOf(c);
	unsigned long until = TickCount() + 60 * 30;

	if (!s)
		return ((SendDataProc)gOrig[hCollect])(c, data, n);
	while (s->len < n && !s->eof && TickCount() < until) {
		Pump();
		SystemTask();
	}
	if (s->len < n) {
		memset(data, 0, n);
		return -23008;
	}
	memcpy(data, s->buf, n);
	s->len -= n;
	if (s->len)
		memmove(s->buf, s->buf + n, s->len);
	s->dataTold = false;
	return noErr;
}

static pascal short HPlaceCall(Ptr win, GMConn **conn, Boolean *ok, StringPtr name)
{
	Slot *s;

	if (!gArmed)
		return ((PlaceCallProc)gOrig[hPlace])(win, conn, ok, name);
	gArmed = false;
	*conn = NULL;
	*ok = false;
	if (!LobbySignedIn() || !(s = NewSlot(win)))
		return -108;
	Send3("GMO ", NumberOf(s), gArmedWho);
	*conn = s->gm;
	*ok = true;
	memcpy(name, s->gm->name, s->gm->name[0] + 1);
	return noErr;
}

static void StartPending(void);
static void FindInstalled(void);
static Ptr FindRulebook(const unsigned char *id);

static pascal Boolean HGetEvent(GMCEvent *ev)
{
	short i;
	Slot *s;

	Pump();
	if (gStartPending)
		StartPending();
	if (gAskPending)
		FindInstalled();
	for (i = 0; i < kSlots; i++) {
		s = &gSlots[i];
		if (!s->gm || s->saidClosed)
			continue;
		if (s->acked && !s->saidConnected) {
			s->saidConnected = true;
			ev->kind = kEvConnected;
		} else if (s->saidConnected && s->len && !s->dataTold) {
			s->dataTold = true;
			ev->kind = kEvData;
		} else if (s->eof && !s->len) {
			s->saidClosed = true;
			ev->kind = s->saidConnected ? kEvClosed : kEvFailed;
		} else
			continue;
		ev->data = s->gm->data;
		ev->count = s->len;
		ev->conn = s->gm;
		memcpy(ev->name, s->gm->name, s->gm->name[0] + 1);
		return true;
	}
	return ((GetEventProc)gOrig[hEvent])(ev);
}

static pascal void HClose(GMConn **conn)
{
	GMConn *c = *conn;
	Slot *s;

	if (!c || c->kind != kKindParlor) {
		((CloseProc)gOrig[hClose])(conn);
		return;
	}
	s = SlotOf(c);
	if (s) {
		if (!s->eof && TcpIsOpen())
			Send3("GMX ", NumberOf(s), NULL);
		FreeSlot(s);
	}
	DisposePtr((Ptr)c);
	*conn = NULL;
}

static pascal void HSetData(GMConn *c, Ptr data)
{
	if (c && c->kind == kKindParlor)
		c->data = data;
	else
		((SetDataProc)gOrig[hSetData])(c, data);
}

static pascal short HOpenActive(GMConn **conn, Ptr addr, Ptr data)
{
	unsigned long ip = *(unsigned long *)(addr + 2), port = *(unsigned long *)(addr + 6);
	short call = port & 0xFFFF;
	Slot *s;
	char k[4];

	if (addr[0] != kKindTCP || ip != 0 || (port >> 16) != 0x504E)
		return ((OpenActiveProc)gOrig[hOpenActive])(conn, addr, data);
	*conn = NULL;
	if (!TcpIsOpen() || call < 1 || call > kSlots || !(s = NewSlot(data)))
		return -108;
	Num(k, call);
	Send3("GMS ", NumberOf(s), k);
	*conn = s->gm;
	return noErr;
}

static const short gEntries[7] = {
	kJTSendData, kJTCollectData, kJTPlaceCall, kJTGetEvent, kJTClose, kJTSetData, kJTOpenActiveSpecial
};

static void FlushCaches(void)
{
	/* 68020 and later keep instructions in a cache: the jump table is code */
	if (NGetTrapAddress(0xA198, kOSTrapType) != NGetTrapAddress(0xA89F, kToolboxTrapType)) {
		register long sel __asm__("d0") = 1;
		__asm__ volatile ("dc.w 0xA198" : "+d"(sel) : : "a0", "a1", "d1", "d2", "memory");
	}
}

Boolean TunnelInstall(void)
{
	long hooks[7];
	short i;

	if (gInstalled)
		return true;
	if (!TunnelCheck())
		return false;
	hooks[hSend] = (long)HSendData;
	hooks[hCollect] = (long)HCollectData;
	hooks[hPlace] = (long)HPlaceCall;
	hooks[hEvent] = (long)HGetEvent;
	hooks[hClose] = (long)HClose;
	hooks[hSetData] = (long)HSetData;
	hooks[hOpenActive] = (long)HOpenActive;
	for (i = 0; i < 7; i++)
		if (*(unsigned short *)(A5() + gEntries[i]) != 0x4EF9)
			return false;
	for (i = 0; i < 7; i++) {
		long *target = (long *)(A5() + gEntries[i] + 2);
		gOrig[i] = *target;
		*target = hooks[i];
	}
	FlushCaches();
	gInstalled = true;
	return true;
}

void TunnelRemove(void)
{
	short i;

	if (!gInstalled)
		return;
	for (i = 0; i < 7; i++)
		*(long *)(A5() + gEntries[i] + 2) = gOrig[i];
	FlushCaches();
	gInstalled = false;
	/* Connections GameMaster still holds become inert: GameMaster's own routines ignore an unknown kind, and
	   its close disposes of the record. */
	for (i = 0; i < kSlots; i++)
		if (gSlots[i].gm) {
			gSlots[i].gm->kind = 0;
			FreeSlot(&gSlots[i]);
		}
}

/* ---- starting a game -------------------------------------------------------------------------------- */

void TunnelAskInstalled(const unsigned char *const *ids, short n)
{
	short i;

	gAskN = n > 16 ? 16 : n;
	for (i = 0; i < gAskN; i++)
		gAskIDs[i] = ids[i];
	gAskPending = true;
}

unsigned short TunnelInstalled(void)
{
	return gHave;
}

/* From the main loop (the GETGMCEVENT hook), as StartPending */
static void FindInstalled(void)
{
	short i;
	unsigned short have = 0;

	gAskPending = false;
	if (!gInstalled)
		return;
	for (i = 0; i < gAskN; i++)
		if (FindRulebook(gAskIDs[i]))
			have |= 1 << i;
	gHave = have;
}

void TunnelStart(const unsigned char *rulebookID, const char *opponent)
{
	memcpy(gStartID, rulebookID, rulebookID[0] + 1);
	strncpy(gStartWho, opponent, sizeof gStartWho - 1);
	gStartPending = true;
}

/* GameMaster's list of rulebooks: the node whose GMRZ ID string is id. */
static Ptr FindRulebook(const unsigned char *id)
{
	Ptr a5 = A5();
	long list = *(long *)(a5 + kA5Rulebooks), cursor = 0, node = 0;

	if (!list)
		return NULL;
	((ReturnHeadProc)(a5 + kJTReturnHead))(list, &cursor);
	while (!((IsTailProc)(a5 + kJTIsTail))(cursor)) {
		((FetchProc)(a5 + kJTFetch))(cursor, &node);
		if (node && memcmp((Ptr)node + kNodeID, id, id[0] + 1) == 0)
			return (Ptr)node;
		((MoveToNextProc)(a5 + kJTMoveToNext))(&cursor);
	}
	return NULL;
}

/* Run from the main loop (the GETGMCEVENT hook), never inside a rulebook call: New Game for the rulebook, then
   Connect on its window, which the PLACECALL hook turns into a tunneled call. */
static void StartPending(void)
{
	Ptr node, win;
	char text[80];

	gStartPending = false;
	if (!gInstalled || !LobbySignedIn())
		return;
	node = FindRulebook(gStartID);
	if (!node) {
		strcpy(text, "Put the ");
		strncat(text, (char *)gStartID + 1, gStartID[0]);
		strcat(text, " rulebook in GameMaster's folder first.");
		LobbyNote(text);
		return;
	}
	((BringUpProc)(gC3 + 4 + kC3BringUpNewGame))(node, A5() + kA5NewWhere);
	win = (Ptr)FrontWindow();
	if (!win || *(Ptr *)(win + kWinRulebook) != node)
		return;
	strcpy(gArmedWho, gStartWho);
	gArmed = true;
	((MenuConnectProc)(gC3 + 4 + kC3MenuConnect))(win);
	gArmed = false;
}
