/*
 * Backgammon for GameMaster: ParlorNet's backgammon/1, ruleset standard, with
 * the doubling cube (docs/types/backgammon-1.md in the ParlorNet repository).
 *
 * The points are numbered 1 to 24 as the parlor numbers them: White (seat 0)
 * moves from 24 down to 1 and bears off from 1-6, Black (seat 1) from 1 up to
 * 24 and bears off from 19-24. The parlor rolls every die and judges every
 * turn, so this rulebook plays only on a parlor. It draws the board with the
 * player's side at the bottom and the home board on the right, or on the left
 * when the player prefers it there (H, or the Home button; the parlor keeps the
 * choice). The player picks a checker and then where it goes; each step is
 * staged on the Mac by the same rules the parlor uses (a die each, as many
 * dice as can be played, the larger die when only one can), Undo takes one
 * back, and Done sends the whole turn: M and the steps (13-9 9-4, bar-22,
 * 6-off). Roll, Double, Take, Drop, Accept and Reject go as M roll, M double
 * and so on, as the parlor's own words.
 *
 * After every change the parlor sends the whole state: BD and the 24 points
 * (seat 0's checkers counted up, seat 1's down), the bar and the checkers
 * borne off, CU the cube, DI the dice left to play, DR the dice rolled, N the
 * steps the roll needs, CA who may double, RS a resignation offered, LM the
 * last turn played, Q the phase, and W whose turn it is, which ends it. H says
 * which side the home board is drawn on. A whole position (rb.h) is the same
 * state between POS and POE.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <Events.h>
#include <string.h>

#include "rb.h"

enum { kL = 4, kT = 4, kB = 252, kPt = 22, kCk = 20, kRise = 19, kTip = 96, kMid = 128, kTrayL = 298,
	kTrayR = 322, kFlip = kL + kTrayR, kPanelL = 330, kPanelR = 492, kBar = 25, kOff = 0, kShown = 5 };
enum { bLeft, bRight, bHome, kButtons };

const short gPlayW = 496, gPlayH = 256;
const OSType gMagic = 'PNbg';
const short gFormat = 1;
const Boolean gDraws = false;
const Boolean gParlorOnly = true;

typedef struct {
	signed char p[24];          /* points 1-24: seat 0's checkers counted up, seat 1's down */
	unsigned char bar[2], off[2];
} Board;

typedef struct {
	unsigned char from, to, die;    /* from 1-24 or kBar, to 1-24 or kOff */
} Step;

typedef struct {
	RBHead h;
	Board b;                    /* 1024: the board as the parlor sent it */
	unsigned char dice[4], nd;  /* 1052: the dice left to play, in the parlor's order */
	unsigned char rolled[2];    /* the dice rolled this turn (0: none) */
	unsigned char need;         /* the steps the roll needs */
	unsigned char cube;         /* the cube's value */
	signed char owner;          /* its owner, -1 in the middle */
	unsigned char may[2];       /* the seat may double when its turn starts */
	char phase;                 /* r roll or double, m move, t take or drop, s a resignation offered, o over */
	unsigned char rsSeat, rsPoints;
	signed char lmSeat;         /* the last turn played: its seat (-1: none), dice, steps and whether it passed */
	unsigned char lmDice[2], lmN, lmPass;
	Step lm[4];
	unsigned char dealt;        /* a state has come from the parlor */
	unsigned char home;         /* 1: the home board on the left */
	unsigned char stageN;       /* the steps made by clicks this turn, not sent yet */
	Step stage[4];
	signed char sel;            /* the point picked up (1-24, kBar), or -1 */
} Gammon;

static const Pattern kGray = { { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 } };
static const Pattern kDots = { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 } };
static Rect gButtonRect[kButtons];

long GameSize(void)
{
	return sizeof(Gammon);
}

const char *GameSeat(short seat)
{
	return seat ? "Black" : "White";
}

static const signed char kStart[24] = { -2, 0, 0, 0, 0, 5, 0, 3, 0, 0, 0, -5, 5, 0, 0, 0, -3, 0, -5, 0, 0, 0, 0, 2 };

void GameInit(RBHead *h)
{
	Gammon *g = (Gammon *)h;

	memcpy(g->b.p, kStart, 24);
	g->cube = 1;
	g->owner = -1;
	g->lmSeat = g->sel = -1;
	h->turn = 0;
}

void GameClear(RBHead *h)
{
	Gammon *g = (Gammon *)h;

	g->stageN = 0;
	g->sel = -1;
}

/* ---- the rules, as the parlor's (plugins/backgammon) ------------------------------------------------ */

static short Sign(short seat)
{
	return seat ? -1 : 1;
}

static Boolean AllHome(const Board *b, short seat)
{
	short i, s = Sign(seat);

	if (b->bar[seat])
		return false;
	for (i = 0; i < 24; i++)
		if (b->p[i] * s > 0 && (seat ? i < 18 : i >= 6))
			return false;
	return true;
}

/* the steps one die d allows: (from, to) pairs into out; -> how many */
static short Steps(const Board *b, short seat, short d, unsigned char out[][2])
{
	short n = 0, s = Sign(seat), p, t, q;
	Boolean home;

	if (b->bar[seat]) {
		t = seat ? d : 25 - d;
		if (b->p[t - 1] * -s < 2) {
			out[0][0] = kBar;
			out[0][1] = t;
			n = 1;
		}
		return n;
	}
	home = AllHome(b, seat);
	for (p = 1; p <= 24; p++) {
		if (b->p[p - 1] * s <= 0)
			continue;
		t = seat ? p + d : p - d;
		if (t >= 1 && t <= 24) {
			if (b->p[t - 1] * -s >= 2)
				continue;
		} else if (!home)
			continue;
		else if (t != (seat ? 25 : 0)) {
			/* past the edge: only from the checker farthest from it */
			for (q = seat ? 19 : p + 1; q < (seat ? p : 7); q++)
				if (b->p[q - 1] * s > 0)
					break;
			if (q < (seat ? p : 7))
				continue;
		}
		out[n][0] = p;
		out[n][1] = t >= 1 && t <= 24 ? t : kOff;
		n++;
	}
	return n;
}

static void Shift(Board *b, short seat, short from, short to)
{
	short s = Sign(seat);

	if (from == kBar)
		b->bar[seat]--;
	else
		b->p[from - 1] -= s;
	if (to == kOff)
		b->off[seat]++;
	else {
		if (b->p[to - 1] * -s == 1) {
			b->p[to - 1] = 0;
			b->bar[1 - seat]++;
		}
		b->p[to - 1] += s;
	}
}

/* can want more of the n dice be played from b? Bearing off the last checker plays them all. */
static Boolean Reach(const Board *b, short seat, const unsigned char *dice, short n, short want)
{
	unsigned char steps[16][2], rest[4];
	short i, j, k, m;
	Board nb;

	if (want <= 0 || b->off[seat] == 15)
		return want <= n;
	for (i = 0; i < n; i++) {
		for (j = 0; j < i; j++)
			if (dice[j] == dice[i])
				break;
		if (j < i)
			continue;               /* the same die value: the same steps */
		for (j = k = 0; j < n; j++)
			if (j != i)
				rest[k++] = dice[j];
		m = Steps(b, seat, dice[i], steps);
		for (j = 0; j < m; j++) {
			nb = *b;
			Shift(&nb, seat, steps[j][0], steps[j][1]);
			if (Reach(&nb, seat, rest, n - 1, want - 1))
				return true;
		}
	}
	return false;
}

/* the board as the staged steps leave it, and the dice still to play */
static short Now(Gammon *g, Board *b, unsigned char *dice)
{
	short i, j, n = g->nd, seat = g->h.local;

	*b = g->b;
	memcpy(dice, g->dice, 4);
	for (i = 0; i < g->stageN; i++) {
		Shift(b, seat, g->stage[i].from, g->stage[i].to);
		for (j = 0; j < n && dice[j] != g->stage[i].die; j++)
			;
		if (j < n) {
			memmove(dice + j, dice + j + 1, n - j - 1);
			n--;
		}
	}
	return n;
}

/* The steps the player may stage now, each with its die (the first die, in the parlor's order, that allows it),
   as the parlor's legal_steps: every step must leave a way to play as many dice as the roll can, and when only
   one die of two can be played, the larger one. -> how many */
static short Legal(Gammon *g, Step *out)
{
	unsigned char dice[4], rest[4], steps[16][2];
	short n, i, j, k, m, x, seat = g->h.local, full, total, big = 0;
	Board b, nb;

	if (g->phase != 'm' || g->h.turn != seat || g->h.over || !g->rolled[0])
		return 0;
	n = Now(g, &b, dice);
	full = g->rolled[0] == g->rolled[1] ? 4 : 2;
	total = g->need - (full - g->nd) - g->stageN;
	if (total <= 0 || b.off[seat] == 15)
		return 0;
	x = 0;
	for (i = 0; i < n; i++) {
		for (j = k = 0; j < n; j++)
			if (j != i)
				rest[k++] = dice[j];
		m = Steps(&b, seat, dice[i], steps);
		for (j = 0; j < m && x < 30; j++) {
			for (k = 0; k < x; k++)
				if (out[k].from == steps[j][0] && out[k].to == steps[j][1])
					break;
			if (k < x)
				continue;
			nb = b;
			Shift(&nb, seat, steps[j][0], steps[j][1]);
			if (Reach(&nb, seat, rest, n - 1, total - 1)) {
				out[x].from = steps[j][0];
				out[x].to = steps[j][1];
				out[x].die = dice[i];
				x++;
			}
		}
	}
	if (total == 1 && n == 2 && dice[0] != dice[1] && g->nd == 2 && g->stageN == 0) {
		big = dice[0] > dice[1] ? dice[0] : dice[1];
		for (i = 0; i < x && out[i].die != big; i++)
			;
		if (i < x) {
			for (i = j = 0; i < x; i++)
				if (out[i].die == big)
					out[j++] = out[i];
			x = j;
		}
	}
	return x;
}

static short Pips(const Board *b, short seat)
{
	short i, n = 25 * b->bar[seat], s = Sign(seat);

	for (i = 0; i < 24; i++)
		if (b->p[i] * s > 0)
			n += b->p[i] * s * (seat ? 24 - i : i + 1);
	return n;
}

/* ---- the parlor's state ------------------------------------------------------------------------- */

Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	/* the parlor sends the whole state after every move: nothing to do here */
	return true;
}

static short Num(const char **p)
{
	short v = 0, neg = 0;

	while (**p == ' ')
		(*p)++;
	if (**p == '-') {
		neg = 1;
		(*p)++;
	}
	if (**p < '0' || **p > '9')
		return neg ? -1 : -99;
	while (**p >= '0' && **p <= '9')
		v = v * 10 + *(*p)++ - '0';
	return neg ? -v : v;
}

/* "13-9", "bar-22", "6-off*" -> a step; false when it is none */
static Boolean ParseStep(const char **p, Step *st)
{
	const char *c = *p;
	short v;

	while (*c == ' ')
		c++;
	if (strncmp(c, "bar", 3) == 0) {
		st->from = kBar;
		c += 3;
	} else if ((v = Num(&c)) >= 1 && v <= 24)
		st->from = v;
	else
		return false;
	if (*c++ != '-')
		return false;
	if (strncmp(c, "off", 3) == 0) {
		st->to = kOff;
		c += 3;
	} else if ((v = Num(&c)) >= 1 && v <= 24)
		st->to = v;
	else
		return false;
	if (*c == '*')
		c++;
	*p = c;
	return true;
}

Boolean GameMessage(RBHead *h, const char *m)
{
	Gammon *g = (Gammon *)h;
	const char *p = m + 3;
	short i, v;

	if (m[0] == 'B' && m[1] == 'D' && m[2] == ' ') {
		Board b;
		for (i = 0; i < 24; i++) {
			v = Num(&p);
			if (v < -15 || v > 15)
				return true;            /* not a board: it stays */
			b.p[i] = v;
		}
		for (i = 0; i < 4; i++) {
			v = Num(&p);
			if (v < 0 || v > 15)
				return true;
			(i < 2 ? b.bar : b.off)[i & 1] = v;
		}
		g->b = b;
		g->nd = g->rolled[0] = g->rolled[1] = g->need = g->rsSeat = g->rsPoints = 0;
		g->lmSeat = -1;
		GameClear(h);
	} else if (m[0] == 'C' && m[1] == 'U' && m[2] == ' ') {
		v = Num(&p);
		g->cube = v >= 1 && v <= 64 ? v : 1;
		while (*p == ' ')
			p++;
		g->owner = *p == '0' || *p == '1' ? *p - '0' : -1;
	} else if (m[0] == 'D' && m[1] == 'I' && (m[2] == ' ' || !m[2])) {
		for (g->nd = 0; m[2] && g->nd < 4 && (v = Num(&p)) >= 1 && v <= 6; g->nd++)
			g->dice[g->nd] = v;
	} else if (m[0] == 'D' && m[1] == 'R' && (m[2] == ' ' || !m[2])) {
		g->rolled[0] = g->rolled[1] = 0;
		if (m[2] && (v = Num(&p)) >= 1 && v <= 6) {
			g->rolled[0] = v;
			v = Num(&p);
			g->rolled[1] = v >= 1 && v <= 6 ? v : g->rolled[0];
		}
	} else if (m[0] == 'N' && m[1] == ' ') {
		p = m + 2;
		v = Num(&p);
		g->need = v >= 0 && v <= 4 ? v : 0;
	} else if (m[0] == 'C' && m[1] == 'A' && m[2] == ' ') {
		g->may[0] = Num(&p) == 1;
		g->may[1] = Num(&p) == 1;
	} else if (m[0] == 'R' && m[1] == 'S' && m[2] == ' ') {
		g->rsSeat = Num(&p) == 1;
		v = Num(&p);
		g->rsPoints = v > 0 && v < 200 ? v : 1;
	} else if (m[0] == 'L' && m[1] == 'M' && m[2] == ' ') {
		v = Num(&p);
		if (v != 0 && v != 1)
			return true;
		g->lmSeat = v;
		g->lmDice[0] = Num(&p);
		g->lmDice[1] = Num(&p);
		while (*p == ' ')
			p++;
		g->lmPass = strncmp(p, "pass", 4) == 0;
		for (g->lmN = 0; g->lmN < 4 && ParseStep(&p, &g->lm[g->lmN]); g->lmN++)
			;
	} else if (m[0] == 'Q' && m[1] == ' ') {
		g->phase = strcmp(m + 2, "roll") == 0 ? 'r' : strcmp(m + 2, "move") == 0 ? 'm' :
			strcmp(m + 2, "take") == 0 ? 't' : strcmp(m + 2, "resign") == 0 ? 's' : 'o';
	} else if (m[0] == 'W' && m[1] == ' ') {
		if (m[2] == '0' || m[2] == '1')
			h->turn = m[2] - '0';
		g->dealt = 1;
		return true;                    /* the end of the state: drawn now */
	} else if (m[0] == 'H' && m[1] == ' ') {
		g->home = strcmp(m + 2, "left") == 0;
		return true;
	} else
		return false;
	RBQuiet();
	return true;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	const char *p = how;
	short points = Num(&p);

	while (*p == ' ')
		p++;
	strcpy(out, GameSeat(seat));
	strcat(out, " wins");
	if (points > 0) {
		strcat(out, " ");
		RBNum(out, points);
		strcat(out, points == 1 ? " point" : " points");
	}
	if (strcmp(p, "gammon") == 0)
		strcat(out, ": a gammon");
	else if (strcmp(p, "backgammon") == 0)
		strcat(out, ": a backgammon");
	strcat(out, ".");
}

/* ---- drawing ------------------------------------------------------------------------------------- */

static short Me(RBHead *h)
{
	return RBBottom(h);
}

/* point p as the player at the bottom counts it (1-24, 1 at the home board's edge); the same both ways */
static short Rel(RBHead *h, short p)
{
	return Me(h) ? 25 - p : p;
}

/* the column (0-12, 6 is the bar) of point p, and whether it is in the bottom row */
static short Col(RBHead *h, short p, Boolean *bottom)
{
	short m = Rel(h, p);

	*bottom = m <= 12;
	return m <= 6 ? 13 - m : m <= 12 ? 12 - m : m <= 18 ? m - 13 : m - 12;
}

static short ColX(short col)
{
	return kL + 2 + col * kPt;
}

/* the mirror image, when the home board is on the left */
static void Flip(Gammon *g, Rect *r)
{
	short l = r->left;

	if (g->home) {
		r->left = kFlip - r->right;
		r->right = kFlip - l;
	}
}

static short FlipX(Gammon *g, short x)
{
	return g->home ? kFlip - x : x;
}

/* the i-th checker (from the edge) on point p, or on the bar (kBar) for seat */
static void Spot(Gammon *g, short p, short seat, short i, Rect *r)
{
	Boolean bottom;
	short x, y;

	if (i >= kShown)
		i = kShown - 1;
	if (p == kBar) {
		x = ColX(6);
		y = seat == Me(&g->h) ? kMid + 24 + i * kRise : kMid - 24 - kCk - i * kRise;
	} else {
		x = ColX(Col(&g->h, p, &bottom));
		y = bottom ? kB - 2 - kCk - i * kRise : kT + 2 + i * kRise;
	}
	SetRect(r, x + 1, y, x + 1 + kCk, y + kCk);
	Flip(g, r);
}

static void Checker(const Rect *r, short seat, Boolean ghost)
{
	if (ghost) {
		EraseOval(r);           /* white, so it shows on a gray point too */
		PenPat(&kDots);
		PenSize(2, 2);
		FrameOval(r);
		PenNormal();
	} else if (seat == 0) {
		EraseOval(r);
		FrameOval(r);
	} else
		PaintOval(r);
}

/* the place of the outermost checker drawn of a stack of n (n > 0): below the +n when there are more than
   kShown */
static short Top(short n)
{
	return n > kShown ? kShown - 2 : n - 1;
}

/* +n in r, the last place of a stack: the checkers not drawn, black on a white label (framed in dots when
   the stack is where the checker picked up can go) */
static void More(const Rect *r, short n, Boolean target)
{
	Rect b = *r;
	char t[8];

	t[0] = '+';
	t[1] = 0;
	RBNum(t, n);
	InsetRect(&b, 0, 3);
	EraseRect(&b);
	if (target) {
		PenPat(&kDots);
		PenSize(2, 2);
	}
	FrameRect(&b);
	PenNormal();
	TextFace(bold);
	if (TextWidth(t, 0, strlen(t)) > b.right - b.left - 3)
		TextFace(0);
	MoveTo((b.left + b.right - TextWidth(t, 0, strlen(t))) / 2 + 1, b.bottom - 4);
	RBText(t);
	TextFace(0);
}

/* n checkers of seat on point p (or the bar): up to kShown, or one fewer and +n in the last place for the
   rest */
static void Stack(Gammon *g, short p, short seat, short n)
{
	short i;
	Rect r;

	for (i = 0; i <= Top(n) && n > 0; i++) {
		Spot(g, p, seat, i, &r);
		Checker(&r, seat, false);
	}
	if (n > kShown) {
		Spot(g, p, seat, kShown - 1, &r);
		More(&r, n - (kShown - 1), false);
	}
}

static void Triangle(Gammon *g, short col, Boolean bottom)
{
	short x = ColX(col), base = bottom ? kB - 2 : kT + 2, tip = bottom ? kB - 2 - kTip : kT + 2 + kTip;
	PolyHandle t = OpenPoly();

	MoveTo(FlipX(g, x + 1), base);
	LineTo(FlipX(g, x + kPt / 2), tip);
	LineTo(FlipX(g, x + kPt - 1), base);
	LineTo(FlipX(g, x + 1), base);
	ClosePoly();
	if ((col + bottom) & 1)
		FillPoly(t, &kGray);
	FramePoly(t);
	KillPoly(t);
}

/* a small mark at the tip of point p: hollow where the last turn's checker left, filled where it went */
static void Mark(Gammon *g, short p, Boolean filled)
{
	Boolean bottom;
	short x;
	Rect r;

	if (p < 1 || p > 24)
		return;
	x = ColX(Col(&g->h, p, &bottom)) + kPt / 2;
	SetRect(&r, x - 3, bottom ? kB - 2 - kTip - 7 : kT + 2 + kTip + 3, x + 3, 0);
	r.bottom = r.top + 4;
	Flip(g, &r);
	if (filled)
		PaintRect(&r);
	else
		FrameRect(&r);
}

static void Die(Rect *r, short v, Boolean used)
{
	/* the nine places of the pips, as bits: 0 top left ... 8 bottom right */
	static const short spots[7] = { 0, 0x010, 0x101, 0x111, 0x145, 0x155, 0x16D };
	short i, x, y;
	Rect s;

	EraseRoundRect(r, 5, 5);
	if (used) {
		PenPat(&kDots);
		FrameRoundRect(r, 5, 5);
		PenNormal();
		return;
	}
	FrameRoundRect(r, 5, 5);
	for (i = 0; i < 9; i++)
		if (spots[v] & (1 << i)) {
			x = r->left + 3 + (i % 3) * 5;
			y = r->top + 3 + (i / 3) * 5;
			SetRect(&s, x, y, x + 3, y + 3);
			PaintRect(&s);
		}
}

/* the dice of the turn on the mover's side of the board, those played drawn faint */
static void Dice(Gammon *g, const unsigned char *left, short nleft)
{
	short n = g->rolled[0] == g->rolled[1] ? 4 : 2, i, j, x, used;
	unsigned char v[4], have[4];
	Rect r;

	for (i = 0; i < n; i++)
		v[i] = g->rolled[i & 1];
	memcpy(have, left, 4);
	x = (g->h.turn == Me(&g->h) ? ColX(7) : ColX(0)) + 3 * kPt - (n * 18 + (n - 1) * 5) / 2;
	for (i = n - 1; i >= 0; i--) {         /* the dice still to play are the last of their value */
		for (j = 0; j < nleft && have[j] != v[i]; j++)
			;
		used = j == nleft;
		if (!used)
			have[j] = 0;
		SetRect(&r, x + i * 23, kMid - 9, x + i * 23 + 18, kMid + 9);
		Flip(g, &r);
		Die(&r, v[i], used);
	}
}

static void DrawButton(short i, const char *label, short x, short y, short w)
{
	SetRect(&gButtonRect[i], x, y, x + w, y + 20);
	if (!label) {
		gButtonRect[i].right = gButtonRect[i].left;
		return;
	}
	RBChrome();
	FrameRoundRect(&gButtonRect[i], 10, 10);
	MoveTo((gButtonRect[i].left + gButtonRect[i].right - TextWidth((Ptr)label, 0, strlen(label))) / 2, y + 14);
	RBText(label);
	RBBody();
}

static Boolean Mine(RBHead *h)
{
	return h->connected && ((Gammon *)h)->dealt && h->turn == h->local && !h->over && !h->pending;
}

/* the two buttons under the panel, as the phase has them */
static void Labels(Gammon *g, const char **a, const char **b, short n)
{
	*a = *b = NULL;
	if (!Mine(&g->h))
		return;
	if (g->phase == 'r') {
		*a = "Roll";
		*b = g->may[g->h.local] ? "Double" : NULL;
	} else if (g->phase == 't') {
		*a = "Take";
		*b = "Drop";
	} else if (g->phase == 's') {
		*a = "Accept";
		*b = "Reject";
	} else if (g->phase == 'm') {
		*a = n || !g->rolled[0] ? NULL : "Done";
		*b = g->stageN ? "Undo" : NULL;
	}
}

static void Words(Gammon *g, short n, char *extra)
{
	RBHead *h = &g->h;
	short me = h->local;

	if (!h->connected || h->over)
		return;
	if (!g->dealt) {
		strcpy(extra, "Waiting for the parlor.");
		return;
	}
	if (Mine(h) && g->phase == 'r') {
		strcpy(extra, "Roll the dice");
		if (g->may[me]) {
			strcat(extra, ", or double the cube to ");
			RBNum(extra, 2 * g->cube);
		}
		strcat(extra, ".");
	} else if (Mine(h) && g->phase == 't') {
		strcpy(extra, GameSeat(1 - me));
		strcat(extra, " doubles to ");
		RBNum(extra, 2 * g->cube);
		strcat(extra, ". Take it, or drop and lose ");
		RBNum(extra, g->cube);
		strcat(extra, ".");
	} else if (Mine(h) && g->phase == 's') {
		strcpy(extra, GameSeat(g->rsSeat));
		strcat(extra, " offers to resign for ");
		RBNum(extra, g->rsPoints);
		strcat(extra, g->rsPoints == 1 ? " point." : " points.");
	} else if (Mine(h) && g->phase == 'm') {
		if (n == 0)
			strcpy(extra, "Done sends the move; Undo takes a step back.");
		else if (g->sel >= 0)
			strcpy(extra, "Click where it goes.");
		else
			strcpy(extra, "Click a checker, then where it goes.");
	} else if (g->phase == 't' && h->turn != me) {
		strcpy(extra, "You double to ");
		RBNum(extra, 2 * g->cube);
		strcat(extra, ": waiting for the answer.");
	} else if (g->lmSeat >= 0 && (g->lmSeat != me || g->lmPass)) {
		strcpy(extra, g->lmSeat == me ? "You" : GameSeat(g->lmSeat));
		strcat(extra, " rolled ");
		RBNum(extra, g->lmDice[0]);
		strcat(extra, "-");
		RBNum(extra, g->lmDice[1]);
		strcat(extra, g->lmPass ? " and could not move." : ".");
	}
}

void GameDraw(RBHead *h)
{
	Gammon *g = (Gammon *)h;
	Step legal[30];
	unsigned char left[4];
	short i, n, nleft, me = Me(h), v;
	const char *a, *b;
	Board bd;
	Rect r, panel;
	char extra[160];

	n = Mine(h) ? Legal(g, legal) : 0;
	nleft = Now(g, &bd, left);
	RBBody();
	/* the frame, the bar and the tray */
	SetRect(&r, kL, kT, kL + 4 + 13 * kPt, kB);
	FrameRect(&r);
	SetRect(&r, ColX(6), kT, ColX(7), kB);
	Flip(g, &r);
	FrameRect(&r);
	SetRect(&r, kTrayL, kT, kTrayR, kB);
	Flip(g, &r);
	FrameRect(&r);
	for (i = 0; i < 13; i++)
		if (i != 6) {
			Triangle(g, i, false);
			Triangle(g, i, true);
		}
	/* the last turn's steps, then the checkers */
	if (!g->stageN && g->lmSeat >= 0)
		for (i = 0; i < g->lmN; i++) {
			Mark(g, g->lm[i].from, false);
			Mark(g, g->lm[i].to, true);
		}
	for (i = 1; i <= 24; i++)
		if (bd.p[i - 1])
			Stack(g, i, bd.p[i - 1] > 0 ? 0 : 1, bd.p[i - 1] > 0 ? bd.p[i - 1] : -bd.p[i - 1]);
	for (i = 0; i < 2; i++)
		Stack(g, kBar, i, bd.bar[i]);
	/* borne off: a slab each, from the player's end of the tray and from the other end */
	for (i = 0; i < 2; i++)
		for (v = 0; v < bd.off[i]; v++) {
			short y = i == me ? kB - 7 - v * 5 : kT + 3 + v * 5;
			SetRect(&r, kTrayL + 2, y, kTrayR - 2, y + 4);
			Flip(g, &r);
			if (i == 0)
				FrameRect(&r);
			else
				PaintRect(&r);
		}
	/* the cube: on the bar in the middle, in the tray on its owner's side */
	if (g->owner < 0)
		SetRect(&r, ColX(6) + 1, kMid - 10, ColX(6) + 21, kMid + 10);
	else {
		v = g->owner == me ? kMid + 2 : kMid - 22;
		SetRect(&r, kTrayL + 2, v, kTrayR - 2, v + 20);
	}
	Flip(g, &r);
	EraseRect(&r);
	PenSize(2, 2);
	FrameRect(&r);
	PenNormal();
	extra[0] = 0;
	RBNum(extra, g->owner < 0 && g->cube == 1 ? 64 : g->cube);
	TextFace(bold);
	MoveTo((r.left + r.right - TextWidth(extra, 0, strlen(extra))) / 2, r.bottom - 6);
	RBText(extra);
	TextFace(0);
	if (h->connected && g->dealt && g->phase == 'm' && g->rolled[0])
		Dice(g, h->turn == h->local ? left : g->dice, h->turn == h->local ? nleft : g->nd);
	/* the checker picked up, and where it can go */
	if (g->sel >= 0) {
		short cnt = g->sel == kBar ? bd.bar[me] : bd.p[g->sel - 1] * Sign(me);
		Spot(g, g->sel, me, cnt > 1 ? Top(cnt) : 0, &r);
		InsetRect(&r, 5, 5);            /* a dot in its middle: picked up, still its own color */
		InvertOval(&r);
		for (i = 0; i < n; i++)
			if (legal[i].from == g->sel) {
				if (legal[i].to == kOff) {
					SetRect(&r, kTrayL + 1, kMid + 24, kTrayR - 1, kB - 1);
					Flip(g, &r);
					PenPat(&kDots);
					PenSize(2, 2);
					FrameRect(&r);
					PenNormal();
				} else {
					cnt = bd.p[legal[i].to - 1] * Sign(me);
					Spot(g, legal[i].to, me, cnt < 0 ? 1 : cnt, &r);
					if (cnt > kShown)
						More(&r, cnt - (kShown - 1), true);
					else
						Checker(&r, me, true);
				}
			}
	}
	/* the panel */
	SetRect(&panel, kPanelL, kT, kPanelR, 198);
	extra[0] = 0;
	Words(g, n, extra);
	if (g->dealt) {
		strcat(extra, extra[0] ? " Pips: you " : "Pips: you ");
		RBNum(extra, Pips(&bd, me));
		strcat(extra, ", ");
		strcat(extra, GameSeat(1 - me));
		strcat(extra, " ");
		RBNum(extra, Pips(&bd, 1 - me));
		strcat(extra, ".");
	}
	RBDrawPanel(h, &panel, "Backgammon", extra);
	if (!h->over) {
		SetRect(&r, kPanelR - 10, kT + 3, kPanelR - 1, kT + 12);
		Checker(&r, h->turn, false);
	}
	Labels(g, &a, &b, n);
	DrawButton(bLeft, a, kPanelL, 206, 76);
	DrawButton(bRight, b, kPanelR - 76, 206, 76);
	DrawButton(bHome, g->home ? "Home right" : "Home left", kPanelL, 232, 76);
}

/* ---- clicks -------------------------------------------------------------------------------------- */

static Boolean Ready(RBHead *h)
{
	Gammon *g = (Gammon *)h;

	if (!h->connected) {
		RBNote(h, "Backgammon plays on a ParlorNet parlor, which rolls the dice: start it from the ParlorNet "
			"window.");
		return false;
	}
	if (!g->dealt) {
		RBNote(h, "Waiting for the parlor.");
		return false;
	}
	if (!RBCanMove(h)) {
		if (!h->over && !h->pending)
			RBNote(h, "Wait for your turn.");
		return false;
	}
	return true;
}

/* the point at pt (1-24), the bar (kBar), the tray (kOff), or -1 */
static short PointAt(Gammon *g, Point pt)
{
	short x = FlipX(g, pt.h), col, m;
	Boolean bottom = pt.v >= kMid;

	if (pt.v < kT || pt.v >= kB)
		return -1;
	if (x >= kTrayL - 2 && x < kTrayR + 2)
		return kOff;
	if (x < kL + 2 || x >= ColX(13))
		return -1;
	col = (x - kL - 2) / kPt;
	if (col == 6)
		return kBar;
	m = bottom ? (col > 6 ? 13 - col : 12 - col) : (col < 6 ? col + 13 : col + 12);
	return Me(&g->h) ? 25 - m : m;
}

static void Tok(short p, char *out)
{
	if (p == kBar)
		strcat(out, "bar");
	else if (p == kOff)
		strcat(out, "off");
	else
		RBNum(out, p);
}

static void Send(RBHead *h, const char *move)
{
	Gammon *g = (Gammon *)h;
	Step keep[4];
	short n = g->stageN;

	memcpy(keep, g->stage, sizeof keep);
	RBPlay(h, move);                /* the staged steps stay on the board until the parlor's answer */
	if (h->pending == 'M') {
		memcpy(g->stage, keep, sizeof keep);
		g->stageN = n;
	}
}

static void Done(RBHead *h)
{
	Gammon *g = (Gammon *)h;
	char move[64];
	short i;

	move[0] = 0;
	for (i = 0; i < g->stageN; i++) {
		if (i)
			strcat(move, " ");
		Tok(g->stage[i].from, move);
		strcat(move, "-");
		Tok(g->stage[i].to, move);
	}
	Send(h, g->stageN ? move : "end");
}

static void Home(RBHead *h)
{
	Gammon *g = (Gammon *)h;

	g->home = !g->home;
	RBSend(h, g->home ? "H left" : "H right");
}

static void Act(RBHead *h, short i)
{
	Gammon *g = (Gammon *)h;
	Step legal[30];
	const char *a, *b, *label;

	if (i == bHome) {
		Home(h);
		return;
	}
	if (!Ready(h))
		return;
	Labels(g, &a, &b, Legal(g, legal));
	label = i == bLeft ? a : b;
	if (!label)
		return;
	h->note[0] = 0;
	if (strcmp(label, "Undo") == 0) {
		g->stageN--;
		g->sel = -1;
	} else if (strcmp(label, "Done") == 0)
		Done(h);
	else {
		char word[8];
		strcpy(word, label);
		word[0] += 'a' - 'A';
		Send(h, word);
	}
}

void GameKey(RBHead *h, short ch)
{
	Gammon *g = (Gammon *)h;

	if (ch == 'h' || ch == 'H')
		Home(h);
	else if ((ch == 'u' || ch == 'U' || ch == 8) && g->phase == 'm' && g->stageN)
		Act(h, bRight);
	else if ((ch == 'r' || ch == 'R') && g->phase == 'r')
		Act(h, bLeft);
	else if ((ch == 'd' || ch == 'D' || ch == 13 || ch == 3) && g->phase == 'm')
		Act(h, bLeft);
}

static void Stage(Gammon *g, const Step *st)
{
	g->stage[g->stageN++] = *st;
	g->sel = -1;
}

void GameClick(RBHead *h, Point pt)
{
	Gammon *g = (Gammon *)h;
	Step legal[30];
	short i, n, at, hits, which = -1;
	Rect r;

	for (i = 0; i < kButtons; i++)
		if (gButtonRect[i].right > gButtonRect[i].left && PtInRect(pt, &gButtonRect[i])) {
			r = gButtonRect[i];
			InvertRoundRect(&r, 10, 10);
			while (StillDown())
				;
			InvertRoundRect(&r, 10, 10);
			GetMouse(&pt);
			if (PtInRect(pt, &gButtonRect[i]))
				Act(h, i);
			return;
		}
	at = PointAt(g, pt);
	if (at < 0 || !Ready(h))
		return;
	if (g->phase != 'm') {
		SysBeep(10);
		RBNote(h, g->phase == 'r' ? "Roll the dice first." : "Answer first, with the buttons below.");
		return;
	}
	n = Legal(g, legal);
	h->note[0] = 0;
	if (g->sel >= 0 && at == g->sel) {
		g->sel = -1;
		return;
	}
	for (i = hits = 0; i < n; i++)
		if (legal[i].to == at && (g->sel < 0 || legal[i].from == g->sel)) {
			hits++;
			which = i;
		}
	if (g->sel >= 0 && hits) {
		Stage(g, &legal[which]);
		return;
	}
	for (i = 0; i < n && legal[i].from != at; i++)
		;
	if (i < n) {                    /* a checker that can move: picked up */
		g->sel = at;
		return;
	}
	if (hits == 1) {                /* the only checker that can go there */
		Stage(g, &legal[which]);
		return;
	}
	SysBeep(10);
	if (!n)
		RBNote(h, "Nothing more to play: click Done.");
	else if (g->sel >= 0)
		RBNote(h, "That checker cannot go there.");
	else if (hits > 1)
		RBNote(h, "More than one checker can go there: click the one to move first.");
	else
		RBNote(h, "That checker cannot move.");
}
