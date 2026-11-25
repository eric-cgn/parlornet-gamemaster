/*
 * Checkers for GameMaster: ParlorNet's checkers/1, ruleset american
 * (docs/types/checkers-1.md in the ParlorNet repository).
 *
 * The 32 dark squares are numbered 1 to 32 in the standard way, 1-4 on the top
 * row as White sees the board. Black (seat 0) starts on 1-12, moves towards 32
 * and moves first; White (seat 1) starts on 21-32. A capture is compulsory, a
 * line of jumps goes on to its end, and a man that reaches the far row is
 * crowned and ends the turn there. A side with no piece or no move loses; forty
 * moves each with no capture and no man moving is a draw.
 *
 * A move is the squares it passes, joined by - for a step and x for jumps:
 * 11-15, 15x22, 15x22x31. The player clicks the piece, then each square it
 * lands on.
 *
 * A whole position from the parlor (rb.h) is BD and the 32 squares as the
 * record keeps them, and L and the turns without a capture or a man moving
 * and their limit.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <string.h>

#include "rb.h"

enum { kSq = 26, kLeft = 6, kTop = 6, kLimit = 80 };

const short gPlayW = 352, gPlayH = 220;
const OSType gMagic = 'PNck';
const short gFormat = 1;
const Boolean gDraws = true;
const Boolean gParlorOnly = false;

typedef struct {
	RBHead h;
	char cells[32];             /* 1024: . x X o O */
	short quiet;                /* 1056: turns in a row with no capture and no man moving */
	short limit;                /* 1058: the turns that make a draw (0: none) */
	unsigned char path[16];     /* 1060: the move being made by clicks */
	short pathN;                /* 1076 */
	unsigned char last[16];     /* 1078: the last move played, to mark it */
	short lastN;                /* 1094 */
} Checkers;

static const Pattern kDark = { { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 } };
static const Pattern kDots = { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 } };

long GameSize(void)
{
	return sizeof(Checkers);
}

const char *GameSeat(short seat)
{
	return seat ? "White" : "Black";
}

/* ---- the rules ----------------------------------------------------------------------------------- */

static short Row(short s)
{
	return (s - 1) / 4;
}

static short Col(short s)
{
	short r = Row(s);
	return 2 * ((s - 1) % 4) + (r % 2 == 0 ? 1 : 0);
}

static short Sq(short r, short c)
{
	if (r < 0 || r > 7 || c < 0 || c > 7 || (r + c) % 2 == 0)
		return 0;
	return r * 4 + c / 2 + 1;
}

static short Owner(char ch)
{
	return ch == 'x' || ch == 'X' ? 0 : ch == 'o' || ch == 'O' ? 1 : -1;
}

static Boolean King(char ch)
{
	return ch == 'X' || ch == 'O';
}

/* may the piece ch move by dr rows? */
static Boolean Goes(char ch, short dr)
{
	return King(ch) || dr == (Owner(ch) == 0 ? 1 : -1);
}

/* the square jumped over from a to b, or 0 if a to b is no jump for the piece on a */
static short Over(const char *cells, short a, short b)
{
	short dr = Row(b) - Row(a), dc = Col(b) - Col(a), m;
	char ch = cells[a - 1];

	if ((dr != 2 && dr != -2) || (dc != 2 && dc != -2) || !Goes(ch, dr / 2) || cells[b - 1] != '.')
		return 0;
	m = Sq(Row(a) + dr / 2, Col(a) + dc / 2);
	return m && Owner(cells[m - 1]) == 1 - Owner(ch) ? m : 0;
}

static Boolean Slides(const char *cells, short a, short b)
{
	short dr = Row(b) - Row(a), dc = Col(b) - Col(a);

	return (dr == 1 || dr == -1) && (dc == 1 || dc == -1) && Goes(cells[a - 1], dr) && cells[b - 1] == '.';
}

static Boolean CanJump(const char *cells, short s)
{
	static const signed char d[4][2] = { { 2, 2 }, { 2, -2 }, { -2, 2 }, { -2, -2 } };
	short i, t;

	for (i = 0; i < 4; i++)
		if ((t = Sq(Row(s) + d[i][0], Col(s) + d[i][1])) && Over(cells, s, t))
			return true;
	return false;
}

static Boolean CanSlide(const char *cells, short s)
{
	static const signed char d[4][2] = { { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };
	short i, t;

	for (i = 0; i < 4; i++)
		if ((t = Sq(Row(s) + d[i][0], Col(s) + d[i][1])) && Slides(cells, s, t))
			return true;
	return false;
}

static Boolean AnyJump(const char *cells, short seat)
{
	short s;

	for (s = 1; s <= 32; s++)
		if (Owner(cells[s - 1]) == seat && CanJump(cells, s))
			return true;
	return false;
}

static Boolean AnyMove(const char *cells, short seat)
{
	short s;

	for (s = 1; s <= 32; s++)
		if (Owner(cells[s - 1]) == seat && (CanJump(cells, s) || CanSlide(cells, s)))
			return true;
	return false;
}

/* Seat's move through the n squares sq on cells (changed): 1 complete, 2 legal so far but the jumps must go
   on, 0 not legal (why says so). progress: a capture or a man moved. */
static short Walk(char *cells, short seat, const unsigned char *sq, short n, char *why, Boolean *progress)
{
	short i, a, b, m;
	Boolean mustJump = AnyJump(cells, seat), jumped = false;
	char ch;

	if (n < 2) {
		strcpy(why, "a move is at least two squares");
		return 0;
	}
	if (Owner(cells[sq[0] - 1]) != seat) {
		strcpy(why, "that is not your piece");
		return 0;
	}
	*progress = !King(cells[sq[0] - 1]);
	for (i = 0; i + 1 < n; i++) {
		a = sq[i];
		b = sq[i + 1];
		ch = cells[a - 1];
		m = Over(cells, a, b);
		if (m) {
			cells[m - 1] = '.';
			jumped = *progress = true;
		} else if (i == 0 && Slides(cells, a, b)) {
			if (mustJump) {
				strcpy(why, "a capture is compulsory");
				return 0;
			}
		} else {
			strcpy(why, i ? "the jumps go on from square to square" : "that piece cannot go there");
			return 0;
		}
		cells[a - 1] = '.';
		cells[b - 1] = ch;
		if (!King(ch) && Row(b) == (seat == 0 ? 7 : 0)) {
			cells[b - 1] = seat == 0 ? 'X' : 'O';
			if (i + 2 < n) {
				strcpy(why, "a man that is crowned ends the move");
				return 0;
			}
			return 1;
		}
		if (!m && i + 2 < n) {
			strcpy(why, "a step is one square");
			return 0;
		}
	}
	if (jumped && CanJump(cells, sq[n - 1]))
		return 2;
	return 1;
}

/* "15x22x31" -> squares; 0 if it is no move */
static short Parse(const char *move, unsigned char *sq, short max)
{
	short n = 0, v;

	while (*move) {
		if (*move < '0' || *move > '9' || n == max)
			return 0;
		v = 0;
		while (*move >= '0' && *move <= '9')
			v = v * 10 + *move++ - '0';
		if (v < 1 || v > 32)
			return 0;
		sq[n++] = v;
		if (*move == '-' || *move == 'x')
			move++;
		else if (*move)
			return 0;
	}
	return n;
}

static void Format(const unsigned char *sq, short n, const char *cells, char *out)
{
	short i;

	out[0] = 0;
	for (i = 0; i < n; i++) {
		if (i)
			strcat(out, Row(sq[i]) - Row(sq[i - 1]) == 1 || Row(sq[i]) - Row(sq[i - 1]) == -1 ? "-" : "x");
		RBNum(out, sq[i]);
	}
}

void GameInit(RBHead *h)
{
	Checkers *g = (Checkers *)h;

	memset(g->cells, 'x', 12);
	memset(g->cells + 12, '.', 8);
	memset(g->cells + 20, 'o', 12);
	g->limit = kLimit;
	h->turn = 0;
}

void GameClear(RBHead *h)
{
	((Checkers *)h)->pathN = 0;
}

Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	Checkers *g = (Checkers *)h;
	unsigned char sq[16];
	char cells[32];
	short n = Parse(move, sq, 16), got;
	Boolean progress;
	char result[32];

	if (h->over) {
		strcpy(why, "the game is over");
		return false;
	}
	if (seat != h->turn) {
		strcpy(why, "it is not that side's turn");
		return false;
	}
	if (!n) {
		strcpy(why, "a move is like 11-15 or 15x22");
		return false;
	}
	memcpy(cells, g->cells, 32);
	got = Walk(cells, seat, sq, n, why, &progress);
	if (got == 2)
		strcpy(why, "the jumps must go on");
	if (got != 1)
		return false;
	memcpy(g->cells, cells, 32);
	memcpy(g->last, sq, n);
	g->lastN = n;
	g->quiet = progress ? 0 : g->quiet + 1;
	h->turn = 1 - seat;
	strcpy(result, "win ");
	RBNum(result, seat);
	if (!AnyMove(cells, 1 - seat)) {
		short s, left = 0;
		for (s = 0; s < 32; s++)
			if (Owner(cells[s]) == 1 - seat)
				left++;
		strcat(result, left ? " blocked" : " captured");
		RBEnd(h, result);
	} else if (g->limit && g->quiet >= g->limit)
		RBEnd(h, "draw limit");
	return true;
}

void GamePosition(RBHead *h)
{
	((Checkers *)h)->lastN = 0;
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
	Checkers *g = (Checkers *)h;
	const char *p = m + 2;
	short i;

	if (m[0] == 'B' && m[1] == 'D' && m[2] == ' ') {
		p = m + 3;
		for (i = 0; i < 32; i++)
			if (Owner(p[i]) < 0 && p[i] != '.')
				return true;            /* not 32 squares: the board stays */
		memcpy(g->cells, p, 32);
		return true;
	}
	if (m[0] == 'L' && m[1] == ' ') {
		g->quiet = Num(&p);
		g->limit = Num(&p);
		if (g->quiet < 0)
			g->quiet = 0;
		if (g->limit < 0)
			g->limit = 0;
		return true;
	}
	return false;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	strcpy(out, GameSeat(seat));
	strcat(out, " wins: ");
	strcat(out, GameSeat(1 - seat));
	strcat(out, strcmp(how, "captured") == 0 ? " has no pieces left." : " cannot move.");
}

/* ---- the board ----------------------------------------------------------------------------------- */

static Boolean Flip(RBHead *h)
{
	return RBBottom(h) == 0;        /* Black starts at the top: turned round when Black sits at the bottom */
}

static void SquareRect(RBHead *h, short s, Rect *r)
{
	short row = Row(s), col = Col(s);

	if (Flip(h)) {
		row = 7 - row;
		col = 7 - col;
	}
	SetRect(r, kLeft + col * kSq, kTop + row * kSq, kLeft + (col + 1) * kSq, kTop + (row + 1) * kSq);
}

static short SquareAt(RBHead *h, Point pt)
{
	short row, col;

	if (pt.h < kLeft || pt.v < kTop || pt.h >= kLeft + 8 * kSq || pt.v >= kTop + 8 * kSq)
		return 0;
	col = (pt.h - kLeft) / kSq;
	row = (pt.v - kTop) / kSq;
	if (Flip(h)) {
		row = 7 - row;
		col = 7 - col;
	}
	return Sq(row, col);
}

static void Piece(const Rect *sqr, char ch, Boolean ghost)
{
	Rect r = *sqr, k;

	InsetRect(&r, 4, 4);
	if (ghost) {
		PenPat(&kDots);
		PenSize(2, 2);
		FrameOval(&r);
		PenNormal();
		return;
	}
	if (Owner(ch) == 0)
		PaintOval(&r);
	else {
		EraseOval(&r);
		FrameOval(&r);
	}
	if (King(ch)) {
		k = r;
		InsetRect(&k, 4, 4);
		if (Owner(ch) == 0)
			EraseOval(&k);
		else
			PaintOval(&k);
		InsetRect(&k, 2, 2);
		if (Owner(ch) == 0)
			PaintOval(&k);
		else
			EraseOval(&k);
	}
}

/* A small piece of the seat's color, at x, baseline y */
static void Swatch(short seat, short x, short y)
{
	Rect r;

	SetRect(&r, x, y - 8, x + 9, y + 1);
	if (seat == 0)
		PaintOval(&r);
	else
		FrameOval(&r);
}

void GameDraw(RBHead *h)
{
	Checkers *g = (Checkers *)h;
	Rect r, panel;
	short s, i, n[4] = { 0, 0, 0, 0 };
	char extra[64];

	SetRect(&r, kLeft - 1, kTop - 1, kLeft + 8 * kSq + 1, kTop + 8 * kSq + 1);
	FrameRect(&r);
	for (s = 1; s <= 32; s++) {
		char ch = g->cells[s - 1];
		SquareRect(h, s, &r);
		FillRect(&r, &kDark);
		if (ch != '.')
			Piece(&r, ch, false);
		n[(Owner(ch) == 1) * 2 + King(ch)] += ch != '.';
	}
	for (i = 0; i < g->lastN && !g->pathN; i++) {
		SquareRect(h, g->last[i], &r);
		InsetRect(&r, 1, 1);
		FrameRect(&r);
	}
	for (i = 0; i < g->pathN; i++) {
		SquareRect(h, g->path[i], &r);
		PenSize(2, 2);
		FrameRect(&r);
		PenNormal();
	}
	if (g->pathN > 1) {
		SquareRect(h, g->path[g->pathN - 1], &r);
		Piece(&r, g->cells[g->path[0] - 1], true);
	}
	SetRect(&panel, kLeft + 8 * kSq + 12, kTop, gPlayW - 6, gPlayH - 2);
	extra[0] = 0;
	strcat(extra, "Black ");
	RBNum(extra, n[0] + n[1]);
	if (n[1]) {
		strcat(extra, " (");
		RBNum(extra, n[1]);
		strcat(extra, "K)");
	}
	strcat(extra, ", White ");
	RBNum(extra, n[2] + n[3]);
	if (n[3]) {
		strcat(extra, " (");
		RBNum(extra, n[3]);
		strcat(extra, "K)");
	}
	RBDrawPanel(h, &panel, "Checkers", extra);
	if (!h->over)
		Swatch(h->turn, panel.right - 10, panel.top + 11);
}

/* ---- clicks -------------------------------------------------------------------------------------- */

static Boolean Movable(const char *cells, short seat, short s)
{
	if (Owner(cells[s - 1]) != seat)
		return false;
	return AnyJump(cells, seat) ? CanJump(cells, s) : CanJump(cells, s) || CanSlide(cells, s);
}

void GameClick(RBHead *h, Point pt)
{
	Checkers *g = (Checkers *)h;
	short s = SquareAt(h, pt), seat = RBMover(h), got;
	unsigned char sq[16];
	char cells[32], why[64], move[64];
	Boolean progress;

	if (!s)
		return;
	if (!RBCanMove(h)) {
		SysBeep(10);
		if (!h->over && !h->pending && !h->offer)
			RBNote(h, "Wait for your turn.");
		return;
	}
	memcpy(sq, g->path, g->pathN);
	sq[g->pathN] = s;
	memcpy(cells, g->cells, 32);
	got = g->pathN ? Walk(cells, seat, sq, g->pathN + 1, why, &progress) : 0;
	if (g->pathN == 0 || (g->pathN == 1 && Owner(g->cells[s - 1]) == seat) || (s == g->path[0] && !got)) {
		Boolean again = g->pathN && s == g->path[0];
		g->pathN = 0;
		h->note[0] = 0;
		if (again)
			return;
		if (Movable(g->cells, seat, s)) {
			g->path[0] = s;
			g->pathN = 1;
		} else if (Owner(g->cells[s - 1]) == seat) {
			SysBeep(10);
			RBNote(h, AnyJump(g->cells, seat) ? "You must jump: choose a piece that can." : "That piece cannot move.");
		} else
			SysBeep(10);
		return;
	}
	if (g->pathN >= 15)
		return;
	if (!got) {
		SysBeep(10);
		why[0] -= 'a' - 'A';
		strcat(why, ".");
		RBNote(h, why);
		return;
	}
	g->path[g->pathN++] = s;
	if (got == 2) {
		RBNote(h, "Jump on: click the next square.");
		return;
	}
	Format(g->path, g->pathN, g->cells, move);
	RBPlay(h, move);
}
