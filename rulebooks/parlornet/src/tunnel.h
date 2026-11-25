/*
 * The tunnel: GameMaster's own game connections, carried over the rulebook's one
 * connection to the parlor, by hooks in GameMaster's jump table.
 */
#ifndef TUNNEL_H
#define TUNNEL_H

#include <Types.h>

/* lobby.c provides these */
void LobbyLine(char *line);                 /* a line from the parlor that is not the tunnel's */
void LobbyLost(void);                       /* the parlor's connection ended */
void LobbyNote(const char *text);           /* a line for the lobby's message area */
Boolean LobbySignedIn(void);

/* tunnel.c */
Boolean TunnelCheck(void);                  /* GameMaster 1.0.0, unchanged: the hooks can go in */
Boolean TunnelInstall(void);
void TunnelRemove(void);
void TunnelReset(void);                     /* the parlor's connection ended: every tunneled game ends */
void TunnelStart(const unsigned char *rulebookID, const char *opponent);
short TunnelGames(void);                    /* games going through the tunnel now */
/* Which of these rulebooks (GMRZ ID strings) GameMaster has: asked now, found from the main loop. */
void TunnelAskInstalled(const unsigned char *const *ids, short n);
unsigned short TunnelInstalled(void);       /* a bit for each one found (all of them until it is known) */
void Pump(void);                            /* read what the parlor sent */
OSErr SendLine(const char *text);

#endif
