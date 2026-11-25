ParlorNet rulebook for GameMaster 0.5
=====================================

This rulebook for GameMaster 1.0.0, the Macintosh game program of 1991,
connects your Mac to a ParlorNet parlor. You put it in the folder with
GameMaster like any other rulebook and choose File > New Game > ParlorNet. Its
window signs you in, starts Othello, Dots&Boxes, BattleShips or one of
ParlorNet's own games against the bot or another player, takes
invitations, and takes up the games you are already playing. Each game opens
in GameMaster's own window, already connected, and two or more games can run
at once.

A parlor plays GameMaster only through this rulebook, which carries
GameMaster's own Othello, Dots&Boxes and BattleShips rulebooks as well as
ParlorNet's seven. A game connected with GameMaster's Game > Connect is turned
down.

GameMaster and its rulebooks are not included.

Web page: http://docs.parlornet.org/parlornet-rulebook.html


Requirements
------------

A Mac Plus or later with System 6.0 or later, MacTCP or Open Transport's
TCP/IP, and GameMaster 1.0.0 with the rulebooks of the games you play: its own
Othello, Dots&Boxes and BattleShips, and ParlorNet's game rulebooks
(http://docs.parlornet.org/gamemaster-rulebooks.html). The rulebook works only
with GameMaster 1.0.0 as it was released.
It checks GameMaster's code first, and if the copy differs, the lobby still
opens but no game starts from it.


Using it
--------

1. Install it with Install ParlorNet Rulebooks, which puts it and the game
   rulebooks in the folder that holds GameMaster.
2. Open GameMaster and choose File > New Game > ParlorNet.
3. The Server field holds the parlor's name and the Port field its GameMaster
   port, 6508. Type your name on that parlor and click Sign in. If the parlor
   asks for a password or a code, type it and click OK.
4. The window has three tabs. On New game, choose a game, then pick the
   opponent from the Opponent menu: Bot, the first choice; Wait for
   opponent, for whoever joins your table; one of the players online; or
   Other..., which asks for the name of any player, online or not. The
   button beside it says what you will play: click "Othello vs the bot", for
   example. GameMaster opens the game, connected, and you move first.
5. When another player invites you, the New game tab says so: Accept opens
   the game and Decline turns the invitation down.
6. Your games lists the games you are playing, wherever you started them,
   with how many on the tab. Resume opens one of ParlorNet's own games in
   GameMaster where it stands, as the parlor keeps it. Othello, Dots&Boxes
   and BattleShips cannot be taken up half way in GameMaster, so play those
   on the web or by telnet.
7. All games lists the tables where someone waits for a player, with Join,
   and the games other players are playing.
8. "Sign out" ends the sign-in and the games started through it.

Keep the ParlorNet window open while you play. Closing it, or signing out,
ends the games it started. A game you took up with Resume stays at its table
instead, and you can take it up again.


How it works
------------

GameMaster sends all of its network traffic through one small part of its
code, and the rest of the program reaches that part through a table of
addresses in memory. While the ParlorNet window is open, the rulebook points
some of those addresses at its own code. The games it starts then travel over
the window's single connection to the parlor, and every other connection still
goes to GameMaster's own code, so calls you make yourself with Game > Connect
work as before. Nothing is written to GameMaster or to any file, and closing
the window puts the table back.

The parlor relays the sign-in to its own accounts, and a game that comes over
your signed-in connection is yours. Neither your Mac's address nor its name
counts.


The files
---------

The rulebook comes in Install ParlorNet Rulebooks
(parlornet-rulebooks-installer.hqx or .bin), with the game rulebooks:
http://docs.parlornet.org/gamemaster-rulebooks.html


GameMaster
----------

GameMaster is by Rhys Hollow, with Peter N. Lewis and Quinn. This rulebook is
not affiliated with them and contains none of GameMaster.


License
-------

MIT License

Copyright (c) 2026 the ParlorNet authors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
