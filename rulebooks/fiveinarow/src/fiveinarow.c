/*
 * Five in a Row for GameMaster: ParlorNet's fiveinarow/1, ruleset freestyle, on
 * the standard 15 by 15 board (docs/types/fiveinarow-1.md in the ParlorNet repository).
 *
 * Black (seat 0) plays first and the players take turns, one stone each. A line
 * of five or more of a player's stones, across, down or diagonally, wins. A
 * full board with no such line is a draw.
 *
 * A move is a point: a column letter, a to o from the left, and a row number,
 * 1 to 15 from the top, like h8.
 *
 * A whole position from the parlor (rb.h) is BD and the 225 points row by row
 * from the top (0 empty, 1 Black, 2 White), and LS and the last stone.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <string.h>

#include "rb.h"

enum { kN = 15, kStep = 14, kLeft = 22, kTop = 20 };

const short gPlayW = 352, gPlayH = 226;
const OSType gMagic = 'PNfr';
const short gFormat = 1;
const Boolean gDraws = false;
const Boolean gParlorOnly = false;

typedef struct {
	RBHead h;
	unsigned char cells[kN * kN];   /* 1024: 0 empty, 1 Black, 2 White; row by row from the top */
	short last;                     /* 1249: the last stone, or -1 (at 1250, after a pad byte) */
	short winA, winB;               /* the winning line's ends, or -1 */
} Five;

long GameSize(void)
{
	return sizeof(Five);
}

const char *GameSeat(short seat)
{
	return seat ? "White" : "Black";
}

static void Name(short i, char *out)
{
	out[0] = 'a' + i % kN;
	out[1] = 0;
	RBNum(out, i / kN + 1);
}

void GameInit(RBHead *h)
{
	Five *g = (Five *)h;

	memset(g->cells, 0, sizeof g->cells);
	g->last = g->winA = g->winB = -1;
	h->turn = 0;
}

void GameClear(RBHead *h)
{
}

/* a line of seat's stones through point i, the first of five or more in the order across, down and the two
   diagonals, or else the longest: its ends in a and b, its length returned */
static short Run(const unsigned char *cells, short i, short seat, short *a, short *b)
{
	static const signed char d[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
	short k, sgn, x, y, best = 0, n, lo, hi;

	for (k = 0; k < 4; k++) {
		n = 1;
		lo = hi = i;
		for (sgn = 1; sgn >= -1; sgn -= 2) {
			x = i % kN + sgn * d[k][0];
			y = i / kN + sgn * d[k][1];
			while (x >= 0 && x < kN && y >= 0 && y < kN && cells[y * kN + x] == seat + 1) {
				n++;
				if (y * kN + x < lo)
					lo = y * kN + x;
				if (y * kN + x > hi)
					hi = y * kN + x;
				x += sgn * d[k][0];
				y += sgn * d[k][1];
			}
		}
		if (n > best) {
			best = n;
			*a = lo;
			*b = hi;
		}
		if (n >= 5)                         /* the first direction that wins, as the parlor reports it */
			return n;
	}
	return best;
}

Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	Five *g = (Five *)h;
	short col, row = 0, i, a = 0, b = 0, n[2] = { 0, 0 };
	const char *p = move + 1;
	char result[32], t[8];

	if (h->over) {
		strcpy(why, "the game is over");
		return false;
	}
	if (seat != h->turn) {
		strcpy(why, "it is not that side's turn");
		return false;
	}
	col = move[0] - 'a';
	while (*p >= '0' && *p <= '9')
		row = row * 10 + *p++ - '0';
	if (col < 0 || col >= kN || row < 1 || row > kN || *p || p == move + 1) {
		strcpy(why, "a move is a point, like h8");
		return false;
	}
	i = (row - 1) * kN + col;
	if (g->cells[i]) {
		strcpy(why, "that point is taken");
		return false;
	}
	g->cells[i] = seat + 1;
	g->last = i;
	h->turn = 1 - seat;
	if (Run(g->cells, i, seat, &a, &b) >= 5) {
		g->winA = a;
		g->winB = b;
		strcpy(result, "win ");
		RBNum(result, seat);
		strcat(result, " ");
		Name(a, t);
		strcat(result, t);
		strcat(result, "-");
		Name(b, t);
		strcat(result, t);
		RBEnd(h, result);
		return true;
	}
	for (i = 0; i < kN * kN; i++)
		if (g->cells[i])
			n[g->cells[i] - 1]++;
	if (n[0] + n[1] == kN * kN) {
		strcpy(result, "draw ");
		RBNum(result, n[0]);
		strcat(result, "-");
		RBNum(result, n[1]);
		RBEnd(h, result);
	}
	return true;
}

void GamePosition(RBHead *h)
{
	Five *g = (Five *)h;

	g->last = g->winA = g->winB = -1;
}

Boolean GameMessage(RBHead *h, const char *m)
{
	Five *g = (Five *)h;
	const char *p = m + 3;
	short i, row = 0;

	if (m[0] == 'B' && m[1] == 'D' && m[2] == ' ') {
		for (i = 0; i < kN * kN; i++)
			if (p[i] < '0' || p[i] > '2')
				return true;            /* not 225 points: the board stays */
		for (i = 0; i < kN * kN; i++)
			g->cells[i] = p[i] - '0';
		return true;
	}
	if (m[0] == 'L' && m[1] == 'S' && m[2] == ' ') {
		if (*p >= 'a' && *p < 'a' + kN) {
			i = *p++ - 'a';
			while (*p >= '0' && *p <= '9')
				row = row * 10 + *p++ - '0';
			if (row >= 1 && row <= kN)
				g->last = (row - 1) * kN + i;
		}
		return true;
	}
	return false;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	strcpy(out, GameSeat(seat));
	strcat(out, " wins with five in a row, ");
	strcat(out, how);
	strcat(out, ".");
}

/* ---- the board ----------------------------------------------------------------------------------- */

static Point At(short i)
{
	Point p;

	p.h = kLeft + (i % kN) * kStep;
	p.v = kTop + (i / kN) * kStep;
	return p;
}

void GameDraw(RBHead *h)
{
	Five *g = (Five *)h;
	short i, n[2] = { 0, 0 };
	Rect r, panel;
	Point p;
	char t[8], extra[48];

	for (i = 0; i < kN; i++) {
		MoveTo(kLeft, kTop + i * kStep);
		LineTo(kLeft + (kN - 1) * kStep, kTop + i * kStep);
		MoveTo(kLeft + i * kStep, kTop);
		LineTo(kLeft + i * kStep, kTop + (kN - 1) * kStep);
		t[0] = 'a' + i;
		t[1] = 0;
		MoveTo(kLeft + i * kStep - 2, kTop - 8);
		RBText(t);
		t[0] = 0;
		RBNum(t, i + 1);
		MoveTo(kLeft - 8 - TextWidth(t, 0, strlen(t)) + 2, kTop + i * kStep + 3);
		RBText(t);
	}
	for (i = 3; i < kN; i += 4)                 /* the star points */
		if (i != 15) {
			short j;
			for (j = 3; j < kN; j += 4) {
				p = At(j * kN + i);
				SetRect(&r, p.h - 1, p.v - 1, p.h + 2, p.v + 2);
				PaintRect(&r);
			}
		}
	for (i = 0; i < kN * kN; i++)
		if (g->cells[i]) {
			n[g->cells[i] - 1]++;
			p = At(i);
			SetRect(&r, p.h - 6, p.v - 6, p.h + 7, p.v + 7);
			if (g->cells[i] == 1)
				PaintOval(&r);
			else {
				EraseOval(&r);
				FrameOval(&r);
			}
			if (i == g->last) {
				InsetRect(&r, 4, 4);
				InvertOval(&r);
			}
		}
	if (g->winA >= 0) {
		Point a = At(g->winA), b = At(g->winB);
		PenSize(2, 2);
		PenMode(patXor);
		MoveTo(a.h - 1, a.v - 1);
		LineTo(b.h - 1, b.v - 1);
		PenNormal();
	}
	SetRect(&panel, kLeft + (kN - 1) * kStep + 16, 6, gPlayW - 6, gPlayH - 2);
	strcpy(extra, "Stones: Black ");
	RBNum(extra, n[0]);
	strcat(extra, ", White ");
	RBNum(extra, n[1]);
	if (g->last >= 0) {
		strcat(extra, ". Last: ");
		Name(g->last, t);
		strcat(extra, t);
	}
	RBDrawPanel(h, &panel, "Five in a Row", extra);
}

void GameClick(RBHead *h, Point pt)
{
	Five *g = (Five *)h;
	short col = (pt.h - kLeft + kStep / 2) / kStep, row = (pt.v - kTop + kStep / 2) / kStep;
	char move[8];

	if (pt.h < kLeft - kStep / 2 || pt.v < kTop - kStep / 2 || col >= kN || row >= kN)
		return;
	if (!RBCanMove(h)) {
		SysBeep(10);
		if (!h->over && !h->pending)
			RBNote(h, "Wait for your turn.");
		return;
	}
	if (g->cells[row * kN + col]) {
		SysBeep(10);
		RBNote(h, "That point is taken.");
		return;
	}
	Name(row * kN + col, move);
	h->note[0] = 0;
	RBPlay(h, move);
}
