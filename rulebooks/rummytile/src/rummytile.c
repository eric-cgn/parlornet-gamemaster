/*
 * Rummy Tile for GameMaster: ParlorNet's rummytile/1, ruleset standard, for two
 * players (docs/types/rummytile-1.md in the ParlorNet repository).
 *
 * 106 tiles, the numbers 1 to 13 in red, blue, black and yellow, two of each,
 * and two jokers. The parlor keeps the pool, deals the racks and checks every
 * step and every turn, so this rulebook plays only on a parlor. It shows the
 * table, the player's rack and what the turn would do, and sends each step the
 * player makes as the parlor's own token: R7B7K7-n (rack tiles to a new set),
 * R7-3 (to set 3), 3.2-5 (tile 2 of set 3 to set 5), 3.2-n, 3.2-r (back to the
 * rack), 3-5 (join set 3 onto set 5), 3/4 (split set 3 before tile 4), reset,
 * UND and END.
 *
 * After every step and every turn the parlor sends the whole state: Z, then
 * ST k kind tiles for each set, H and the rack (in lines), TP played points
 * code, P pool, C and the racks' sizes, I and the first melds, and W and whose
 * turn it is, which ends it. A whole position (rb.h) is the same state between
 * POS and POE.
 */
#include <Quickdraw.h>
#include <Sound.h>
#include <Events.h>
#include <string.h>

#include "rb.h"

enum { kSets = 40, kRack = 48, kTW = 16, kTH = 22, kTableW = 492, kTableTop = 4, kTableRows = 5, kRowH = 28,
	kRackLeft = 4, kRackTop = 154, kRackW = 362 };
enum { bNew, bSplit, bJoin, bBack, bUndo, bReset, bEnd, kButtons };

const short gPlayW = 500, gPlayH = 256;
const OSType gMagic = 'PNrt';
const short gFormat = 1;
const Boolean gDraws = false;
const Boolean gParlorOnly = true;

typedef struct {
	unsigned char n;            /* tiles */
	char kind;                  /* r a run, g a group, x not a set */
	unsigned char changed;      /* made or changed this turn */
	unsigned char t[13];        /* tiles: color * 16 + number, 64 a joker */
} TileSet;

typedef struct {
	RBHead h;
	short nsets;                /* 1024: sets on the table (0 before the deal) */
	TileSet sets[kSets];        /* 1026 */
	short nrack;
	unsigned char rack[kRack];
	short pool, count[2];
	unsigned char melded[2];
	short tpPlayed, tpPoints;   /* this turn: tiles played from the rack, points of its sets (-1: melded) */
	char tpCode[8];             /* ok, none, sets, meld, joker */
	unsigned char dealt;        /* a state has come from the parlor */
	unsigned char joining;      /* Join was clicked: the next set clicked takes the selected tile's set */
	unsigned char selRack[kRack];   /* rack tiles selected */
	short selSet, selPos;       /* a table tile selected (from 1), or 0 */
} Rummy;

static const char kColours[] = "RBKY";
static const Pattern kGray = { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 } };
static Rect gButtonRect[kButtons];

long GameSize(void)
{
	return sizeof(Rummy);
}

const char *GameSeat(short seat)
{
	return seat ? "Player 2" : "Player 1";
}

void GameInit(RBHead *h)
{
	h->turn = 0;
}

void GameClear(RBHead *h)
{
	Rummy *g = (Rummy *)h;

	memset(g->selRack, 0, sizeof g->selRack);
	g->selSet = g->selPos = 0;
	g->joining = 0;
}

/* ---- tiles --------------------------------------------------------------------------------------- */

/* "R7" -> its code, 0 if no tile; p moves past it */
static unsigned char Parse(const char **p)
{
	const char *c = *p, *k;
	short n = 0;

	while (*c == ' ')
		c++;
	if (*c == 'J') {
		*p = c + 1;
		return 64;
	}
	k = strchr(kColours, *c);
	if (!*c || !k)
		return 0;
	c++;
	while (*c >= '0' && *c <= '9')
		n = n * 10 + *c++ - '0';
	*p = c;
	return n >= 1 && n <= 13 ? (k - kColours) * 16 + n : 0;
}

static void Name(unsigned char t, char *out)
{
	if (t == 64) {
		strcat(out, "J");
		return;
	}
	out += strlen(out);
	*out++ = kColours[t >> 4];
	*out = 0;
	RBNum(out, t & 15);
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
		return -1;
	while (**p >= '0' && **p <= '9')
		v = v * 10 + *(*p)++ - '0';
	return neg ? -v : v;
}

/* ---- the parlor's state ------------------------------------------------------------------------- */

Boolean GameApply(RBHead *h, short seat, const char *move, char *why)
{
	/* the parlor sends the whole state after each step: nothing to do here */
	return true;
}

Boolean GameMessage(RBHead *h, const char *m)
{
	Rummy *g = (Rummy *)h;
	const char *p = m + 2;
	unsigned char t;

	if (m[0] == 'Z' && !m[1]) {
		g->nsets = g->nrack = 0;
		GameClear(h);
		RBQuiet();
	} else if (m[0] == 'S' && m[1] == 'T' && m[2] == ' ') {
		short k;
		TileSet *s;
		p = m + 3;
		k = Num(&p);
		if (k < 1 || k > kSets)
			return true;
		s = &g->sets[k - 1];
		while (*p == ' ')
			p++;
		s->kind = *p++;
		s->changed = *p == '*';
		if (*p == '*')
			p++;
		s->n = 0;
		while (s->n < 13 && (t = Parse(&p)) != 0)
			s->t[s->n++] = t;
		if (k > g->nsets)
			g->nsets = k;
		RBQuiet();
	} else if (m[0] == 'H' && m[1] == ' ') {
		while (g->nrack < kRack && (t = Parse(&p)) != 0)
			g->rack[g->nrack++] = t;
		RBQuiet();
	} else if (m[0] == 'T' && m[1] == 'P' && m[2] == ' ') {
		p = m + 3;
		g->tpPlayed = Num(&p);
		g->tpPoints = Num(&p);
		if (g->tpPoints < 0) {          /* "-": the first meld is made */
			while (*p == ' ' || *p == '-')
				p++;
		}
		while (*p == ' ')
			p++;
		strncpy(g->tpCode, p, 7);
		g->tpCode[7] = 0;
		RBQuiet();
	} else if (m[0] == 'P' && m[1] == ' ') {
		g->pool = Num(&p);
		RBQuiet();
	} else if (m[0] == 'C' && m[1] == ' ') {
		g->count[0] = Num(&p);
		g->count[1] = Num(&p);
		RBQuiet();
	} else if (m[0] == 'I' && m[1] == ' ') {
		g->melded[0] = Num(&p) == 1;
		g->melded[1] = Num(&p) == 1;
		RBQuiet();
	} else if (m[0] == 'W' && m[1] == ' ') {
		if (*p >= '0' && *p <= '1')
			h->turn = *p - '0';
		g->dealt = 1;
	} else
		return false;
	return true;
}

void GameWin(RBHead *h, short seat, const char *how, char *out)
{
	strcpy(out, GameSeat(seat));
	strcat(out, " wins. Scores ");
	strcat(out, how);
	strcat(out, ".");
}

/* ---- drawing ------------------------------------------------------------------------------------- */

static void TileRect(short x, short y, Rect *r)
{
	SetRect(r, x, y, x + kTW, y + kTH);
}

static void DrawTile(const Rect *r, unsigned char t, Boolean selected)
{
	static const Pattern dots = { { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 } };
	char a[4], b[2];
	Rect in = *r;

	EraseRect(r);
	if (t == 64) {
		strcpy(a, "J");
		strcpy(b, "*");
	} else {
		a[0] = 0;
		RBNum(a, t & 15);
		b[0] = kColours[t >> 4];
		b[1] = 0;
	}
	FrameRect(r);
	if (t != 64 && t >> 4 == 1) {          /* blue: a dotted frame inside */
		InsetRect(&in, 2, 2);
		PenPat(&dots);
		FrameRect(&in);
		PenNormal();
		in = *r;
	}
	if (t != 64 && t >> 4 == 3) {          /* yellow: a double frame */
		InsetRect(&in, 2, 2);
		FrameRect(&in);
	}
	TextFace(bold);
	MoveTo((r->left + r->right - TextWidth(a, 0, strlen(a))) / 2, r->top + 10);
	RBText(a);
	TextFace(0);
	MoveTo((r->left + r->right - TextWidth(b, 0, 1)) / 2, r->top + 19);
	RBText(b);
	if (t != 64 && t >> 4 == 2) {          /* black: drawn in reverse */
		Rect k = *r;
		InsetRect(&k, 1, 1);
		InvertRect(&k);
	}
	if (selected)
		InvertRect(r);
}

/* where set k's first tile goes, the sets flowing in rows; false when it is below the table's rows */
static Boolean SetPlace(Rummy *g, short k, short *x, short *y)
{
	short i, cx = 4, cy = kTableTop, w;

	for (i = 1; i <= k; i++) {
		w = g->sets[i - 1].n * (kTW + 1) + 8;
		if (cx + w > kTableW && cx > 4) {
			cx = 4;
			cy += kRowH;
		}
		if (i == k)
			break;
		cx += w;
	}
	*x = cx;
	*y = cy;
	return cy + kTH <= kTableTop + kTableRows * kRowH;
}

static void DrawButton(short i, const char *label, short x, short y)
{
	SetRect(&gButtonRect[i], x, y, x + 86, y + 18);
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

void GameDraw(RBHead *h)
{
	Rummy *g = (Rummy *)h;
	short k, i, x, y;
	Rect r, panel;
	char extra[96];
	Boolean mine = h->connected && g->dealt && h->turn == h->local && !h->over && !h->pending;

	RBBody();
	for (k = 1; k <= g->nsets; k++) {
		TileSet *s = &g->sets[k - 1];
		if (!SetPlace(g, k, &x, &y)) {
			MoveTo(4, kTableTop + kTableRows * kRowH + 8);
			RBText("More sets than this window shows.");
			break;
		}
		for (i = 0; i < s->n; i++) {
			TileRect(x + i * (kTW + 1), y, &r);
			DrawTile(&r, s->t[i], g->selSet == k && g->selPos == i + 1);
		}
		if (s->kind == 'x' || s->changed) {    /* not a set yet: a broken line under it; changed: a solid one */
			if (s->kind == 'x')
				PenPat(&kGray);
			MoveTo(x, y + kTH + 2);
			LineTo(x + s->n * (kTW + 1) - 2, y + kTH + 2);
			PenNormal();
		}
	}
	MoveTo(0, kRackTop - 4);
	LineTo(kRackLeft + kRackW, kRackTop - 4);
	for (i = 0; i < g->nrack; i++) {
		short per = kRackW / (kTW + 1);
		if (i == 2 * per) {
			MoveTo(kRackLeft, kRackTop + 2 * (kTH + 3) + 10);
			RBText("More tiles than the rack shows.");
			break;
		}
		TileRect(kRackLeft + (i % per) * (kTW + 1), kRackTop + (i / per) * (kTH + 3), &r);
		DrawTile(&r, g->rack[i], g->selRack[i]);
	}
	SetRect(&panel, kRackLeft + kRackW + 8, kRackTop - 2, gPlayW - 4, gPlayH - 2);
	extra[0] = 0;
	if (g->dealt) {
		strcat(extra, "Pool ");
		RBNum(extra, g->pool);
		strcat(extra, ", racks ");
		RBNum(extra, g->count[0]);
		strcat(extra, " and ");
		RBNum(extra, g->count[1]);
		strcat(extra, ".");
	}
	if (mine) {
		strcat(extra, " Played ");
		RBNum(extra, g->tpPlayed);
		if (g->tpPoints >= 0 && !g->melded[h->local]) {
			strcat(extra, ", ");
			RBNum(extra, g->tpPoints);
			strcat(extra, " points");
		}
		strcat(extra, strcmp(g->tpCode, "ok") == 0 ? ": the turn counts." :
			strcmp(g->tpCode, "sets") == 0 ? ": a set is not valid." :
			strcmp(g->tpCode, "meld") == 0 ? ": a first meld needs 30." :
			strcmp(g->tpCode, "joker") == 0 ? ": a joker needs its tile." : ".");
	}
	RBDrawPanel(h, &panel, "Rummy Tile", extra);
	y = kRackTop + 2 * (kTH + 3) + 2;
	if (mine) {
		DrawButton(bNew, "New set", 4, y);
		DrawButton(bSplit, "Split", 94, y);
		DrawButton(bJoin, g->joining ? "Join: where?" : "Join", 184, y);
		DrawButton(bBack, "To rack", 274, y);
		DrawButton(bUndo, "Undo", 4, y + 22);
		DrawButton(bReset, "Start over", 94, y + 22);
		DrawButton(bEnd, strcmp(g->tpCode, "ok") == 0 ? "End turn" : "Draw a tile", 184, y + 22);
	} else
		for (i = 0; i < kButtons; i++)
			DrawButton(i, NULL, 0, 0);
}

/* ---- steps -------------------------------------------------------------------------------------- */

static Boolean Ready(RBHead *h)
{
	Rummy *g = (Rummy *)h;

	if (!h->connected) {
		RBNote(h, "Rummy Tile plays on a ParlorNet parlor, which deals the tiles: start it from the ParlorNet "
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

/* the rack tiles selected, written together ("R7B7K7"); 0 if none */
static short Selected(Rummy *g, char *out)
{
	short i, n = 0;

	out[0] = 0;
	for (i = 0; i < g->nrack; i++)
		if (g->selRack[i]) {
			Name(g->rack[i], out);
			n++;
		}
	return n;
}

static void Step(RBHead *h, const char *token)
{
	h->note[0] = 0;
	RBPlay(h, token);
}

static void TableTile(Rummy *g, char *out)
{
	out[0] = 0;
	RBNum(out, g->selSet);
	strcat(out, ".");
	RBNum(out, g->selPos);
}

void GameClick(RBHead *h, Point pt)
{
	Rummy *g = (Rummy *)h;
	short k, i, x, y;
	Rect r;
	char tok[64], tiles[48];

	for (i = 0; i < kButtons; i++)
		if (gButtonRect[i].right > gButtonRect[i].left && PtInRect(pt, &gButtonRect[i])) {
			r = gButtonRect[i];
			InvertRoundRect(&r, 10, 10);
			while (StillDown())
				;
			InvertRoundRect(&r, 10, 10);
			if (!Ready(h))
				return;
			if (i == bUndo) {
				GameClear(h);
				Step(h, "UND");
			} else if (i == bReset) {
				GameClear(h);
				Step(h, "reset");
			} else if (i == bEnd) {
				GameClear(h);
				Step(h, "END");
			} else if (i == bNew) {
				if (Selected(g, tiles)) {
					strcpy(tok, tiles);
					strcat(tok, "-n");
				} else if (g->selSet) {
					TableTile(g, tok);
					strcat(tok, "-n");
				} else {
					SysBeep(10);
					RBNote(h, "Select tiles of your rack, or a tile of the table, first.");
					return;
				}
				GameClear(h);
				Step(h, tok);
			} else if (i == bSplit || i == bBack || i == bJoin) {
				if (!g->selSet) {
					SysBeep(10);
					RBNote(h, "Select a tile of the table first.");
					return;
				}
				if (i == bJoin) {
					g->joining = 1;
					RBNote(h, "Click the set to join this tile's set onto.");
					return;
				}
				if (i == bSplit) {
					if (g->selPos < 2) {
						SysBeep(10);
						RBNote(h, "Select the tile the second set starts with.");
						return;
					}
					tok[0] = 0;
					RBNum(tok, g->selSet);
					strcat(tok, "/");
					RBNum(tok, g->selPos);
				} else {
					TableTile(g, tok);
					strcat(tok, "-r");
				}
				GameClear(h);
				Step(h, tok);
			}
			return;
		}
	/* the rack */
	if (pt.v >= kRackTop && pt.h >= kRackLeft && pt.h < kRackLeft + kRackW) {
		short per = kRackW / (kTW + 1);
		i = ((pt.v - kRackTop) / (kTH + 3)) * per + (pt.h - kRackLeft) / (kTW + 1);
		if (i >= g->nrack || !Ready(h))
			return;
		g->selSet = g->selPos = 0;
		g->joining = 0;
		g->selRack[i] = !g->selRack[i];
		h->note[0] = 0;
		return;
	}
	/* the table */
	for (k = 1; k <= g->nsets; k++) {
		TileSet *s = &g->sets[k - 1];
		if (!SetPlace(g, k, &x, &y))
			break;
		SetRect(&r, x, y, x + s->n * (kTW + 1), y + kTH);
		if (!PtInRect(pt, &r))
			continue;
		if (!Ready(h))
			return;
		i = (pt.h - x) / (kTW + 1) + 1;
		if (g->joining && g->selSet) {
			tok[0] = 0;
			RBNum(tok, g->selSet);
			strcat(tok, "-");
			RBNum(tok, k);
			GameClear(h);
			Step(h, tok);
			return;
		}
		if (Selected(g, tiles)) {               /* the rack tiles selected go into this set */
			strcpy(tok, tiles);
			strcat(tok, "-");
			RBNum(tok, k);
			GameClear(h);
			Step(h, tok);
			return;
		}
		if (g->selSet && g->selSet != k) {      /* the table tile selected moves to this set */
			TableTile(g, tok);
			strcat(tok, "-");
			RBNum(tok, k);
			GameClear(h);
			Step(h, tok);
			return;
		}
		if (g->selSet == k && g->selPos == i)
			g->selSet = g->selPos = 0;
		else {
			g->selSet = k;
			g->selPos = i;
		}
		h->note[0] = 0;
		return;
	}
}
