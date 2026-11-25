/*
 * Cannon Fire for GameMaster: ParlorNet's cannonfire/1, ruleset classic, for two
 * players (docs/types/cannonfire-1.md in the ParlorNet repository).
 *
 * Two tanks on a landscape that shells dig into. On its turn a tank may drive
 * left or right once (four times a game), then sets an angle and a power and
 * fires. All the physics run on the parlor, so this rulebook plays only on a
 * parlor: it sends FIRE angle power and MOVE L|R n, and draws what the parlor
 * sends back.
 *
 * Besides the messages of rb.h the parlor sends the shell's path (TRJ x y ...)
 * and the state from its view: Z x heights (the terrain, one character a
 * column), K seat x hp (a tank), AIM seat angle power (its last aim: the view's
 * A line, renamed because A answers a move), L seat moves
 * (moves left), N wind, H x y (the last burst), B budget moved, T seat points,
 * and W seat, which ends it. A whole position (rb.h) is the same state between
 * POS and POE; the aim then comes from the parlor's again.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <Events.h>
#include <string.h>

#include "rb.h"

enum { kCols = 200, kLeft = 4, kBase = 134, kArenaW = 488, kTrj = 54 };
enum { bAm5, bAm1, bAp1, bAp5, bPm5, bPm1, bPp1, bPp5, bFire, bLeft, bRight, kButtons };

const short gPlayW = 496, gPlayH = 256;
const OSType gMagic = 'PNcf';
const short gFormat = 1;
const Boolean gDraws = false;
const Boolean gParlorOnly = true;

typedef struct {
	RBHead h;
	short w;                    /* 1024: arena width in columns (0 until the parlor sends the state) */
	unsigned char height[kCols];
	short tankX[2], hp[2], aimA[2], aimP[2], moves[2], points[2];
	short wind, burstX, burstY, budget, moved;
	short angle, power;         /* this Mac's aim, set with the buttons or the arrow keys */
	unsigned char aimSet;       /* the aim was taken from the parlor's state */
	unsigned char trjN;         /* the last shell's path: points */
	unsigned char trjX[kTrj], trjY[kTrj];
} Cannon;

static const Pattern kEarth = { { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 } };
static Rect gButtonRect[kButtons];

long GameSize(void)
{
	return sizeof(Cannon);
}

const char *GameSeat(short seat)
{
	return seat ? "Tank B" : "Tank A";
}

void GameInit(RBHead *h)
{
	Cannon *g = (Cannon *)h;

	g->burstX = -1;
	g->angle = 45;
	g->power = 50;
	h->turn = 0;
}

void GameClear(RBHead *h)
{
}

static short Num(const char **p)
{
	short v = 0, neg = 0;

	while (**p == ' ')
		(*p)++;
	if (**p == '-' || **p == '+') {
		neg = **p == '-';
		(*p)++;
	}
	if (**p < '0' || **p > '9')
		return -32000;
	while (**p >= '0' && **p <= '9')
		v = v * 10 + *(*p)++ - '0';
	return neg ? -v : v;
}

Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	/* the parlor sends the state after each move and shot: nothing to do here */
	return true;
}

Boolean GameMessage(RBHead *h, const char *m)
{
	Cannon *g = (Cannon *)h;
	const char *p = m + 2;
	short s, x, y;

	if (m[0] == 'Z' && m[1] == ' ') {
		x = Num(&p);
		if (*p == ' ')
			p++;
		while (x >= 0 && x < kCols && *p >= '0') {
			g->height[x++] = *p++ - '0';
			if (x > g->w)
				g->w = x;
		}
	} else if (m[0] == 'K' && m[1] == ' ') {
		s = Num(&p);
		if (s == 0 || s == 1) {
			g->tankX[s] = Num(&p);
			g->hp[s] = Num(&p);
		}
	} else if (strncmp(m, "AIM ", 4) == 0) {
		p = m + 4;
		s = Num(&p);
		if (s == 0 || s == 1) {
			g->aimA[s] = Num(&p);
			g->aimP[s] = Num(&p);
		}
	} else if (m[0] == 'L' && m[1] == ' ') {
		s = Num(&p);
		if (s == 0 || s == 1)
			g->moves[s] = Num(&p);
	} else if (m[0] == 'N' && m[1] == ' ') {
		g->wind = Num(&p);
	} else if (m[0] == 'H' && m[1] == ' ') {
		g->burstX = Num(&p);
		g->burstY = Num(&p);
	} else if (m[0] == 'B' && m[1] == ' ') {
		g->budget = Num(&p);
		g->moved = Num(&p);
	} else if (m[0] == 'T' && m[1] == ' ') {
		s = Num(&p);
		if (s == 0 || s == 1)
			g->points[s] = Num(&p);
	} else if (m[0] == 'F' && m[1] == ' ') {
		g->trjN = 0;                    /* a new shot: its path follows */
		g->burstX = -1;
	} else if (m[0] == 'T' && m[1] == 'R' && m[2] == 'J') {
		p = m + 3;
		while (g->trjN < kTrj && (x = Num(&p)) != -32000 && (y = Num(&p)) != -32000) {
			g->trjX[g->trjN] = x < 0 ? 0 : x > 255 ? 255 : x;
			g->trjY[g->trjN] = y < 0 ? 0 : y > 255 ? 255 : y;
			g->trjN++;
		}
	} else if (m[0] == 'W' && m[1] == ' ') {
		if (*p == '0' || *p == '1')
			h->turn = *p - '0';
		if (!g->aimSet && h->connected) {
			g->angle = g->aimA[h->local];
			g->power = g->aimP[h->local];
			g->aimSet = 1;
		}
		return true;
	} else
		return false;
	RBQuiet();
	return true;
}

void GamePosition(RBHead *h)
{
	Cannon *g = (Cannon *)h;

	g->aimSet = 0;
	g->trjN = 0;
	g->burstX = -1;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	strcpy(out, GameSeat(seat));
	strcat(out, " wins. Points ");
	strcat(out, how);
	strcat(out, ".");
}

/* ---- drawing ------------------------------------------------------------------------------------- */

/* the left edge of column x: the columns share the arena's width, so they are 6 or 7 pixels wide */
static short Cx(Cannon *g, short x)
{
	return kLeft + (short)((long)x * kArenaW / (g->w ? g->w : 1));
}

/* the middle of column x */
static short Cm(Cannon *g, short x)
{
	return (Cx(g, x) + Cx(g, x + 1)) / 2;
}

static short Y(short height)
{
	return kBase - 2 * height;
}

static void DrawButton(short i, const char *label, short x, short y, short wd)
{
	SetRect(&gButtonRect[i], x, y, x + wd, y + 18);
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

/* the gun's end from a tank on column x, for an angle and a power */
static void Gun(Cannon *g, short x, short angle, Point *from, Point *to)
{
	static const short cosT[19] = { 100, 98, 94, 87, 77, 64, 50, 34, 17, 0, -17, -34, -50, -64, -77, -87, -94, -98,
		-100 };
	short k = (angle + 5) / 10, len = 12;

	if (k > 18)
		k = 18;
	from->h = Cm(g, x);
	from->v = Y(g->height[x]) - 5;
	to->h = from->h + cosT[k] * len / 100;
	to->v = from->v - cosT[k < 9 ? 9 - k : k - 9] * len / 100;
}

void GameDraw(RBHead *h)
{
	Cannon *g = (Cannon *)h;
	short x, s, i, c, rx;
	Rect r, panel;
	char t[48], extra[96];
	Point a, b;
	Boolean mine = h->connected && g->w && h->turn == h->local && !h->over && !h->pending;

	SetRect(&r, kLeft - 1, 0, kLeft + kArenaW + 1, kBase + 1);
	FrameRect(&r);
	for (x = 0; x < g->w; x++) {
		SetRect(&r, Cx(g, x), Y(g->height[x]), Cx(g, x + 1), kBase);
		FillRect(&r, &kEarth);
		MoveTo(r.left, r.top);
		LineTo(r.right - 1, r.top);
	}
	for (s = 0; s < 2 && g->w; s++) {
		x = g->tankX[s];
		SetRect(&r, Cx(g, x) - 3, Y(g->height[x]) - 5, Cx(g, x + 1) + 3, Y(g->height[x]));
		if (g->hp[s] > 0) {
			EraseRect(&r);
			FrameRect(&r);
			if (s == 1)
				InvertRect(&r);
			Gun(g, x, s == h->local && h->connected ? g->angle : g->aimA[s], &a, &b);
			PenSize(2, 2);
			MoveTo(a.h, a.v);
			LineTo(b.h, b.v);
			PenNormal();
		} else {
			MoveTo(r.left, r.top);
			LineTo(r.right, r.bottom);
			MoveTo(r.left, r.bottom);
			LineTo(r.right, r.top);
		}
		t[0] = 'A' + s;
		t[1] = 0;
		MoveTo(r.left + 3, r.top - 3 < 9 ? 9 : r.top - 3);
		RBText(t);
	}
	for (i = 0; i < g->trjN; i++)           /* the last shell's path */
		if (g->trjY[i] <= 66) {
			c = Cm(g, g->trjX[i]);
			SetRect(&r, c - 1, Y(g->trjY[i]) - 1, c + 1, Y(g->trjY[i]) + 1);
			PaintRect(&r);
		}
	if (g->burstX >= 0) {
		c = Cm(g, g->burstX);
		rx = Cx(g, 7) - kLeft;              /* seven columns */
		SetRect(&r, c - rx, Y(g->burstY) - 14, c + rx, Y(g->burstY) + 14);
		PenPat(&kEarth);
		FrameOval(&r);
		PenNormal();
	}
	/* the aim and its buttons */
	strcpy(t, "Angle ");
	RBNum(t, g->angle);
	MoveTo(kLeft, kBase + 20);
	RBText(t);
	strcpy(t, "Power ");
	RBNum(t, g->power);
	MoveTo(kLeft, kBase + 42);
	RBText(t);
	DrawButton(bAm5, mine ? "+5" : NULL, 74, kBase + 6, 36);
	DrawButton(bAm1, mine ? "+1" : NULL, 114, kBase + 6, 36);
	DrawButton(bAp1, mine ? "-1" : NULL, 154, kBase + 6, 36);
	DrawButton(bAp5, mine ? "-5" : NULL, 194, kBase + 6, 36);
	DrawButton(bPm5, mine ? "-5" : NULL, 74, kBase + 28, 36);
	DrawButton(bPm1, mine ? "-1" : NULL, 114, kBase + 28, 36);
	DrawButton(bPp1, mine ? "+1" : NULL, 154, kBase + 28, 36);
	DrawButton(bPp5, mine ? "+5" : NULL, 194, kBase + 28, 36);
	DrawButton(bFire, mine ? "Fire" : NULL, 4, kBase + 54, 70);
	DrawButton(bLeft, mine && !g->moved && g->moves[h->local] ? "Drive left" : NULL, 80, kBase + 54, 90);
	DrawButton(bRight, mine && !g->moved && g->moves[h->local] ? "Drive right" : NULL, 176, kBase + 54, 90);
	if (mine) {
		MoveTo(kLeft, kBase + 92);
		RBText("Arrow keys aim: left and right the angle, up and down the power; Return fires.");
	}
	SetRect(&panel, 300, kBase + 4, gPlayW - 4, gPlayH - 2);
	extra[0] = 0;
	if (g->w) {
		strcat(extra, "A ");
		RBNum(extra, g->hp[0]);
		strcat(extra, " hp, B ");
		RBNum(extra, g->hp[1]);
		strcat(extra, " hp. Wind ");
		RBNum(extra, g->wind);
		strcat(extra, g->wind > 0 ? " to the right." : g->wind < 0 ? " to the left." : ".");
	}
	RBDrawPanel(h, &panel, "Cannon Fire", extra);
}

/* ---- aiming and firing --------------------------------------------------------------------------- */

static Boolean Ready(RBHead *h)
{
	Cannon *g = (Cannon *)h;

	if (!h->connected) {
		RBNote(h, "Cannon Fire plays on a ParlorNet parlor, which flies the shells: start it from the ParlorNet "
			"window.");
		return false;
	}
	if (!g->w) {
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

static void Aim(Cannon *g, short da, short dp)
{
	g->angle += da;
	g->power += dp;
	if (g->angle < 0)
		g->angle = 0;
	if (g->angle > 180)
		g->angle = 180;
	if (g->power < 0)
		g->power = 0;
	if (g->power > 100)
		g->power = 100;
}

static void Fire(RBHead *h)
{
	Cannon *g = (Cannon *)h;
	char move[24];

	strcpy(move, "FIRE ");
	RBNum(move, g->angle);
	strcat(move, " ");
	RBNum(move, g->power);
	h->note[0] = 0;
	RBPlay(h, move);
}

void GameClick(RBHead *h, Point pt)
{
	Cannon *g = (Cannon *)h;
	static const signed char da[8] = { 5, 1, -1, -5, 0, 0, 0, 0 }, dp[8] = { 0, 0, 0, 0, -5, -1, 1, 5 };
	short i;
	Rect r;

	for (i = 0; i < kButtons; i++)
		if (gButtonRect[i].right > gButtonRect[i].left && PtInRect(pt, &gButtonRect[i])) {
			r = gButtonRect[i];
			InvertRoundRect(&r, 10, 10);
			while (StillDown())
				;
			InvertRoundRect(&r, 10, 10);
			if (!Ready(h))
				return;
			if (i < 8)
				Aim(g, da[i], dp[i]);
			else if (i == bFire)
				Fire(h);
			else {
				h->note[0] = 0;
				RBPlay(h, i == bLeft ? "MOVE L 999" : "MOVE R 999");
			}
			return;
		}
}

void GameKey(RBHead *h, short ch)
{
	Cannon *g = (Cannon *)h;

	if (!h->connected || !g->w || !RBCanMove(h))
		return;
	if (ch == 28)
		Aim(g, 1, 0);           /* left arrow: the gun turns to the left */
	else if (ch == 29)
		Aim(g, -1, 0);
	else if (ch == 30)
		Aim(g, 0, 1);
	else if (ch == 31)
		Aim(g, 0, -1);
	else if (ch == '\r' || ch == 3)
		Fire(h);
}
