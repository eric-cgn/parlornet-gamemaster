/*
 * The ParlorNet rulebook: a lobby for a ParlorNet parlor, inside GameMaster.
 *
 * GameMaster calls RULES (PROC 128) for each event of the rulebook's window. The
 * window has one user item, the play area, which this file draws and reads by
 * itself: the server, port and name fields, the sign-in questions the parlor
 * asks, and the lobby, in three tabs under the sign-in line and its Sign out
 * button:
 *
 *   New game     a game, an opponent (a pop-up menu: Wait for opponent, Bot,
 *                the players online, Other...), the button that starts it, and
 *                the invitations the player has, with Accept and Decline
 *   Your games   the player's games being played, with Resume for the games a
 *                rulebook here can take up half way (ParlorNet's own)
 *   All games    open tables where someone waits for a player, with Join, and
 *                other players' games
 *
 * While the window is open the tunnel's hooks are in GameMaster's jump table
 * (tunnel.c), and every game started from the lobby goes through the window's
 * one connection to the parlor.
 *
 * Plain 68000 code for System 6.0 and later, drawn in black and white for a
 * 512 by 342 screen.
 */
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Menus.h>
#include <Fonts.h>
#include <Memory.h>
#include <Events.h>
#include <ToolUtils.h>
#include <Retro68Runtime.h>
#include <string.h>

#include "gm.h"
#include "machine.h"
#include "tcp.h"
#include "tunnel.h"

#define kW 400
#define kH 256
#define kVersion "0.6"          /* the rulebook's version, as its 'vers' resource gives it (ParlorNet.r) */

enum { sOffline, sConnecting, sSigning, sAsking, sLobby };
enum { fHost, fPort, fName, fAnswer, kFields };
enum { bSignIn, bOK, bPlay, bAccept, bDecline, bSignOut, kButtons };
enum { tNew, tMine, tAll, kTabs };
enum { rMine, rWait, rPlay, kKinds };       /* the rows of TBL's answer: YG, WG, PG */
enum { oWait, oBot, oName };                /* the opponent chosen */
enum { kInvites = 4, kRows = 8, kWho = 16, kPollTicks = 5 * 60, kNoteY = 216, kNotes = 4, kPopID = 211,
	kMineShown = 6, kWaitShown = 3 };

typedef struct {
	char text[66];
	short max;
} Field;

static WindowPtr gWin;
static short gState;
static Field gFields[kFields];
static short gFocus;
static char gAskField[16], gAskLabel[48];
static Boolean gAskSecret;
static char gServer[16], gAuth[12], gMe[16];
static char gNotes[kNotes][72];
static Boolean gHooked;
static short gTab;

/* The lobby's requests to the parlor whose answers are still to come, oldest first: W (WHO), I (IVL), U (UNV),
   T (TBL). */
static char gReqs[8];
static short gReqN;
static unsigned long gPolled;

/* The games the lobby starts: the name shown, the parlor's game and rules, the GMRZ ID string of the rulebook
   that plays it, and whether that rulebook takes up a game half way (ParlorNet's own, from 1.1.0). */
typedef struct {
	const char *name;
	const char *key;
	const unsigned char *id;
	Boolean resumes;
} Game;

static const Game gGames[] = {
	{ "Othello", "reversi/1 standard", (const unsigned char *)"\015Othello [RHS]", false },
	{ "Dots&Boxes", "dotsandboxes/1 majority", (const unsigned char *)"\020Dots&Boxes [PNL]", false },
	{ "BattleShips", "navalcombat/1 salvo", (const unsigned char *)"\021BattleShips [RHS]", false },
	{ "Checkers", "checkers/1 american", (const unsigned char *)"\024Checkers [ParlorNet]", true },
	{ "Mancala", "mancala/1 kalah", (const unsigned char *)"\023Mancala [ParlorNet]", true },
	{ "Five in a Row", "fiveinarow/1 freestyle", (const unsigned char *)"\031Five in a Row [ParlorNet]", true },
	{ "Grid Words", "gridwords/1 standard", (const unsigned char *)"\026Grid Words [ParlorNet]", true },
	{ "Rummy Tile", "rummytile/1 standard", (const unsigned char *)"\026Rummy Tile [ParlorNet]", true },
	{ "Cannon Fire", "cannonfire/1 classic", (const unsigned char *)"\027Cannon Fire [ParlorNet]", true },
	{ "Backgammon", "backgammon/1 standard", (const unsigned char *)"\026Backgammon [ParlorNet]", true },
};
enum { kGames = sizeof gGames / sizeof gGames[0] };
static short gGame;

/* the opponent for New game */
static short gOpp = oBot;
static char gOppName[16];

/* the players online, from the last answer to WHO, without this player and the bots */
static char gWho[kWho][14], gNewWho[kWho][14];
static short gWhoN, gNewWhoN;

/* invitations to the player, to games the lobby starts */
typedef struct {
	char table[12];
	char from[16];
	short game;
} Invite;

static Invite gInvites[kInvites], gNewInvites[kInvites];
static short gInviteN, gNewInviteN;

/* the tables of TBL's answer */
typedef struct {
	char table[10];
	short game;
	short seat, turn;           /* YG: this player's seat and the seat to move (-1: not said) */
	char a[14], b[14];          /* YG: the opponent; WG: who waits; PG: the two players */
} Row;

static Row gRows[kKinds][kRows], gNewRows[kKinds][kRows];
static short gRowN[kKinds], gNewRowN[kKinds];
static Boolean gRowsKnown;

static const Rect gFieldRect[kFields] = {
	{ 28, 70, 44, 350 }, { 50, 70, 66, 140 }, { 72, 70, 88, 250 }, { 50, 8, 66, 350 }
};
static const Rect gButtonRect[kButtons] = {
	{ 96, 70, 116, 170 }, { 74, 8, 94, 108 },
	{ 114, 210, 136, 392 }, { 150, 254, 168, 320 }, { 150, 326, 168, 392 },
	{ 4, 318, 22, 394 }
};
static const char *const gButtonText[kButtons] = {
	"Sign in", "OK", "", "Accept", "Decline", "Sign out"
};
static const Rect gPane = { 46, 4, 202, 396 };
static const Pattern kWhite = { { 0, 0, 0, 0, 0, 0, 0, 0 } };
static const Rect gPop = { 116, 78, 134, 200 };
static Rect gTabRect[kTabs];

/* ---- drawing ----------------------------------------------------------------------------------------- */

static void DrawC(const char *s)
{
	DrawText((Ptr)s, 0, strlen(s));
}

/* s, cut to fit width */
static void DrawFit(const char *s, short width)
{
	short n = strlen(s);

	if (TextWidth((Ptr)s, 0, n) <= width) {
		DrawText((Ptr)s, 0, n);
		return;
	}
	while (n > 0 && TextWidth((Ptr)s, 0, n) + CharWidth('.') * 3 > width)
		n--;
	DrawText((Ptr)s, 0, n);
	DrawText((Ptr)"...", 0, 3);
}

static void Body(void)
{
	TextFont(3);        /* Geneva */
	TextSize(9);
	TextFace(0);
}

static void Chrome(void)
{
	TextFont(0);        /* the system font */
	TextSize(12);
	TextFace(0);
}

static void Num(char *out, long v)
{
	char t[12];
	short n = 0;

	out += strlen(out);
	if (v < 0) {
		*out++ = '-';
		v = -v;
	}
	do {
		t[n++] = '0' + v % 10;
		v /= 10;
	} while (v);
	while (n)
		*out++ = t[--n];
	*out = 0;
}

static void PStr(unsigned char *out, const char *s)
{
	short n = strlen(s);

	if (n > 255)
		n = 255;
	out[0] = n;
	memcpy(out + 1, s, n);
}

static Boolean Same(const char *a, const char *b)
{
	while (*a && *b) {
		char x = *a++, y = *b++;
		if (x >= 'A' && x <= 'Z')
			x += 'a' - 'A';
		if (y >= 'A' && y <= 'Z')
			y += 'a' - 'A';
		if (x != y)
			return false;
	}
	return *a == *b;
}

static Boolean Shown(short b)
{
	switch (b) {
	case bSignIn:
		return gState == sOffline;
	case bOK:
		return gState == sAsking;
	case bSignOut:
		return gState == sLobby;
	case bPlay:
		return gState == sLobby && gTab == tNew;
	case bAccept: case bDecline:
		return gState == sLobby && gTab == tNew && gInviteN > 0;
	}
	return false;
}

static Boolean FieldShown(short f)
{
	return f == fAnswer ? gState == sAsking : gState == sOffline;
}

/* The opponent as the pop-up box shows it */
static const char *OppText(void)
{
	return gOpp == oWait ? "Wait for opponent" : gOpp == oBot ? "Bot" : gOppName;
}

/* "Checkers vs the bot", "Checkers: wait for opponent", "Checkers vs ann" */
static void PlayLabel(char *out)
{
	strcpy(out, gGames[gGame].name);
	if (gOpp == oWait)
		strcat(out, ": wait for opponent");
	else {
		strcat(out, " vs ");
		strcat(out, gOpp == oBot ? "the bot" : gOppName);
	}
}

static void GameCell(short i, Rect *r)
{
	SetRect(r, 10 + (i % 4) * 96, 52 + (i / 4) * 18, 102 + (i % 4) * 96, 68 + (i / 4) * 18);
}

static void DrawField(short f)
{
	Rect r = gFieldRect[f];
	char shown[66];
	short n = strlen(gFields[f].text), i;

	EraseRect(&r);
	FrameRect(&r);
	Body();
	strcpy(shown, gFields[f].text);
	if (f == fAnswer && gAskSecret)
		for (i = 0; i < n; i++)
			shown[i] = '*';
	MoveTo(r.left + 4, r.bottom - 5);
	DrawC(shown);
	if (f == gFocus) {
		short x = r.left + 4 + TextWidth(shown, 0, n);
		MoveTo(x, r.top + 3);
		LineTo(x, r.bottom - 4);
	}
}

static void DrawButtonAt(const Rect *r, const char *label)
{
	Rect b = *r;

	Chrome();
	EraseRoundRect(&b, 12, 12);
	FrameRoundRect(&b, 12, 12);
	MoveTo((b.left + b.right - TextWidth((Ptr)label, 0, strlen(label))) / 2, b.bottom - 5);
	DrawC(label);
	Body();
}

/* a small button in a row of a list: Resume, Join */
static void RowButton(Rect *r, short y)
{
	SetRect(r, 322, y, 390, y + 17);
}

static void TabLabel(short i, char *out)
{
	static const char *const names[kTabs] = { "New game", "Your games", "All games" };

	strcpy(out, names[i]);
	if (i == tMine && gRowsKnown && gRowN[rMine]) {
		strcat(out, " (");
		Num(out, gRowN[rMine]);
		strcat(out, ")");
	}
}

/* Three tabs on the pane's top edge; the one chosen is joined to the pane, the others stand a little lower. */
static void DrawTabs(void)
{
	short i, x = gPane.left + 6;
	char label[32];
	Rect r, p = gPane;

	Chrome();
	EraseRect(&p);
	FrameRect(&p);
	for (i = 0; i < kTabs; i++) {
		TabLabel(i, label);
		TextFace(i == gTab ? bold : 0);
		SetRect(&r, x, gPane.top - 18, x + TextWidth(label, 0, strlen(label)) + 20, gPane.top + 1);
		gTabRect[i] = r;
		x = r.right + 2;
		if (i != gTab)
			r.top += 3;
		MoveTo(r.left, r.bottom - 1);
		LineTo(r.left, r.top + 3);
		LineTo(r.left + 3, r.top);
		LineTo(r.right - 4, r.top);
		LineTo(r.right - 1, r.top + 3);
		LineTo(r.right - 1, r.bottom - 1);
		MoveTo(r.left + 10, r.bottom - 5);
		DrawC(label);
		if (i == gTab) {            /* joined to the pane: no line under it */
			PenPat(&kWhite);
			MoveTo(r.left + 1, gPane.top);
			LineTo(r.right - 2, gPane.top);
			PenNormal();
		}
	}
	TextFace(0);
	Body();
}

/* a line of a list: "Grid Words with ann: your move. Table 12." */
static void MineText(const Row *w, char *out)
{
	strcpy(out, gGames[w->game].name);
	strcat(out, " with ");
	strcat(out, w->a);
	if (w->turn == w->seat)
		strcat(out, ": your move.");
	else if (w->turn >= 0) {
		strcat(out, ": ");
		strcat(out, w->a);
		strcat(out, "'s move.");
	} else
		strcat(out, ".");
	strcat(out, " Table ");
	strcat(out, w->table);
	strcat(out, ".");
}

static void DrawNewTab(void)
{
	short i;
	char text[96];

	Body();
	for (i = 0; i < kGames; i++) {
		Rect r;
		GameCell(i, &r);
		MoveTo(r.left + 4, r.bottom - 4);
		DrawC(gGames[i].name);
		if (i == gGame)
			InvertRect(&r);
		else
			FrameRect(&r);
	}
	MoveTo(12, gPop.bottom - 5);
	DrawC("Opponent");
	{
		/* the pop-up box: framed, a one-pixel shadow, the choice and a small triangle */
		Rect b = gPop;
		short tx = b.right - 16, ty = b.top + 7, k;
		Chrome();
		EraseRect(&b);
		FrameRect(&b);
		MoveTo(b.left + 2, b.bottom);
		LineTo(b.right, b.bottom);
		LineTo(b.right, b.top + 2);
		MoveTo(b.left + 6, b.bottom - 5);
		DrawFit(OppText(), b.right - b.left - 30);
		for (k = 0; k < 6; k++) {
			MoveTo(tx + k, ty + k);
			LineTo(tx + 10 - k, ty + k);
		}
		Body();
	}
	if (gInviteN) {
		MoveTo(12, gButtonRect[bAccept].bottom - 5);
		strcpy(text, gInvites[0].from);
		strcat(text, " invites you to ");
		strcat(text, gGames[gInvites[0].game].name);
		strcat(text, ".");
		DrawFit(text, gButtonRect[bAccept].left - 20);
		if (gInviteN > 1) {
			MoveTo(12, gButtonRect[bAccept].bottom + 12);
			text[0] = 0;
			Num(text, gInviteN - 1);
			strcat(text, gInviteN > 2 ? " more invitations follow." : " more invitation follows.");
			DrawC(text);
		}
	} else {
		MoveTo(12, gButtonRect[bAccept].bottom - 5);
		DrawC("No invitations for you just now.");
	}
}

static void DrawMineTab(void)
{
	short i, y = gPane.top + 8, n = gRowN[rMine];
	Boolean others = false;
	char text[96];
	Rect r;

	Body();
	if (!gRowsKnown) {
		MoveTo(12, y + 12);
		DrawC("Asking the parlor...");
		return;
	}
	if (!n) {
		MoveTo(12, y + 12);
		DrawC("You have no games being played.");
		return;
	}
	for (i = 0; i < n && i < kMineShown; i++, y += 20) {
		Row *w = &gRows[rMine][i];
		MineText(w, text);
		MoveTo(12, y + 12);
		DrawFit(text, 300);
		RowButton(&r, y);
		if (gGames[w->game].resumes)
			DrawButtonAt(&r, "Resume");
		else {
			others = true;
			MoveTo(r.left + 2, y + 12);
			DrawC("web, telnet");
		}
	}
	MoveTo(12, gPane.bottom - 8);
	if (n > kMineShown) {
		text[0] = 0;
		Num(text, n - kMineShown);
		strcat(text, " more are listed on the web.");
		DrawC(text);
	} else if (others)
		DrawC("Othello, Dots&Boxes and BattleShips go on on the web or by telnet.");
}

static void DrawAllTab(void)
{
	short i, y = gPane.top + 6, n = gRowN[rWait];
	char text[96];
	Rect r;

	Body();
	if (!gRowsKnown) {
		MoveTo(12, y + 14);
		DrawC("Asking the parlor...");
		return;
	}
	TextFace(bold);
	MoveTo(12, y + 12);
	DrawC("Waiting for a player");
	TextFace(0);
	y += 18;
	if (!n) {
		MoveTo(12, y + 10);
		DrawC("Nobody is waiting for a player just now.");
		y += 16;
	}
	for (i = 0; i < n && i < kWaitShown; i++, y += 20) {
		Row *w = &gRows[rWait][i];
		strcpy(text, gGames[w->game].name);
		strcat(text, ": ");
		strcat(text, w->a);
		strcat(text, " waits. Table ");
		strcat(text, w->table);
		strcat(text, ".");
		MoveTo(12, y + 12);
		DrawFit(text, 300);
		RowButton(&r, y);
		DrawButtonAt(&r, "Join");
	}
	y += 4;
	TextFace(bold);
	MoveTo(12, y + 12);
	DrawC("Being played");
	TextFace(0);
	y += 14;
	n = gRowN[rPlay];
	if (!n) {
		MoveTo(12, y + 10);
		DrawC("No other games are being played.");
	}
	for (i = 0; i < n && y + 12 < gPane.bottom - 2; i++, y += 12) {
		Row *w = &gRows[rPlay][i];
		strcpy(text, gGames[w->game].name);
		strcat(text, ": ");
		strcat(text, w->a);
		strcat(text, " and ");
		strcat(text, w->b);
		strcat(text, ". Table ");
		strcat(text, w->table);
		strcat(text, ".");
		MoveTo(12, y + 10);
		DrawFit(text, 370);
	}
}

static void DrawAll(void)
{
	GrafPtr old;
	Rect all = { 0, 0, kH, kW };
	short i, y;
	static const char *const labels[3] = { "Server", "Port", "Name" };
	char label[64];

	if (!gWin)
		return;
	GetPort(&old);
	SetPort(gWin);
	EraseRect(&all);
	Chrome();
	TextFace(bold);
	MoveTo(8, 18);
	DrawC("ParlorNet");
	TextFace(0);
	Body();
	MoveTo(90, 18);
	switch (gState) {
	case sOffline:
		DrawC("Sign in to a ParlorNet parlor.");
		for (i = 0; i < 3; i++) {
			MoveTo(8, gFieldRect[i].bottom - 5);
			DrawC(labels[i]);
		}
		break;
	case sConnecting:
	case sSigning:
		DrawC("Signing in...");
		break;
	case sAsking:
		DrawC("The parlor asks:");
		MoveTo(8, 42);
		DrawC(gAskLabel);
		break;
	case sLobby:
		strcpy(label, "Signed in as ");
		strcat(label, gMe);
		strcat(label, " at ");
		strcat(label, gServer);
		strcat(label, ".");
		DrawFit(label, gButtonRect[bSignOut].left - 98);
		DrawTabs();
		if (gTab == tNew)
			DrawNewTab();
		else if (gTab == tMine)
			DrawMineTab();
		else
			DrawAllTab();
		break;
	}
	for (i = 0; i < kFields; i++)
		if (FieldShown(i))
			DrawField(i);
	for (i = 0; i < kButtons; i++)
		if (Shown(i)) {
			strcpy(label, gButtonText[i]);
			if (i == bPlay)
				PlayLabel(label);
			DrawButtonAt(&gButtonRect[i], label);
		}
	Body();
	if (gState != sLobby) {
		MoveTo(0, gPane.bottom);
		LineTo(kW, gPane.bottom);
	}
	for (i = 0, y = kNoteY; i < kNotes; i++, y += 12) {
		MoveTo(8, y);
		DrawC(gNotes[i]);
	}
	Chrome();
	SetPort(old);
}

static pascal void DrawItem(WindowPtr w, short item)
{
	DrawAll();
}

/* A line for the message area, wrapped to its width; the oldest lines scroll away. */
void LobbyNote(const char *text)
{
	GrafPtr old;
	short n = strlen(text), take, i;

	if (!gWin)
		return;
	GetPort(&old);
	SetPort(gWin);
	Body();
	while (n > 0) {
		take = n > 71 ? 71 : n;
		while (take > 1 && TextWidth((Ptr)text, 0, take) > kW - 16)
			take--;
		if (take < n)
			for (i = take; i > take / 2; i--)
				if (text[i] == ' ') {
					take = i;
					break;
				}
		for (i = 0; i < kNotes - 1; i++)
			strcpy(gNotes[i], gNotes[i + 1]);
		memcpy(gNotes[kNotes - 1], text, take);
		gNotes[kNotes - 1][take] = 0;
		text += take;
		n -= take;
		while (n > 0 && *text == ' ') {
			text++;
			n--;
		}
	}
	Chrome();
	SetPort(old);
	DrawAll();
}

/* ---- the parlor ------------------------------------------------------------------------------------- */

Boolean LobbySignedIn(void)
{
	return gState == sLobby;
}

static void Go(short state)
{
	gState = state;
	gFocus = state == sAsking ? fAnswer : state == sOffline ? fName : -1;
	if (state != sLobby) {
		gReqN = gInviteN = 0;
		gRowsKnown = false;
		gWhoN = 0;
	}
	DrawAll();
}

static void Word(const char *line, short n, char *out, short max)
{
	short len = 0;

	while (n-- > 0) {
		while (*line && *line != ' ')
			line++;
		while (*line == ' ')
			line++;
	}
	while (*line && *line != ' ' && len < max - 1)
		out[len++] = *line++;
	out[len] = 0;
}

static short Int(const char *s)
{
	short v = 0, neg = *s == '-';

	if (neg)
		s++;
	while (*s >= '0' && *s <= '9')
		v = v * 10 + *s++ - '0';
	return neg ? -v : v;
}

static void Request(char kind, const char *line)
{
	short i;

	if (gReqN == sizeof gReqs)
		return;
	gReqs[gReqN++] = kind;
	if (kind == 'I')
		gNewInviteN = 0;
	else if (kind == 'W')
		gNewWhoN = 0;
	else if (kind == 'T')
		for (i = 0; i < kKinds; i++)
			gNewRowN[i] = 0;
	SendLine(line);
}

static Boolean Waiting(char kind)
{
	short i;

	for (i = 0; i < gReqN; i++)
		if (gReqs[i] == kind)
			return true;
	return false;
}

/* the game of "key rules", if a rulebook here plays it; else -1 */
static short GameOf(const char *key, const char *rules)
{
	char k[48];
	short i;

	strcpy(k, key);
	strcat(k, " ");
	strcat(k, rules);
	for (i = 0; i < kGames; i++)
		if (strcmp(k, gGames[i].key) == 0)
			return (TunnelInstalled() & (1 << i)) ? i : -1;
	return -1;
}

/* An IVT line: table, game/version, rules, from, status, [seat=n] */
static void InviteLine(const char *line)
{
	char key[24], rules[24], status[12];
	Invite *v;

	if (gNewInviteN == kInvites)
		return;
	v = &gNewInvites[gNewInviteN];
	Word(line, 1, v->table, sizeof v->table);
	Word(line, 2, key, sizeof key);
	Word(line, 3, rules, sizeof rules);
	Word(line, 4, v->from, sizeof v->from);
	Word(line, 5, status, sizeof status);
	if (strcmp(status, "open") != 0)
		return;
	if ((v->game = GameOf(key, rules)) >= 0)
		gNewInviteN++;
}

/* A PLR line: a player online; this player and the bots are left out */
static void WhoLine(const char *line)
{
	char name[16], flag[12];
	short i;

	Word(line, 1, name, sizeof name);
	for (i = 2; i < 6; i++) {
		Word(line, i, flag, sizeof flag);
		if (strcmp(flag, "bot") == 0)
			return;
	}
	if (!name[0] || Same(name, gMe) || gNewWhoN == kWho || strlen(name) > 12)
		return;
	strcpy(gNewWho[gNewWhoN++], name);
}

/* A line of TBL's answer: YG table game rules seat turn opponent, WG table game rules name, PG table game rules
   name name */
static void TableLine(const char *line, short kind)
{
	char key[24], rules[24], f[16];
	Row *w;

	if (gNewRowN[kind] == kRows)
		return;
	w = &gNewRows[kind][gNewRowN[kind]];
	Word(line, 1, w->table, sizeof w->table);
	Word(line, 2, key, sizeof key);
	Word(line, 3, rules, sizeof rules);
	if ((w->game = GameOf(key, rules)) < 0)
		return;
	w->b[0] = 0;
	if (kind == rMine) {
		Word(line, 4, f, sizeof f);
		w->seat = Int(f);
		Word(line, 5, f, sizeof f);
		w->turn = Int(f);
		Word(line, 6, w->a, sizeof w->a);
	} else {
		Word(line, 4, w->a, sizeof w->a);
		if (kind == rPlay)
			Word(line, 5, w->b, sizeof w->b);
	}
	gNewRowN[kind]++;
}

/* The answer to the oldest request has ended (OK or ERR). */
static void Answered(Boolean ok)
{
	char kind = gReqs[0], text[80];
	short i;

	for (i = 1; i < gReqN; i++)
		gReqs[i - 1] = gReqs[i];
	gReqN--;
	if (kind == 'W' && ok) {
		memcpy(gWho, gNewWho, sizeof gWho);
		gWhoN = gNewWhoN;
	} else if (kind == 'I' && ok) {
		Boolean changed = gNewInviteN != gInviteN || memcmp(gNewInvites, gInvites, gNewInviteN * sizeof(Invite));
		Boolean more = gNewInviteN > gInviteN;
		memcpy(gInvites, gNewInvites, sizeof gInvites);
		gInviteN = gNewInviteN;
		if (more && gTab != tNew) {
			strcpy(text, gInvites[gInviteN - 1].from);
			strcat(text, " invites you to ");
			strcat(text, gGames[gInvites[gInviteN - 1].game].name);
			strcat(text, ": see New game.");
			LobbyNote(text);
		} else if (changed)
			DrawAll();
	} else if (kind == 'T' && ok) {
		memcpy(gRows, gNewRows, sizeof gRows);
		memcpy(gRowN, gNewRowN, sizeof gRowN);
		gRowsKnown = true;
		DrawAll();
	}
}

/* From the lobby's idle time: the player's invitations, every few seconds. */
void LobbyIdle(void)
{
	if (gState == sLobby && TcpIsOpen() && TickCount() - gPolled > kPollTicks && gReqN == 0) {
		gPolled = TickCount();
		Request('I', "IVL");
	}
}

void LobbyLost(void)
{
	if (gState != sOffline) {
		LobbyNote("The connection to the parlor ended.");
		Go(sOffline);
	}
}

static void SignOut(void)
{
	if (TcpIsOpen()) {
		SendLine("OUT");
		SendLine("BYE");
	}
	TcpClose();
	TunnelReset();
	Go(sOffline);
}

/* CLI: what the turns played from here say they were played with, the program and the Mac (ParlorNet's
   docs/protocol.md, Clients and machines). GameMaster's version is known when its code is 1.0.0's (TunnelCheck). The answers are
   OK, or ERR from a parlor older than CLI, and change nothing. */
static void Claim(void)
{
	char line[64];

	strcpy(line, TunnelCheck() ? "CLI PROG GameMaster 1.0.0 + ParlorNet " : "CLI PROG GameMaster + ParlorNet ");
	strcat(line, kVersion);
	Request('C', line);
	strcpy(line, "CLI MACH ");
	strcat(line, MachineName());
	Request('C', line);
}

void LobbyLine(char *line)
{
	char verb[16];

	Word(line, 0, verb, sizeof verb);
	if (gState == sConnecting && strcmp(verb, "PARLORNET/1") == 0) {
		char f[24];
		short i;
		Word(line, 1, gServer, sizeof gServer);
		gAuth[0] = 0;
		for (i = 2; i < 6; i++) {
			Word(line, i, f, sizeof f);
			if (strncmp(f, "AUTH=", 5) == 0)
				strcpy(gAuth, f + 5);
		}
		strcpy(f, strcmp(gAuth, "none") == 0 ? "HELO " : "USER ");
		strcat(f, gFields[fName].text);
		strcpy(gMe, gFields[fName].text);
		SendLine(f);
		Go(sSigning);
	} else if (strcmp(verb, "INF") == 0) {
		LobbyNote(line + 4);
	} else if (strcmp(verb, "ASK") == 0 && (gState == sSigning || gState == sAsking)) {
		char flag[12];
		short i;
		Word(line, 1, gAskField, sizeof gAskField);
		Word(line, 2, gAskLabel, sizeof gAskLabel);
		for (i = 0; gAskLabel[i]; i++)
			if (gAskLabel[i] == '_')
				gAskLabel[i] = ' ';
		gAskSecret = false;
		for (i = 3; i < 7; i++) {
			Word(line, i, flag, sizeof flag);
			if (strcmp(flag, "secret") == 0)
				gAskSecret = true;
		}
		gFields[fAnswer].text[0] = 0;
		Go(sAsking);
	} else if (strcmp(verb, "OK") == 0 && gState == sSigning) {
		LobbyNote("Signed in. New game starts a game; Your games takes up one you are playing; All games shows "
			"who waits for a player.");
		gPolled = 0;
		gTab = tNew;
		Go(sLobby);
		Claim();
		Request('T', "TBL");            /* for the count on Your games */
	} else if (strcmp(verb, "ERR") == 0 && (gState == sSigning || gState == sAsking || gState == sConnecting)) {
		char *text = line + 4;
		while (*text && *text != ' ')
			text++;
		while (*text == ' ')
			text++;
		if (strncmp(text, "wait=", 5) == 0)
			while (*text && *text++ != ' ')
				;
		LobbyNote(*text ? text : line);
		SignOut();
	} else if (gState != sLobby || !gReqN) {
		return;
	} else if (gReqs[0] == 'W' && strcmp(verb, "PLR") == 0) {
		WhoLine(line);
	} else if (gReqs[0] == 'I' && strcmp(verb, "IVT") == 0) {
		InviteLine(line);
	} else if (gReqs[0] == 'T' && (strcmp(verb, "YG") == 0 || strcmp(verb, "WG") == 0 || strcmp(verb, "PG") == 0)) {
		TableLine(line, verb[0] == 'Y' ? rMine : verb[0] == 'W' ? rWait : rPlay);
	} else if (strcmp(verb, "OK") == 0 || strcmp(verb, "ERR") == 0) {
		Answered(verb[0] == 'O');
	}
}

static void SignIn(void)
{
	unsigned char host[66];
	unsigned long addr;
	long port = 0;
	short i;
	OSErr err;
	Boolean lookup;
	char *p;

	if (!gFields[fName].text[0]) {
		LobbyNote("Type your name on the parlor first.");
		return;
	}
	for (p = gFields[fPort].text; *p >= '0' && *p <= '9'; p++)
		port = port * 10 + (*p - '0');
	if (*p || port < 1 || port > 65535) {
		LobbyNote("The port is a number from 1 to 65535.");
		return;
	}
	host[0] = strlen(gFields[fHost].text);
	memcpy(host + 1, gFields[fHost].text, host[0]);
	Go(sConnecting);
	LobbyNote("Connecting...");
	err = TcpResolve(host, &addr);
	lookup = err != noErr;
	if (err == noErr)
		err = TcpOpen(addr, (unsigned short)port);
	if (err == noErr)
		err = TcpSend("PARLORNET/1\r\n", 13);     /* the opening, as on the core's public port */
	if (err == fnfErr || err == resNotFound) {
		LobbyNote("This Mac has no MacTCP (or TCP/IP control panel), so it cannot reach a parlor.");
		TcpClose();
		Go(sOffline);
	} else if (err != noErr) {
		char text[64];
		strcpy(text, lookup ? "Could not look up the parlor's name (error " : "Could not reach the parlor (error ");
		i = strlen(text);
		text[i] = 0;
		Num(text, err);
		strcat(text, ").");
		LobbyNote(text);
		TcpClose();
		Go(sOffline);
	}
}

static void Answer(void)
{
	char line[90];

	strcpy(line, "AUT ");
	strcat(line, gAskField);
	strcat(line, " ");
	strcat(line, gFields[fAnswer].text);
	memset(gFields[fAnswer].text, 0, sizeof gFields[fAnswer].text);
	SendLine(line);
	memset(line, 0, sizeof line);
	Go(sSigning);
}

static void Play(const unsigned char *id, const char *who)
{
	if (!gHooked) {
		LobbyNote("This copy of GameMaster is not GameMaster 1.0.0 as released, so games cannot start here.");
		return;
	}
	TunnelStart(id, who);
}

static void PlayNew(void)
{
	Play(gGames[gGame].id, gOpp == oWait ? "anyone" : gOpp == oBot ? "bot" : gOppName);
}

/* the first invitation is taken or declined: it leaves the list until the next answer to IVL */
static void DropInvite(void)
{
	short i;

	for (i = 1; i < gInviteN; i++)
		gInvites[i - 1] = gInvites[i];
	gInviteN--;
	gPolled = TickCount();
	DrawAll();
}

/* ---- the opponent: a pop-up menu, and a dialog for a name ------------------------------------------- */

static Boolean NameOK(const char *s)
{
	short n = 0;

	for (; *s; s++, n++)
		if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_'
				|| *s == '-'))
			return false;
	return n >= 1 && n <= 12;
}

static pascal void OutlineOK(DialogPtr d, short item)
{
	short type;
	Handle h;
	Rect r;

	GetDialogItem(d, 1, &type, &h, &r);
	InsetRect(&r, -4, -4);
	PenSize(3, 3);
	FrameRoundRect(&r, 16, 16);
	PenNormal();
}

/* An item of a dialog's item list, built in memory: its rectangle, type and text */
static Ptr Item(Ptr p, short top, short left, short bottom, short right, unsigned char type, const char *text)
{
	short n = strlen(text);

	memset(p, 0, 4);
	p += 4;
	*(short *)p = top;
	*(short *)(p + 2) = left;
	*(short *)(p + 4) = bottom;
	*(short *)(p + 6) = right;
	p += 8;
	*p++ = type;
	*p++ = n;
	memcpy(p, text, n);
	p += n + (n & 1);
	return p;
}

/* Other...: a small dialog for a player's name, made at run time (the rulebook has no dialog resources of its
   own); true with the name in out when OK was clicked */
static Boolean AskName(char *out)
{
	Handle items = NewHandleClear(256);
	DialogPtr d;
	Rect r;
	Point at;
	short item = 0, type, n;
	Handle h;
	Ptr p;
	Str255 got;

	if (!items)
		return false;
	HLock(items);
	p = *items + 2;
	p = Item(p, 72, 186, 92, 256, 4, "OK");                     /* button */
	p = Item(p, 72, 110, 92, 176, 4, "Cancel");
	p = Item(p, 10, 14, 42, 256, 8 + 128, "The name of the player to invite (online or not):");
	p = Item(p, 46, 16, 62, 254, 16, "");                       /* edit text */
	p = Item(p, 0, 0, 0, 0, 0 + 128, "");                       /* the outline of OK */
	*(short *)*items = 5 - 1;
	n = p - *items;
	HUnlock(items);
	SetHandleSize(items, n);
	at.h = 60;
	at.v = 60;
	SetPort(gWin);
	LocalToGlobal(&at);
	SetRect(&r, at.h, at.v, at.h + 270, at.v + 102);
	d = NewDialog(NULL, &r, (ConstStringPtr)"\p", true, dBoxProc, (WindowPtr)-1, false, 0, items);
	if (!d)
		return false;
	GetDialogItem(d, 5, &type, &h, &r);
	SetDialogItem(d, 5, userItem + itemDisable, (Handle)OutlineOK, &r);
	SelectDialogItemText(d, 4, 0, 32767);
	while (item != 1 && item != 2)
		ModalDialog(NULL, &item);
	GetDialogItem(d, 4, &type, &h, &r);
	GetDialogItemText(h, got);
	DisposeDialog(d);
	SetPort(gWin);
	if (item != 1)
		return false;
	n = got[0] > 15 ? 15 : got[0];
	memcpy(out, got + 1, n);
	out[n] = 0;
	return true;
}

/* The players online, fresh: WHO, waited for a moment (the menu shows the last answer if it is slow) */
static void AskWho(void)
{
	unsigned long until = TickCount() + 90;

	if (!TcpIsOpen())
		return;
	Request('W', "WHO");
	while (Waiting('W') && TcpIsOpen() && TickCount() < until) {
		Pump();
		SystemTask();
	}
}

static void OpponentMenu(void)
{
	MenuHandle m;
	Str255 s;
	short i, n, names, current, other;
	long got;
	Point at;
	char name[16];

	AskWho();
	m = NewMenu(kPopID, (ConstStringPtr)"\p");
	if (!m)
		return;
	/* each item is added as a plain x, then its text set, so that a name shows as it is (; ( / and the like are
	   no menu codes there) */
	AppendMenu(m, (ConstStringPtr)"\px");
	SetMenuItemText(m, 1, (ConstStringPtr)"\pWait for opponent");
	AppendMenu(m, (ConstStringPtr)"\px");
	SetMenuItemText(m, 2, (ConstStringPtr)"\pBot");
	AppendMenu(m, (ConstStringPtr)"\p(-");
	n = 3;
	current = gOpp == oWait ? 1 : gOpp == oBot ? 2 : 0;
	names = gWhoN;
	for (i = 0; i < names; i++) {
		AppendMenu(m, (ConstStringPtr)"\px");
		PStr(s, gWho[i]);
		SetMenuItemText(m, ++n, s);
		if (gOpp == oName && Same(gWho[i], gOppName))
			current = n;
	}
	if (!names) {
		AppendMenu(m, (ConstStringPtr)"\px");
		SetMenuItemText(m, ++n, (ConstStringPtr)"\pNobody else is online");
		DisableItem(m, n);
	}
	AppendMenu(m, (ConstStringPtr)"\p(-");
	n++;
	AppendMenu(m, (ConstStringPtr)"\px");
	SetMenuItemText(m, ++n, (ConstStringPtr)"\pOther...");
	other = n;
	if (!current)
		current = other;
	CheckItem(m, current, true);
	InsertMenu(m, -1);
	SetPort(gWin);
	at.v = gPop.top + 1;
	at.h = gPop.left + 1;
	LocalToGlobal(&at);
	got = PopUpMenuSelect(m, at.v, at.h, current);
	DeleteMenu(kPopID);
	DisposeMenu(m);
	i = LoWord(got);
	if (HiWord(got) != kPopID || i == 0)
		;
	else if (i == 1)
		gOpp = oWait;
	else if (i == 2)
		gOpp = oBot;
	else if (i == other) {
		if (AskName(name)) {
			if (!NameOK(name)) {
				SysBeep(10);
				LobbyNote("A name is 1 to 12 letters, digits, _ or -.");
			} else if (Same(name, gMe)) {
				SysBeep(10);
				LobbyNote("You cannot play yourself.");
			} else if (Same(name, "bot")) {
				gOpp = oBot;
			} else {
				strcpy(gOppName, name);
				gOpp = oName;
			}
		}
	} else if (i >= 4 && i - 4 < names) {
		strcpy(gOppName, gWho[i - 4]);
		gOpp = oName;
	}
	DrawAll();
}

/* ---- clicks and keys -------------------------------------------------------------------------------- */

static Boolean Track(const Rect *r0)
{
	Rect r = *r0;
	Point pt;

	InvertRoundRect(&r, 12, 12);
	while (StillDown())
		;
	InvertRoundRect(&r, 12, 12);
	GetMouse(&pt);
	return PtInRect(pt, &r);
}

static void OpenTab(short t)
{
	gTab = t;
	if (t != tNew && TcpIsOpen() && !Waiting('T'))
		Request('T', "TBL");
	DrawAll();
}

/* Resume and Join, in the rows of Your games and All games */
static Boolean RowClick(Point pt)
{
	short i, n, y;
	Rect r;
	char who[16], text[96];
	Row *w;

	if (gTab == tMine) {
		n = gRowN[rMine];
		for (i = 0, y = gPane.top + 8; i < n && i < kMineShown; i++, y += 20) {
			w = &gRows[rMine][i];
			RowButton(&r, y);
			if (!PtInRect(pt, &r) || !gGames[w->game].resumes)
				continue;
			if (Track(&r)) {
				strcpy(text, "Taking up ");
				strcat(text, gGames[w->game].name);
				strcat(text, " at table ");
				strcat(text, w->table);
				strcat(text, ", as it stands.");
				LobbyNote(text);
				strcpy(who, "@");
				strcat(who, w->table);
				Play(gGames[w->game].id, who);
			}
			return true;
		}
	} else if (gTab == tAll) {
		n = gRowN[rWait];
		for (i = 0, y = gPane.top + 24; i < n && i < kWaitShown; i++, y += 20) {
			w = &gRows[rWait][i];
			RowButton(&r, y);
			if (!PtInRect(pt, &r))
				continue;
			if (Track(&r)) {
				strcpy(text, "Joining ");
				strcat(text, w->a);
				strcat(text, " at table ");
				strcat(text, w->table);
				strcat(text, ".");
				LobbyNote(text);
				strcpy(who, "#");
				strcat(who, w->table);
				Play(gGames[w->game].id, who);
			}
			return true;
		}
	}
	return false;
}

static void Click(Point pt)
{
	short i;

	for (i = 0; i < kFields; i++)
		if (FieldShown(i) && PtInRect(pt, &gFieldRect[i])) {
			gFocus = i;
			DrawAll();
			return;
		}
	if (gState == sLobby) {
		for (i = 0; i < kTabs; i++)
			if (PtInRect(pt, &gTabRect[i])) {
				OpenTab(i);
				return;
			}
		if (gTab == tNew) {
			for (i = 0; i < kGames; i++) {
				Rect r;
				GameCell(i, &r);
				if (PtInRect(pt, &r)) {
					gGame = i;
					DrawAll();
					return;
				}
			}
			if (PtInRect(pt, &gPop)) {
				OpponentMenu();
				return;
			}
		} else if (RowClick(pt))
			return;
	}
	for (i = 0; i < kButtons; i++)
		if (Shown(i) && PtInRect(pt, &gButtonRect[i])) {
			if (!Track(&gButtonRect[i]))
				return;
			switch (i) {
			case bSignIn:
				SignIn();
				break;
			case bOK:
				Answer();
				break;
			case bPlay:
				PlayNew();
				break;
			case bAccept:
				if (gInviteN) {
					char text[80];
					strcpy(text, "Taking ");
					strcat(text, gInvites[0].from);
					strcat(text, "'s invitation to ");
					strcat(text, gGames[gInvites[0].game].name);
					strcat(text, ".");
					LobbyNote(text);
					Play(gGames[gInvites[0].game].id, gInvites[0].from);
					DropInvite();
				}
				break;
			case bDecline:
				if (gInviteN) {
					char line[48];
					strcpy(line, "UNV ");
					strcat(line, gInvites[0].table);
					strcat(line, " ");
					strcat(line, gMe);
					Request('U', line);
					DropInvite();
				}
				break;
			case bSignOut:
				if (TunnelGames())
					LobbyNote("Your games through ParlorNet end with the sign-out.");
				SignOut();
				LobbyNote("Signed out.");
				break;
			}
			return;
		}
}

static void Key(short ch)
{
	Field *f;
	short n;

	if (gState == sLobby) {
		if ((ch == '\r' || ch == 3) && gTab == tNew)
			PlayNew();
		return;
	}
	if (gFocus < 0)
		return;
	f = &gFields[gFocus];
	n = strlen(f->text);
	if (ch == '\r' || ch == 3) {
		if (gState == sOffline)
			SignIn();
		else if (gState == sAsking)
			Answer();
		return;
	}
	if (ch == '\t') {
		if (gState == sOffline)
			gFocus = gFocus == fName ? fHost : gFocus + 1;
	} else if (ch == 8) {
		if (n)
			f->text[n - 1] = 0;
	} else if (ch >= ' ' && ch <= '~' && n < f->max)
		f->text[n] = (char)ch, f->text[n + 1] = 0;
	DrawField(gFocus);
}

/* ---- the rulebook ------------------------------------------------------------------------------------ */

static void OpenLobby(GameEvent *ev)
{
	short type, i;
	Handle h;
	Rect r;
	const unsigned char *ids[kGames];

	SetHandleSize(ev->game, 4);
	ev->myturn = true;
	if (gWin) {
		LobbyNote("The ParlorNet window is already open.");
		return;
	}
	GetPort(&gWin);
	GetDialogItem((DialogPtr)gWin, 1, &type, &h, &r);
	SetDialogItem((DialogPtr)gWin, 1, type, (Handle)DrawItem, &r);
	gHooked = TunnelInstall();
	if (!gHooked)
		LobbyNote("This copy of GameMaster is not GameMaster 1.0.0 as released: the lobby works, games do not.");
	for (i = 0; i < kGames; i++)
		ids[i] = gGames[i].id;
	TunnelAskInstalled(ids, kGames);
	gState = sOffline;
	gFocus = fName;
}

static void CloseLobby(GameEvent *ev)
{
	GrafPtr port;

	GetPort(&port);
	if (port != gWin)
		return;
	SignOut();
	TunnelRemove();
	gHooked = false;
	gWin = NULL;
}

pascal void RULES(GameEvent *ev)
{
	RETRO68_RELOCATE();
	switch (ev->event) {
	case ge_InitRuleBook:
		ev->int1 = kW;
		ev->int2 = kH;
		ev->globals = NULL;
		strcpy(gFields[fHost].text, "parlornet.org");
		gFields[fHost].max = 63;
		strcpy(gFields[fPort].text, "6508");
		gFields[fPort].max = 5;
		gFields[fName].max = 12;
		gFields[fAnswer].max = 64;
		gOpp = oBot;
		break;
	case ge_FinishRuleBook:
		if (gWin) {
			SignOut();
			TunnelRemove();
			gWin = NULL;
		}
		/* GameMaster closes the rulebook's file next, code and all: the zeroed globals Retro68 keeps in a
		   block of their own go with it */
		Retro68FreeGlobals();
		break;
	case ge_NewGame:
	case ge_OldGame:
		OpenLobby(ev);
		break;
	case ge_Close:
		CloseLobby(ev);
		break;
	case ge_MouseDown:
		Click(ev->where);
		break;
	case ge_KeyDown:
		Key(ev->int1);
		break;
	case ge_Idle:
		Pump();
		LobbyIdle();
		break;
	case ge_SleepQuery:
		ev->int1 = 15;
		break;
	case ge_Activate:
	case ge_Deactivate:
	case ge_UpdateMenus:
		break;
	default:
		break;
	}
}
