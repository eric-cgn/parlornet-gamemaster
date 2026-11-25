/*
 * Mancala for GameMaster: ParlorNet's mancala/1, ruleset kalah with four seeds
 * a pit (docs/types/mancala-1.md in the ParlorNet repository).
 *
 * Two rows of six pits and a store for each player. South (seat 0) moves
 * first. A move picks up every seed in one of the mover's own pits and sows
 * them one by one counterclockwise: along the mover's row, into the mover's
 * store, along the other row, past the other store. A last seed in the
 * mover's store earns another move; a last seed in an empty pit of the mover's
 * own row captures it and the seeds opposite, when there are any. When either
 * row is empty, each player takes the seeds left on their own row and the
 * bigger store wins.
 *
 * A move is the pit's number, 1 to 6, from the mover's left.
 *
 * A whole position from the parlor (rb.h) is BD and the 14 counts (South's
 * pits and store, then North's) and LM and the last move, seat and pit.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <string.h>

#include "rb.h"

enum { kPit = 34, kGap = 4, kLeft = 6, kTop = 22, kRows = 2 };

const short gPlayW = 452, gPlayH = 170;
const OSType gMagic = 'PNmc';
const short gFormat = 1;
const Boolean gDraws = false;
const Boolean gParlorOnly = false;

typedef struct {
	RBHead h;
	unsigned char b[14];        /* 1024: seat 0's pits 1-6, its store, seat 1's pits 1-6, its store */
	unsigned char seeds;        /* 1038: seeds a pit started with */
	unsigned char lastSeat;     /* 1039: the last move, seat + 1 (0: none) */
	unsigned char lastPit;      /* 1040 */
	unsigned char pad;
} Mancala;

long GameSize(void)
{
	return sizeof(Mancala);
}

const char *GameSeat(short seat)
{
	return seat ? "North" : "South";
}

/* ---- the rules ----------------------------------------------------------------------------------- */

static short Store(short seat)
{
	return seat ? 13 : 6;
}

static short RowSum(const unsigned char *b, short seat)
{
	short i, n = 0;

	for (i = 0; i < 6; i++)
		n += b[seat * 7 + i];
	return n;
}

void GameInit(RBHead *h)
{
	Mancala *g = (Mancala *)h;
	short i;

	g->seeds = 4;
	for (i = 0; i < 14; i++)
		g->b[i] = i == 6 || i == 13 ? 0 : g->seeds;
	h->turn = 0;
}

void GameClear(RBHead *h)
{
}

Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	Mancala *g = (Mancala *)h;
	unsigned char b[14];
	short pit, start, n, pos, i;
	Boolean again = false;
	char result[32];

	if (h->over) {
		strcpy(why, "the game is over");
		return false;
	}
	if (seat != h->turn) {
		strcpy(why, "it is not that side's turn");
		return false;
	}
	if (move[0] < '1' || move[0] > '6' || move[1]) {
		strcpy(why, "a move is a pit, 1 to 6");
		return false;
	}
	pit = move[0] - '0';
	start = seat * 7 + pit - 1;
	memcpy(b, g->b, 14);
	n = b[start];
	if (!n) {
		strcpy(why, "that pit is empty");
		return false;
	}
	b[start] = 0;
	pos = start;
	while (n) {
		pos = (pos + 1) % 14;
		if (pos == Store(1 - seat))
			continue;
		b[pos]++;
		n--;
	}
	if (pos == Store(seat))
		again = true;
	else if (pos >= seat * 7 && pos < seat * 7 + 6 && b[pos] == 1 && b[12 - pos]) {
		b[Store(seat)] += b[pos] + b[12 - pos];
		b[pos] = b[12 - pos] = 0;
	}
	memcpy(g->b, b, 14);
	g->lastSeat = seat + 1;
	g->lastPit = pit;
	if (!RowSum(b, 0) || !RowSum(b, 1)) {
		for (i = 0; i < 2; i++) {
			g->b[Store(i)] += RowSum(g->b, i);
			memset(g->b + i * 7, 0, 6);
		}
		if (g->b[6] == g->b[13])
			strcpy(result, "draw ");
		else {
			strcpy(result, "win ");
			RBNum(result, g->b[13] > g->b[6]);
			strcat(result, " ");
		}
		RBNum(result, g->b[6]);
		strcat(result, "-");
		RBNum(result, g->b[13]);
		RBEnd(h, result);
		return true;
	}
	h->turn = again ? seat : 1 - seat;
	if (again && (!h->connected || seat == h->local))
		RBNote(h, "The last seed went into your store: move again.");
	return true;
}

void GamePosition(RBHead *h)
{
	((Mancala *)h)->lastSeat = 0;
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

Boolean GameMessage(RBHead *h, const char *m)
{
	Mancala *g = (Mancala *)h;
	const char *p = m + 3;
	short i, n, s;
	unsigned char b[14];

	if (m[0] == 'B' && m[1] == 'D' && m[2] == ' ') {
		for (i = 0; i < 14; i++) {
			n = Num(&p);
			if (n < 0 || n > 255)
				return true;            /* not 14 counts: the board stays */
			b[i] = n;
		}
		memcpy(g->b, b, 14);
		return true;
	}
	if (m[0] == 'L' && m[1] == 'M' && m[2] == ' ') {
		s = Num(&p);
		n = Num(&p);
		if ((s == 0 || s == 1) && n >= 1 && n <= 6) {
			g->lastSeat = s + 1;
			g->lastPit = n;
		}
		return true;
	}
	return false;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	strcpy(out, GameSeat(seat));
	strcat(out, " wins, ");
	strcat(out, how);
	strcat(out, ".");
}

/* ---- the board ----------------------------------------------------------------------------------- */

/* the place in b drawn at column col (0-5) of row (0: top, 1: bottom); the bottom row is the player's */
static short PlaceAt(RBHead *h, short row, short col)
{
	short bottom = RBBottom(h);

	return row ? bottom * 7 + col : (1 - bottom) * 7 + 5 - col;
}

static void PitRect(short row, short col, Rect *r)
{
	short x = kLeft + kPit + kGap + col * (kPit + kGap), y = kTop + row * (kPit + 18);

	SetRect(r, x, y, x + kPit, y + kPit);
}

static void StoreRect(short right, Rect *r)
{
	short x = right ? kLeft + kPit + kGap + 6 * (kPit + kGap) : kLeft;

	SetRect(r, x, kTop, x + kPit, kTop + 2 * kPit + 18);
}

static void Count(const Rect *r, short n)
{
	char t[8];

	t[0] = 0;
	RBNum(t, n);
	RBChrome();
	TextFace(bold);
	MoveTo((r->left + r->right - TextWidth(t, 0, strlen(t))) / 2, (r->top + r->bottom) / 2 + 5);
	RBText(t);
	RBBody();
}

void GameDraw(RBHead *h)
{
	Mancala *g = (Mancala *)h;
	short row, col, bottom = RBBottom(h), i;
	Rect r, panel;
	char label[4], extra[64];

	for (i = 0; i < 2; i++) {                   /* the top player's store on the left, the bottom's on the right */
		StoreRect(i, &r);
		FrameRoundRect(&r, 16, 16);
		Count(&r, g->b[Store(i ? bottom : 1 - bottom)]);
	}
	for (row = 0; row < kRows; row++)
		for (col = 0; col < 6; col++) {
			short at = PlaceAt(h, row, col);
			PitRect(row, col, &r);
			FrameOval(&r);
			Count(&r, g->b[at]);
			if (g->lastSeat && at == (g->lastSeat - 1) * 7 + g->lastPit - 1) {
				InsetRect(&r, -2, -2);
				FrameOval(&r);
			}
			label[0] = '1' + (row ? col : 5 - col);
			label[1] = 0;
			MoveTo((r.left + r.right) / 2 - 2, row ? r.bottom + 12 : r.top - 4);
			RBText(label);
		}
	MoveTo(kLeft, gPlayH - 8);
	RBText(GameSeat(1 - bottom));
	RBText(" plays the top row; ");
	RBText(GameSeat(bottom));
	RBText(" the bottom.");
	SetRect(&panel, kLeft + 8 * (kPit + kGap) + 8, kTop - 16, gPlayW - 6, gPlayH - 2);
	extra[0] = 0;
	strcat(extra, "Stores: South ");
	RBNum(extra, g->b[6]);
	strcat(extra, ", North ");
	RBNum(extra, g->b[13]);
	RBDrawPanel(h, &panel, "Mancala", extra);
}

void GameClick(RBHead *h, Point pt)
{
	Mancala *g = (Mancala *)h;
	short col, row, seat = RBMover(h), mine = seat == RBBottom(h) ? 1 : 0, pit;
	Rect r;
	char move[2];

	for (row = 0; row < kRows; row++)
		for (col = 0; col < 6; col++) {
			PitRect(row, col, &r);
			if (PtInRect(pt, &r))
				goto found;
		}
	return;
found:
	if (!RBCanMove(h)) {
		SysBeep(10);
		if (!h->over && !h->pending)
			RBNote(h, "Wait for your turn.");
		return;
	}
	if (row != mine) {
		SysBeep(10);
		RBNote(h, mine ? "Your pits are the bottom row." : "It is North's turn: play from the top row.");
		return;
	}
	pit = mine ? col + 1 : 6 - col;
	if (!g->b[seat * 7 + pit - 1]) {
		SysBeep(10);
		RBNote(h, "That pit is empty.");
		return;
	}
	move[0] = '0' + pit;
	move[1] = 0;
	h->note[0] = 0;
	RBPlay(h, move);
}
