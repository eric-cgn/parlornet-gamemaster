/*
 * Grid Words for GameMaster: ParlorNet's gridwords/1, ruleset classic, for two
 * players (docs/types/gridwords-1.md in the ParlorNet repository).
 *
 * A 15 by 15 board with the usual premium squares, a rack of seven tiles each,
 * and a bag the parlor keeps: the parlor deals, checks every word against its
 * word list and keeps the score, so this rulebook plays only on a parlor. It
 * draws the board, the rack and the scores the parlor sends, and builds a play
 * by clicks or typing.
 *
 * Moves are the parlor's own: PLAY x y H|V letters (the whole main word from x y,
 * across or down, the letters already on the board written out and _ before a
 * blank's letter), EXCH letters, PASS. Besides the messages of rb.h the parlor
 * sends K and the rack (_ a blank, - none), S and the two scores, and B and the
 * tiles left in the bag. A whole position (rb.h) is BD and the 225 squares row
 * by row (. empty, a letter, a blank's letter in lower case), K, S, B, and LP
 * x y H|V length for the last play.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <Events.h>
#include <string.h>

#include "rb.h"

enum { kN = 15, kSq = 15, kLeft = 6, kTop = 6, kTile = 22, kRackX = 246, kRackY = 152 };
enum { mBuild, mExchange };
enum { bPlay, bClear, bExchange, bPass, kButtons };

const short gPlayW = 432, gPlayH = 236;
const OSType gMagic = 'PNgw';
const short gFormat = 1;
const Boolean gDraws = false;
const Boolean gParlorOnly = true;

typedef struct {
	RBHead h;
	char board[kN * kN];        /* 1024: 0 empty, 'A'-'Z' a tile, 'a'-'z' a blank standing for that letter */
	char rack[8];               /* 1249: the rack the parlor sent, _ a blank */
	unsigned char dealt;        /* 1257: the parlor has sent a rack */
	unsigned char mode;         /* 1258: building a play, or marking tiles to exchange */
	short score[2];             /* 1260 */
	short bag;
	short start;                /* the square a play starts from, or -1 */
	unsigned char across;       /* the direction of the play */
	unsigned char placedN;
	short placedSq[7];          /* the play being built: its squares, */
	unsigned char placedSlot[7];    /* the rack's tiles on them, */
	char placedAs[7];           /* and the letters they stand for (a blank's chosen letter) */
	unsigned char marked;       /* exchange: a bit for each rack tile */
	unsigned char waitBlank;    /* a blank was clicked: the next letter typed is its letter */
	short lastSq, lastLen;      /* the last word played, to mark it */
	unsigned char lastAcross;
	unsigned char pad;
} Grid;

static const Pattern kTL = { { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 } };
static const char kValues[26] = { 1, 3, 3, 2, 1, 4, 2, 4, 1, 8, 5, 1, 3, 1, 1, 3, 10, 1, 1, 1, 1, 4, 4, 8, 4, 10 };

static Rect gButtonRect[kButtons];

long GameSize(void)
{
	return sizeof(Grid);
}

const char *GameSeat(short seat)
{
	return seat ? "Player 2" : "Player 1";
}

/* ---- the board ----------------------------------------------------------------------------------- */

/* 'T' 'D' (triple and double word), 't' 'd' (triple and double letter), or 0; x and y from 1 */
static char Premium(short x, short y)
{
	static const unsigned char tw[][2] = { { 1, 1 }, { 8, 1 } };
	static const unsigned char dw[][2] = { { 2, 2 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 8, 8 } };
	static const unsigned char tl[][2] = { { 6, 2 }, { 6, 6 } };
	static const unsigned char dl[][2] = { { 4, 1 }, { 7, 3 }, { 8, 4 }, { 7, 7 } };
	short a = x > 8 ? 16 - x : x, b = y > 8 ? 16 - y : y, i;

	for (i = 0; i < 2; i++)
		if ((tw[i][0] == a && tw[i][1] == b) || (tw[i][0] == b && tw[i][1] == a))
			return 'T';
	for (i = 0; i < 5; i++)
		if ((dw[i][0] == a && dw[i][1] == b) || (dw[i][0] == b && dw[i][1] == a))
			return 'D';
	for (i = 0; i < 2; i++)
		if ((tl[i][0] == a && tl[i][1] == b) || (tl[i][0] == b && tl[i][1] == a))
			return 't';
	for (i = 0; i < 4; i++)
		if ((dl[i][0] == a && dl[i][1] == b) || (dl[i][0] == b && dl[i][1] == a))
			return 'd';
	return 0;
}

static char Upper(char c)
{
	return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
}

/* the letter on a square, with the play being built: 0 for none */
static char On(Grid *g, short sq)
{
	short i;

	if (g->board[sq])
		return g->board[sq];
	for (i = 0; i < g->placedN; i++)
		if (g->placedSq[i] == sq)
			return g->rack[g->placedSlot[i]] == '_' ? g->placedAs[i] - 'A' + 'a' : g->placedAs[i];
	return 0;
}

void GameInit(RBHead *h)
{
	Grid *g = (Grid *)h;

	memset(g->board, 0, sizeof g->board);
	g->rack[0] = 0;
	g->start = g->lastSq = -1;
	g->across = 1;
	h->turn = 0;
}

void GameClear(RBHead *h)
{
	Grid *g = (Grid *)h;

	g->placedN = 0;
	g->marked = 0;
	g->mode = mBuild;
	g->waitBlank = 0;
}

static short Num(const char **p)
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

/* PLAY x y H|V letters (_ before a blank's letter, or a blank's letter in lower case), EXCH ..., PASS */
Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	Grid *g = (Grid *)h;
	const char *p = move + 5;
	short x, y, dx, sq, i;
	char d, word[16], c;
	Boolean blank;
	short n = 0;

	if (h->over) {
		strcpy(why, "the game is over");
		return false;
	}
	if (seat != h->turn) {
		strcpy(why, "it is not that side's turn");
		return false;
	}
	if (strncmp(move, "PASS", 4) == 0 || strncmp(move, "EXCH", 4) == 0) {
		h->turn = 1 - seat;
		return true;
	}
	if (strncmp(move, "PLAY ", 5) != 0 || (x = Num(&p)) < 1 || x > kN || (y = Num(&p)) < 1 || y > kN) {
		strcpy(why, "a move is PLAY x y H|V letters, EXCH or PASS");
		return false;
	}
	while (*p == ' ')
		p++;
	d = Upper(*p++);
	if ((d != 'H' && d != 'V') || *p++ != ' ') {
		strcpy(why, "the direction is H or V");
		return false;
	}
	dx = d == 'H' ? 1 : kN;
	sq = (y - 1) * kN + x - 1;
	while (*p && n < kN) {
		blank = false;
		if (*p == '_') {
			blank = true;
			p++;
		}
		c = *p++;
		if (c >= 'a' && c <= 'z') {
			blank = true;
			c = Upper(c);
		}
		if (c < 'A' || c > 'Z' || (d == 'H' ? (x - 1 + n >= kN) : (y - 1 + n >= kN))) {
			strcpy(why, "the word does not fit");
			return false;
		}
		word[n++] = blank ? c - 'A' + 'a' : c;
	}
	for (i = 0; i < n; i++) {
		char on = g->board[sq + i * dx];
		if (on && Upper(on) != Upper(word[i])) {
			strcpy(why, "the word does not match the board");
			return false;
		}
	}
	for (i = 0; i < n; i++)
		if (!g->board[sq + i * dx]) {
			g->board[sq + i * dx] = word[i];
			if (seat == h->local && g->dealt) {         /* the tile leaves the rack until the parlor's K */
				char *r = strchr(g->rack, word[i] >= 'a' ? '_' : word[i]);
				if (r)
					memmove(r, r + 1, strlen(r));
			}
		}
	g->lastSq = sq;
	g->lastLen = n;
	g->lastAcross = d == 'H';
	g->placedN = 0;
	g->start = -1;
	h->turn = 1 - seat;
	return true;
}

static Boolean PositionLine(Grid *g, const char *m);

Boolean GameMessage(RBHead *h, const char *m)
{
	Grid *g = (Grid *)h;
	const char *p = m + 2;

	if (m[0] == 'K' && m[1] == ' ') {
		strncpy(g->rack, strcmp(p, "-") == 0 ? "" : p, 7);
		g->rack[7] = 0;
		g->dealt = 1;
		GameClear(h);
		return true;
	}
	if (m[0] == 'S' && m[1] == ' ') {
		g->score[0] = Num(&p);
		g->score[1] = Num(&p);
		return true;
	}
	if (m[0] == 'B' && m[1] == ' ') {
		g->bag = Num(&p);
		return true;
	}
	return PositionLine(g, m);
}

void GamePosition(RBHead *h)
{
	Grid *g = (Grid *)h;

	g->start = g->lastSq = -1;
	g->across = 1;
}

/* BD and the board, LP and the last play: a whole position's own lines */
static Boolean PositionLine(Grid *g, const char *m)
{
	const char *p = m + 3;
	short i, x, y, n;
	char c;

	if (m[0] == 'B' && m[1] == 'D' && m[2] == ' ') {
		for (i = 0; i < kN * kN; i++)
			if (p[i] != '.' && !(p[i] >= 'A' && p[i] <= 'Z') && !(p[i] >= 'a' && p[i] <= 'z'))
				return true;            /* not 225 squares: the board stays */
		for (i = 0; i < kN * kN; i++)
			g->board[i] = p[i] == '.' ? 0 : p[i];
		return true;
	}
	if (m[0] == 'L' && m[1] == 'P' && m[2] == ' ') {
		x = Num(&p);
		y = Num(&p);
		while (*p == ' ')
			p++;
		c = *p++;
		n = Num(&p);
		if (x >= 1 && x <= kN && y >= 1 && y <= kN && (c == 'H' || c == 'V') && n >= 1
			&& (c == 'H' ? x : y) - 1 + n <= kN) {
			g->lastSq = (y - 1) * kN + x - 1;
			g->lastLen = n;
			g->lastAcross = c == 'H';
		}
		return true;
	}
	return false;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	const char *p = how;

	strcpy(out, GameSeat(seat));
	strcat(out, " wins. Scores ");
	while (*p && *p != '/')
		p++;
	strncat(out, how, p - how);
	if (*p) {
		strcat(out, " and ");
		strcat(out, p + 1);
	}
	strcat(out, ".");
}

/* ---- drawing ------------------------------------------------------------------------------------- */

static void SquareRect(short sq, Rect *r)
{
	SetRect(r, kLeft + (sq % kN) * kSq, kTop + (sq / kN) * kSq, kLeft + (sq % kN + 1) * kSq + 1,
		kTop + (sq / kN + 1) * kSq + 1);
}

static void Letter(const Rect *r, char c, Boolean big)
{
	char t[2];

	t[0] = Upper(c);
	t[1] = 0;
	if (big)
		RBChrome();
	TextFace(c >= 'a' && c <= 'z' ? 0 : bold);
	MoveTo((r->left + r->right - TextWidth(t, 0, 1)) / 2, (r->top + r->bottom) / 2 + (big ? 5 : 4));
	RBText(t);
	RBBody();
}

static void DrawButton(short i, const char *label, short x, short y)
{
	SetRect(&gButtonRect[i], x, y, x + 88, y + 18);
	if (!label) {
		gButtonRect[i].right = gButtonRect[i].left;
		return;
	}
	RBChrome();
	FrameRoundRect(&gButtonRect[i], 10, 10);
	MoveTo((gButtonRect[i].left + gButtonRect[i].right - TextWidth((Ptr)label, 0, strlen(label))) / 2, y + 13);
	RBText(label);
	RBBody();
}

static Boolean Used(Grid *g, short slot)
{
	short i;

	for (i = 0; i < g->placedN; i++)
		if (g->placedSlot[i] == slot)
			return true;
	return false;
}

void GameDraw(RBHead *h)
{
	Grid *g = (Grid *)h;
	short sq, i, n = strlen(g->rack);
	Rect r, panel;
	char c, extra[64], t[8];
	Boolean mine = h->connected && h->turn == h->local && !h->over;

	for (sq = 0; sq < kN * kN; sq++) {
		SquareRect(sq, &r);
		c = On(g, sq);
		if (c) {
			EraseRect(&r);
			FrameRect(&r);
			Letter(&r, c, false);
			if (!g->board[sq])
				InvertRect(&r);         /* a tile of the play being built */
		} else {
			char k = Premium(sq % kN + 1, sq / kN + 1);
			FrameRect(&r);
			if (k) {                    /* the premium in words: 3W, 2W, 3L, 2L */
				char t[3];
				t[0] = k == 'T' || k == 't' ? '3' : '2';
				t[1] = k == 'T' || k == 'D' ? 'W' : 'L';
				t[2] = 0;
				MoveTo((r.left + r.right - TextWidth(t, 0, 2)) / 2, r.bottom - 4);
				RBText(t);
			}
		}
	}
	if (g->lastSq >= 0 && !g->placedN) {
		Rect a, b;
		SquareRect(g->lastSq, &a);
		SquareRect(g->lastSq + (g->lastLen - 1) * (g->lastAcross ? 1 : kN), &b);
		UnionRect(&a, &b, &r);
		InsetRect(&r, -1, -1);
		FrameRect(&r);
	}
	if (g->start >= 0 && mine && g->mode == mBuild && !g->placedN) {     /* where the play starts, and its direction */
		PolyHandle tri;
		short x, y;
		SquareRect(g->start, &r);
		x = r.left;
		y = r.top;
		tri = OpenPoly();
		if (g->across) {
			MoveTo(x + 4, y + 3);
			LineTo(x + 12, y + 7);
			LineTo(x + 4, y + 11);
		} else {
			MoveTo(x + 3, y + 4);
			LineTo(x + 11, y + 4);
			LineTo(x + 7, y + 12);
		}
		ClosePoly();
		InvertPoly(tri);
		KillPoly(tri);
	}
	/* the panel, the rack and the buttons */
	SetRect(&panel, kRackX, kTop, gPlayW - 6, kRackY - 8);
	strcpy(extra, "Scores: ");
	RBNum(extra, g->score[0]);
	strcat(extra, " and ");
	RBNum(extra, g->score[1]);
	strcat(extra, ". Bag ");
	RBNum(extra, g->bag);
	strcat(extra, ".");
	RBDrawPanel(h, &panel, "Grid Words", extra);
	for (i = 0; i < 7; i++) {
		SetRect(&r, kRackX + i * (kTile + 4), kRackY, kRackX + i * (kTile + 4) + kTile, kRackY + kTile);
		if (i >= n || Used(g, i)) {
			PenPat(&kTL);
			FrameRect(&r);
			PenNormal();
			continue;
		}
		EraseRect(&r);
		FrameRect(&r);
		if (g->rack[i] != '_') {
			Letter(&r, g->rack[i], true);
			t[0] = 0;
			RBNum(t, kValues[g->rack[i] - 'A']);
			MoveTo(r.right - 2 - TextWidth(t, 0, strlen(t)), r.bottom - 2);
			TextSize(9);
			RBText(t);
			RBBody();
		}
		if (g->mode == mExchange && (g->marked & (1 << i)))
			InvertRect(&r);
	}
	if (mine && g->dealt) {
		DrawButton(bPlay, g->mode == mExchange ? NULL : "Play", kRackX, kRackY + 30);
		DrawButton(bClear, "Clear", kRackX + 94, kRackY + 30);
		DrawButton(bExchange, g->mode == mExchange ? "Exchange" : "Exchange...", kRackX, kRackY + 54);
		DrawButton(bPass, g->mode == mExchange ? NULL : "Pass", kRackX + 94, kRackY + 54);
	} else
		for (i = 0; i < kButtons; i++)
			DrawButton(i, NULL, 0, 0);
}

/* ---- building a play ----------------------------------------------------------------------------- */

static Boolean Mine(RBHead *h)
{
	Grid *g = (Grid *)h;

	if (!h->connected) {
		RBNote(h, "Grid Words plays on a ParlorNet parlor, which deals the tiles: start it from the ParlorNet "
			"window.");
		return false;
	}
	if (!g->dealt) {
		RBNote(h, "Waiting for the parlor to deal.");
		return false;
	}
	if (!RBCanMove(h)) {
		if (!h->over && !h->pending)
			RBNote(h, "Wait for your turn.");
		return false;
	}
	return true;
}

/* the next empty square from the start, along the play, or -1 */
static short NextSquare(Grid *g)
{
	short sq = g->start, step = g->across ? 1 : kN;

	if (sq < 0)
		return -1;
	while (On(g, sq)) {
		if (g->across ? sq % kN == kN - 1 : sq / kN == kN - 1)
			return -1;
		sq += step;
	}
	return sq;
}

static void Place(RBHead *h, short slot, char as)
{
	Grid *g = (Grid *)h;
	short sq = NextSquare(g);

	if (sq < 0) {
		SysBeep(10);
		RBNote(h, g->start < 0 ? "Click the square the word starts on first." : "The word runs off the board.");
		return;
	}
	g->placedSq[g->placedN] = sq;
	g->placedSlot[g->placedN] = slot;
	g->placedAs[g->placedN] = as;
	g->placedN++;
	h->note[0] = 0;
}

/* the play built, as the parlor takes it: PLAY x y H|V and the whole main word */
static Boolean Token(Grid *g, char *out)
{
	short step, first, last, sq, i;
	Boolean across = g->across;
	char c;

	if (!g->placedN)
		return false;
	step = across ? 1 : kN;
	first = last = g->placedSq[0];
	for (i = 1; i < g->placedN; i++) {
		if (g->placedSq[i] < first)
			first = g->placedSq[i];
		if (g->placedSq[i] > last)
			last = g->placedSq[i];
	}
	while ((across ? first % kN : first / kN) > 0 && On(g, first - step))
		first -= step;
	while ((across ? last % kN : last / kN) < kN - 1 && On(g, last + step))
		last += step;
	strcpy(out, "PLAY ");
	RBNum(out, first % kN + 1);
	strcat(out, " ");
	RBNum(out, first / kN + 1);
	strcat(out, across ? " H " : " V ");
	for (sq = first; sq <= last; sq += step) {
		short n = strlen(out);
		c = On(g, sq);
		if (!c)
			return false;           /* a gap: the parlor would refuse it */
		if (c >= 'a' && c <= 'z') {
			out[n++] = '_';
			c = Upper(c);
		}
		out[n++] = c;
		out[n] = 0;
	}
	return true;
}

static void Play(RBHead *h)
{
	Grid *g = (Grid *)h;
	char move[64];

	if (!Token(g, move)) {
		SysBeep(10);
		RBNote(h, g->placedN ? "The tiles must make one word, with no gap." : "Place some tiles first.");
		return;
	}
	g->start = -1;
	RBPlay(h, move);
}

void GameClick(RBHead *h, Point pt)
{
	Grid *g = (Grid *)h;
	short i, sq;
	Rect r;

	for (i = 0; i < kButtons; i++)
		if (gButtonRect[i].right > gButtonRect[i].left && PtInRect(pt, &gButtonRect[i])) {
			r = gButtonRect[i];
			InvertRoundRect(&r, 10, 10);
			while (StillDown())
				;
			InvertRoundRect(&r, 10, 10);
			if (!Mine(h))
				return;
			if (i == bPlay)
				Play(h);
			else if (i == bClear) {
				GameClear(h);
				h->note[0] = 0;
			} else if (i == bPass) {
				GameClear(h);
				RBPlay(h, "PASS");
			} else if (g->mode != mExchange) {
				GameClear(h);
				g->mode = mExchange;
				RBNote(h, g->bag < 7 ? "The bag holds fewer than seven tiles: no exchange now."
					: "Click the tiles to put back, then Exchange.");
			} else {
				char move[16];
				short n = 5;
				strcpy(move, "EXCH ");
				for (i = 0; i < 7 && g->rack[i]; i++)
					if (g->marked & (1 << i))
						move[n++] = g->rack[i];
				move[n] = 0;
				if (n == 5) {
					SysBeep(10);
					RBNote(h, "Click the tiles to put back first.");
					return;
				}
				RBPlay(h, move);
			}
			return;
		}
	if (pt.h >= kRackX && pt.v >= kRackY && pt.v < kRackY + kTile) {     /* the rack */
		i = (pt.h - kRackX) / (kTile + 4);
		if (i >= 7 || i >= (short)strlen(g->rack) || !Mine(h))
			return;
		if (g->mode == mExchange) {
			g->marked ^= 1 << i;
			return;
		}
		if (Used(g, i)) {
			SysBeep(10);
			return;
		}
		if (g->rack[i] == '_') {
			if (NextSquare(g) < 0) {
				Place(h, i, 'A');
				return;
			}
			g->waitBlank = i + 1;
			RBNote(h, "Type the letter the blank stands for.");
			return;
		}
		Place(h, i, g->rack[i]);
		return;
	}
	if (pt.h < kLeft || pt.v < kTop || pt.h >= kLeft + kN * kSq || pt.v >= kTop + kN * kSq)
		return;
	sq = (pt.v - kTop) / kSq * kN + (pt.h - kLeft) / kSq;
	if (!Mine(h) || g->mode == mExchange)
		return;
	for (i = 0; i < g->placedN; i++)
		if (g->placedSq[i] == sq) {         /* a tile of the play: it goes back, with those after it */
			g->placedN = i;
			return;
		}
	if (g->board[sq]) {
		SysBeep(10);
		return;
	}
	if (g->placedN) {
		SysBeep(10);
		RBNote(h, "Clear the tiles placed to start somewhere else.");
		return;
	}
	if (sq == g->start)
		g->across = !g->across;
	else {
		g->start = sq;
		g->across = 1;
	}
	RBNote(h, g->across ? "Across: click tiles in the rack, or type letters." :
		"Down: click tiles in the rack, or type letters.");
}

void GameKey(RBHead *h, short ch)
{
	Grid *g = (Grid *)h;
	short i;
	char c = Upper((char)ch);

	if (!h->connected || !g->dealt || !RBCanMove(h) || g->mode == mExchange)
		return;
	if (ch == 8) {
		if (g->waitBlank)
			g->waitBlank = 0;
		else if (g->placedN)
			g->placedN--;
		return;
	}
	if (ch == '\r' || ch == 3) {
		Play(h);
		return;
	}
	if (c < 'A' || c > 'Z')
		return;
	if (g->waitBlank) {
		short slot = g->waitBlank - 1;
		g->waitBlank = 0;
		Place(h, slot, c);
		return;
	}
	for (i = 0; i < 7 && g->rack[i]; i++)
		if (g->rack[i] == c && !Used(g, i)) {
			Place(h, i, c);
			return;
		}
	for (i = 0; i < 7 && g->rack[i]; i++)
		if (g->rack[i] == '_' && !Used(g, i)) {
			Place(h, i, c);
			return;
		}
	SysBeep(10);
}
