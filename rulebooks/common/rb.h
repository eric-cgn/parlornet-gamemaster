/*
 * The shared part of ParlorNet's game rulebooks for GameMaster: the game record's
 * header, the messages between the two sides, the panel beside the board, and
 * the RULES entry point. Each game (checkers.c, ...) supplies the Game functions
 * below.
 *
 * Messages, the same both ways, at most 250 characters:
 *
 *   M move       the sender plays this move
 *   A move       the move is confirmed: the sender of M now shows it
 *   R reason     the last move or offer is refused
 *   D            the sender offers a draw
 *   DY, DN       the sender accepts or declines the other side's offer
 *   G            the sender resigns
 *   E result     the game is over, in the parlor's words ("win 0 captured", "draw agreed",
 *                "resign 1", "timeout 0")
 *
 * A game may add messages of its own (GameMessage): Grid Words' rack, scores and bag.
 *
 * The whole position, from the parlor only, at any time (at least when a game
 * being played at a parlor table is taken up from GameMaster):
 *
 *   POS seat turn offer   the position follows: this Mac plays seat, turn is the
 *                         seat to move, offer a draw offer's seat + 1 (0: none)
 *   ...                   the game's own lines, which set the rest (GameMessage):
 *                         BD and the board, and whatever else the game keeps
 *   POE                   the position is complete: the window is drawn again
 *
 * POS forgets the move being made, a move waiting for its answer, the note and
 * the end (GameClear, then GamePosition for the game's own fields), and the
 * window is not drawn until POE.
 *
 * A side shows its own move only when the other side has confirmed it with A.
 * Against the parlor that is the server, which checks every move first; against
 * another Mac it is the other copy of the rulebook, which checks it by the same
 * rules. A resignation and an accepted draw are likewise shown when E comes back.
 * Seat 0 moves first; a new game puts this Mac in seat 0, and GameMaster's swap
 * puts the Mac that was called in seat 1.
 */
#ifndef RB_H
#define RB_H

#include <Types.h>
#include <Quickdraw.h>

#include "rulebook.h"

enum { kRBMsg = 252, kRBOut = 2, kRBHeadSize = 1024 };

/* The first kRBHeadSize bytes of every game record; the game's own fields follow. */
typedef struct RBHead {
	OSType magic;               /* 0: the game's own code */
	short format;               /* 4: the record's layout */
	unsigned char local;        /* 6: the seat this Mac plays when connected */
	unsigned char connected;    /* 7 */
	unsigned char turn;         /* 8: the seat to move */
	unsigned char over;         /* 9 */
	unsigned char pending;      /* 10: what waits for the other side: 0, 'M' (a move), 'Y' (a draw accepted), 'G' */
	unsigned char offer;        /* 11: a draw offer waiting: the offering seat + 1 */
	unsigned char noOffer;      /* 12: a seat + 1 that may offer again only after its next move */
	unsigned char outN;         /* 13: messages waiting to go */
	unsigned char armed;        /* 14: Resign was clicked once */
	unsigned char posing;       /* 15: between POS and POE: the window waits for the whole position */
	char result[32];            /* 16: the end, in the parlor's words */
	char note[160];             /* 48: a line for the player */
	char sent[kRBMsg];          /* 208: the move sent, until A or R */
	char out[kRBOut][kRBMsg];   /* 460: messages waiting to go */
	char reserved[kRBHeadSize - 460 - kRBOut * kRBMsg];
} RBHead;

/* ---- what each game supplies -------------------------------------------------------------------- */

extern const short gPlayW, gPlayH;     /* the play area */
extern const OSType gMagic;
extern const short gFormat;
extern const Boolean gDraws;          /* the game has draw offers */
extern const Boolean gParlorOnly;     /* the game plays only on a parlor, which deals or flies the shells */

long GameSize(void);                   /* the whole record */
void GameInit(RBHead *h);              /* a new game; the header is zero but for magic and format */
void GameClear(RBHead *h);             /* forget a move being made by clicks */
/* Check seat's whole move and play it: the turn passes, and RBEnd ends the game if it is over. false and why
   when the move is not legal now. */
Boolean GameApply(RBHead *h, short seat, const char *move, char *why);
void GameDraw(RBHead *h);              /* the whole play area, with RBDrawPanel */
void GameClick(RBHead *h, Point pt);   /* a click outside the panel's buttons */
const char *GameSeat(short seat);      /* "Black" */
void GameWin(RBHead *h, short seat, const char *how, char *out);   /* words for "win seat how" */
/* Optional (rb.c has empty ones): a message rb.c does not know (true if taken), a key typed, and the game's own
   fields forgotten before a whole position (POS). */
Boolean GameMessage(RBHead *h, const char *m);
void GameKey(RBHead *h, short ch);
void GamePosition(RBHead *h);

/* ---- what rb.c offers --------------------------------------------------------------------------- */

void RBPlay(RBHead *h, const char *move);      /* this Mac's move: sent for confirmation, or played */
void RBSend(RBHead *h, const char *text);      /* a message of the game's own, when connected */
void RBQuiet(void);                            /* the message being taken needs no drawing yet */
void RBEnd(RBHead *h, const char *result);
Boolean RBCanMove(RBHead *h);                  /* the player at this Mac may make a move now */
short RBMover(RBHead *h);                      /* the seat that clicks play for */
short RBBottom(RBHead *h);                     /* the seat drawn nearest the player */
void RBNote(RBHead *h, const char *text);
/* The panel: the game's name, who plays what, whose turn, extra (a line of the game's own), the note and the
   buttons, in r. */
void RBDrawPanel(RBHead *h, const Rect *r, const char *title, const char *extra);
void RBText(const char *s);
void RBBody(void);
void RBChrome(void);
short RBWrap(const char *s, short left, short top, short width, short lines);   /* -> lines used */
void RBNum(char *out, long v);                 /* appends v */

#endif
