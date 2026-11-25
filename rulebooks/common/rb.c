/*
 * The shared part of ParlorNet's game rulebooks (rb.h): RULES, the game record's
 * header, the messages between the two sides and the panel beside the board.
 *
 * Plain 68000 code for System 6.0 and later, drawn for a 512 by 342 black and
 * white screen.
 */
#include <Quickdraw.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Fonts.h>
#include <Memory.h>
#include <Events.h>
#include <Sound.h>
#include <Retro68Runtime.h>
#include <string.h>

#include "rb.h"

/* the panel's last place, for clicks on its buttons */
static Rect gButton[2];
static char gButtonText[2][16];

/* ---- text ---------------------------------------------------------------------------------------- */

void RBText(const char *s)
{
	DrawText((Ptr)s, 0, strlen(s));
}

void RBBody(void)
{
	TextFont(3);        /* Geneva */
	TextSize(9);
	TextFace(0);
}

void RBChrome(void)
{
	TextFont(0);        /* the system font */
	TextSize(12);
	TextFace(0);
}

void RBNum(char *out, long v)
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

/* s in lines of width, from top (the first baseline); -> the lines used */
short RBWrap(const char *s, short left, short top, short width, short lines)
{
	short n = strlen(s), take, i, used = 0;

	while (n > 0 && used < lines) {
		take = n;
		while (take > 1 && TextWidth((Ptr)s, 0, take) > width)
			take--;
		if (take < n)
			for (i = take; i > take / 2; i--)
				if (s[i] == ' ') {
					take = i;
					break;
				}
		MoveTo(left, top + used * 11);
		DrawText((Ptr)s, 0, take);
		used++;
		s += take;
		n -= take;
		while (n > 0 && *s == ' ') {
			s++;
			n--;
		}
	}
	return used;
}

/* ---- the game's state -------------------------------------------------------------------------- */

void RBNote(RBHead *h, const char *text)
{
	strncpy(h->note, text, sizeof h->note - 1);
	h->note[sizeof h->note - 1] = 0;
}

static void Send(RBHead *h, const char *text)
{
	if (!h->connected)
		return;
	if (h->outN == kRBOut)
		h->outN--;
	strncpy(h->out[h->outN], text, kRBMsg - 1);
	h->out[h->outN][kRBMsg - 1] = 0;
	h->outN++;
}

static void Send2(RBHead *h, const char *verb, const char *text)
{
	char m[kRBMsg];

	strcpy(m, verb);
	strncat(m, text, kRBMsg - 1 - strlen(m));
	Send(h, m);
}

void RBEnd(RBHead *h, const char *result)
{
	h->over = 1;
	h->pending = h->offer = h->armed = 0;
	strncpy(h->result, result, sizeof h->result - 1);
	h->result[sizeof h->result - 1] = 0;
	h->note[0] = 0;
	GameClear(h);
}

short RBMover(RBHead *h)
{
	return h->connected ? h->local : h->turn;
}

short RBBottom(RBHead *h)
{
	return h->connected ? h->local : 0;
}

Boolean RBCanMove(RBHead *h)
{
	return !h->over && !h->pending && !h->offer && (!h->connected || h->turn == h->local);
}

/* a move, applied: the seat that made it may offer a draw again */
static Boolean Apply(RBHead *h, short seat, const char *move, char *why)
{
	if (!GameApply(h, seat, move, why))
		return false;
	if (h->noOffer == seat + 1)
		h->noOffer = 0;
	return true;
}

void RBPlay(RBHead *h, const char *move)
{
	char why[96];

	GameClear(h);
	h->armed = 0;
	if (!h->connected) {
		if (!Apply(h, h->turn, move, why))
			RBNote(h, why);
		else
			h->note[0] = 0;
		return;
	}
	strncpy(h->sent, move, kRBMsg - 1);
	h->pending = 'M';
	h->note[0] = 0;
	Send2(h, "M ", move);
}

/* ---- what a game may leave out ------------------------------------------------------------------ */

__attribute__((weak)) Boolean GameMessage(RBHead *h, const char *m)
{
	return false;
}

__attribute__((weak)) void GameKey(RBHead *h, short ch)
{
}

__attribute__((weak)) void GamePosition(RBHead *h)
{
}

void RBSend(RBHead *h, const char *text)
{
	Send(h, text);
}

static Boolean gQuiet;

void RBQuiet(void)
{
	gQuiet = true;
}

/* ---- the messages ------------------------------------------------------------------------------ */

/* the next number in *p, past spaces; -1 if there is none */
static short Next(const char **p)
{
	short v = 0;

	while (**p == ' ')
		(*p)++;
	if (**p < '0' || **p > '9')
		return -1;
	while (**p >= '0' && **p <= '9')
		v = v * 10 + *(*p)++ - '0';
	return v;
}

/* POS seat turn offer: a whole position follows */
static void Position(RBHead *h, const char *p)
{
	short seat = Next(&p), turn = Next(&p), offer = Next(&p);

	if (seat < 0 || seat > 1 || turn < 0 || turn > 1)
		return;
	h->local = seat;
	h->turn = turn;
	h->offer = offer == 1 || offer == 2 ? offer : 0;
	h->over = h->pending = h->noOffer = h->armed = 0;
	h->result[0] = h->note[0] = 0;
	GameClear(h);
	GamePosition(h);
	h->posing = 1;
}

static void Received(RBHead *h, const unsigned char *msg)
{
	char m[256], why[96], line[160];
	short other = 1 - h->local;

	memcpy(m, msg + 1, msg[0]);
	m[msg[0]] = 0;
	if (m[0] == 'M' && m[1] == ' ') {
		if (h->over)
			Send(h, "R the game is over");
		else if (Apply(h, other, m + 2, why))
			Send2(h, "A ", m + 2);
		else
			Send2(h, "R ", why);
	} else if (m[0] == 'A' && m[1] == ' ') {
		if (h->pending == 'M' && strcmp(h->sent, m + 2) == 0) {
			h->pending = 0;
			if (!Apply(h, h->local, m + 2, why)) {
				strcpy(line, "The two sides disagree: ");
				strncat(line, why, 60);
				RBNote(h, line);
			}
		}
	} else if (m[0] == 'R' && (m[1] == ' ' || !m[1])) {
		if (h->pending == 'M' || h->pending == 'Y' || h->pending == 'G' || h->offer == h->local + 1) {
			if (h->offer == h->local + 1)
				h->offer = 0;
			h->pending = 0;
			GameClear(h);
			strncpy(line, m[1] ? m + 2 : "Refused.", sizeof line - 2);
			line[sizeof line - 2] = 0;
			if (line[0] >= 'a' && line[0] <= 'z')
				line[0] -= 'a' - 'A';
			if (line[strlen(line) - 1] != '.')
				strcat(line, ".");
			RBNote(h, line);
		}
	} else if (strcmp(m, "D") == 0) {
		if (h->over || h->offer || h->noOffer == other + 1)
			Send(h, "R you cannot offer a draw now");
		else {
			h->offer = other + 1;
			GameClear(h);
			strcpy(line, GameSeat(other));
			strcat(line, " offers a draw.");
			RBNote(h, line);
		}
	} else if (strcmp(m, "DY") == 0) {
		if (h->offer == h->local + 1) {
			RBEnd(h, "draw agreed");
			Send(h, "E draw agreed");
		}
	} else if (strcmp(m, "DN") == 0) {
		if (h->offer == h->local + 1) {
			h->offer = 0;
			h->noOffer = h->local + 1;
			strcpy(line, GameSeat(other));
			strcat(line, " declines the draw.");
			RBNote(h, line);
		}
	} else if (strcmp(m, "G") == 0) {
		if (!h->over) {
			strcpy(line, "resign ");
			RBNum(line, other);
			RBEnd(h, line);
			Send2(h, "E ", line);
		}
	} else if (m[0] == 'E' && m[1] == ' ') {
		RBEnd(h, m + 2);
	} else if (strncmp(m, "POS ", 4) == 0) {
		Position(h, m + 4);
	} else if (strcmp(m, "POE") == 0) {
		h->posing = 0;
	} else
		GameMessage(h, m);
}

/* ---- the panel ---------------------------------------------------------------------------------- */

static void ResultText(RBHead *h, char *out)
{
	char how[32], *p = h->result;
	short seat = -1;

	out[0] = 0;
	if (strncmp(p, "win ", 4) == 0 || strncmp(p, "resign ", 7) == 0 || strncmp(p, "timeout ", 8) == 0) {
		p = strchr(p, ' ') + 1;
		if (*p == '0' || *p == '1')
			seat = *p - '0';
		p++;
		while (*p == ' ')
			p++;
		strncpy(how, p, sizeof how - 1);
		how[sizeof how - 1] = 0;
	}
	if (seat >= 0 && h->result[0] == 'w')
		GameWin(h, seat, how, out);
	else if (seat >= 0 && h->result[0] == 'r') {
		strcpy(out, GameSeat(seat));
		strcat(out, " resigns. ");
		strcat(out, GameSeat(1 - seat));
		strcat(out, " wins.");
	} else if (seat >= 0 && h->result[0] == 't') {
		strcpy(out, GameSeat(seat));
		strcat(out, " ran out of time. ");
		strcat(out, GameSeat(1 - seat));
		strcat(out, " wins.");
	} else if (strcmp(h->result, "draw agreed") == 0)
		strcpy(out, "A draw, by agreement.");
	else if (strncmp(h->result, "draw", 4) == 0)
		strcpy(out, "A draw.");
	else {
		strcpy(out, "The game is over: ");
		strcat(out, h->result);
	}
}

static void Buttons(RBHead *h)
{
	gButtonText[0][0] = gButtonText[1][0] = 0;
	if (!h->connected || h->over || h->pending)
		return;
	if (h->offer == 2 - h->local) {
		strcpy(gButtonText[0], "Accept draw");
		strcpy(gButtonText[1], "Decline");
		return;
	}
	if (gDraws && !h->offer && h->noOffer != h->local + 1)
		strcpy(gButtonText[0], "Offer draw");
	strcpy(gButtonText[1], h->armed ? "Resign: sure?" : "Resign");
}

void RBDrawPanel(RBHead *h, const Rect *r, const char *title, const char *extra)
{
	char line[128];
	short y = r->top + 12, i, w = r->right - r->left, limit;

	Buttons(h);
	limit = r->bottom - (gButtonText[0][0] ? 44 : 20);     /* the text stops above the first button drawn */
	RBChrome();
	TextFace(bold);
	MoveTo(r->left, y);
	RBText(title);
	RBBody();
	y += 16;
	MoveTo(r->left, y);
	if (h->connected) {
		strcpy(line, "You are ");
		strcat(line, GameSeat(h->local));
		strcat(line, ".");
	} else
		strcpy(line, gParlorOnly ? "Start it from ParlorNet." : "Both sides play here.");
	RBText(line);
	y += 14;
	TextFace(bold);
	if (h->over)
		ResultText(h, line);
	else if (h->pending == 'M')
		strcpy(line, "Sent; waiting for the answer.");
	else if (h->pending)
		strcpy(line, "Waiting for the answer.");
	else if (h->offer == h->local + 1 && h->connected)
		strcpy(line, "You offer a draw; waiting.");
	else if (h->offer)
		strcpy(line, "Accept the draw?");
	else if (!h->connected && gParlorOnly)
		strcpy(line, "It plays on a ParlorNet parlor.");
	else if (h->connected && h->turn == h->local)
		strcpy(line, "Your move.");
	else {
		strcpy(line, GameSeat(h->turn));
		strcat(line, " to move.");
	}
	y += 11 * RBWrap(line, r->left, y, w, 3);
	TextFace(0);
	y += 3;
	if (extra && *extra) {
		i = (limit - y + 11) / 11;          /* two lines, or three where no button is in the way */
		y += 11 * RBWrap(extra, r->left, y, w, i < 2 ? 2 : i > 3 ? 3 : i) + 3;
	}
	i = (limit - y + 11) / 11;              /* the note's lines, above the buttons */
	RBWrap(h->note, r->left, y, w, i < 0 ? 0 : i > 5 ? 5 : i);
	for (i = 0; i < 2; i++) {
		SetRect(&gButton[i], r->left, r->bottom - 44 + 24 * i, r->right, r->bottom - 24 + 24 * i);
		if (gButtonText[i][0]) {
			RBChrome();
			FrameRoundRect(&gButton[i], 12, 12);
			MoveTo((gButton[i].left + gButton[i].right - TextWidth(gButtonText[i], 0, strlen(gButtonText[i]))) / 2,
				gButton[i].bottom - 6);
			RBText(gButtonText[i]);
		}
	}
	RBBody();
}

static Boolean PanelClick(RBHead *h, Point pt)
{
	short i;
	Rect r;

	Buttons(h);
	for (i = 0; i < 2; i++)
		if (gButtonText[i][0] && PtInRect(pt, &gButton[i])) {
			r = gButton[i];
			InvertRoundRect(&r, 12, 12);
			while (StillDown())
				;
			InvertRoundRect(&r, 12, 12);
			GetMouse(&pt);
			if (!PtInRect(pt, &gButton[i]))
				return true;
			if (strcmp(gButtonText[i], "Accept draw") == 0) {
				h->pending = 'Y';
				Send(h, "DY");
			} else if (strcmp(gButtonText[i], "Decline") == 0) {
				h->offer = 0;
				h->noOffer = 2 - h->local;
				Send(h, "DN");
				h->note[0] = 0;
			} else if (strcmp(gButtonText[i], "Offer draw") == 0) {
				GameClear(h);
				h->offer = h->local + 1;
				h->armed = 0;
				h->note[0] = 0;
				Send(h, "D");
			} else if (!h->armed) {
				h->armed = 1;
				RBNote(h, "Click once more to resign.");
			} else {
				GameClear(h);
				h->armed = 0;
				h->pending = 'G';
				h->note[0] = 0;
				Send(h, "G");
			}
			return true;
		}
	return false;
}

/* ---- RULES ------------------------------------------------------------------------------------- */

static void Redraw(void)
{
	GrafPtr port;
	Rect r;

	RgnHandle clip = NewRgn();

	GetPort(&port);
	GetClip(clip);
	SetRect(&r, 0, 0, gPlayW, gPlayH);
	ClipRect(&r);
	EraseRect(&r);
	{
		Handle g = (Handle)GetWRefCon(port);
		if (g && *g) {
			SignedByte st = HGetState(g);
			HLock(g);
			GameDraw((RBHead *)*g);
			HSetState(g, st);
		}
	}
	SetClip(clip);
	DisposeRgn(clip);
	RBBody();
}

static pascal void DrawItem(WindowPtr w, short item)
{
	GrafPtr old;

	GetPort(&old);
	SetPort(w);
	Redraw();
	SetPort(old);
}

static void Attach(GameEvent *ev)
{
	GrafPtr port;
	short type;
	Handle item;
	Rect r;

	GetPort(&port);
	SetWRefCon(port, (long)ev->game);
	GetDialogItem((DialogPtr)port, 1, &type, &item, &r);
	SetDialogItem((DialogPtr)port, 1, type, (Handle)DrawItem, &r);
}

static void Fresh(RBHead *h, short local)
{
	memset(h, 0, GameSize());
	h->magic = gMagic;
	h->format = gFormat;
	h->local = local;
	GameInit(h);
}

pascal void RULES(GameEvent *ev)
{
	Handle g = ev->game;
	SignedByte st = 0;
	RBHead *h;
	short e = ev->event;

	RETRO68_RELOCATE();
	if (e == ge_InitRuleBook) {
		ev->int1 = gPlayW;
		ev->int2 = gPlayH;
		ev->globals = NULL;
		return;
	}
	if (e == ge_FinishRuleBook) {
		/* GameMaster closes the rulebook's file next, code and all */
		Retro68FreeGlobals();
		return;
	}
	if (!g)
		return;
	if (e == ge_NewGame || (e == ge_OldGame && GetHandleSize(g) != GameSize())) {
		SetHandleSize(g, GameSize());
		if (MemError() != noErr)
			return;
	}
	st = HGetState(g);
	HLock(g);
	h = (RBHead *)*g;
	switch (e) {
	case ge_NewGame:
		Fresh(h, 0);
		Attach(ev);
		break;
	case ge_OldGame:
		if (h->magic != gMagic || h->format != gFormat)
			Fresh(h, 0);
		h->connected = h->pending = h->outN = h->armed = h->posing = 0;
		h->note[0] = 0;
		GameClear(h);
		Attach(ev);
		break;
	case ge_Swap:
		h->local = 1 - h->local;
		GameClear(h);
		Redraw();
		break;
	case ge_Restart:
		{
			short local = h->local, connected = h->connected;
			Fresh(h, local);
			h->connected = connected;
		}
		ev->modified = false;
		Redraw();
		break;
	case ge_ConnectionMade:
		h->connected = 1;
		h->outN = h->pending = h->offer = h->armed = h->posing = 0;
		h->note[0] = 0;
		GameClear(h);
		Redraw();
		break;
	case ge_ConnectionLost:
		h->posing = 0;
		h->connected = h->pending = h->offer = h->outN = h->armed = 0;
		GameClear(h);
		if (!h->over)
			RBNote(h, "The connection has ended. Both sides now play on this Mac.");
		Redraw();
		break;
	case ge_MessageReceived:
		gQuiet = false;
		Received(h, ev->message);
		ev->modified = true;
		if (!gQuiet && !h->posing)
			Redraw();
		gQuiet = false;
		break;
	case ge_MouseDown:
		if (!PanelClick(h, ev->where)) {
			if (h->armed) {
				h->armed = 0;
				h->note[0] = 0;
			}
			GameClick(h, ev->where);
		}
		ev->modified = true;
		Redraw();
		break;
	case ge_KeyDown:
		GameKey(h, ev->int1);
		ev->modified = true;
		Redraw();
		break;
	case ge_SleepQuery:
		ev->int1 = h->outN ? 1 : 30;
		break;
	default:
		break;
	}
	if (e != ge_Flatten && e != ge_Close && e != ge_Activate && e != ge_Deactivate)
		ev->myturn = true;
	/* a finished game has nothing to save: GameMaster then closes it without asking */
	if (h->over && (e == ge_MessageReceived || e == ge_MouseDown || e == ge_Idle))
		ev->modified = false;
	if (h->outN && h->connected && (e == ge_MouseDown || e == ge_MessageReceived || e == ge_Idle || e == ge_KeyDown)) {
		short n = strlen(h->out[0]), i;
		ev->message[0] = n;
		memcpy(ev->message + 1, h->out[0], n);
		for (i = 1; i < h->outN; i++)
			strcpy(h->out[i - 1], h->out[i]);
		h->outN--;
		ev->event = ge_SendMessage;
	}
	HSetState(g, st);
}
