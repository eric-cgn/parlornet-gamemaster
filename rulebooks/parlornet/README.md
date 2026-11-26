# ParlorNet rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that connects your Mac to a ParlorNet parlor. You put it in the folder with GameMaster like any other rulebook and choose File > New Game > ParlorNet. Its window signs you in, starts Othello, Dots&Boxes, BattleShips or one of ParlorNet's own games (Checkers, Mancala, Five in a Row, Grid Words, Rummy Tile, Cannon Fire, Backgammon) against the bot or another player, takes invitations, and takes up the games you are already playing. Each game opens in GameMaster's own window, already connected, and two or more games can run at once. A parlor plays GameMaster only through this rulebook, which carries GameMaster's own Othello, Dots&Boxes and BattleShips rulebooks as well as ParlorNet's seven. A game connected with GameMaster's Game > Connect is turned down.

## What you need

You need a Mac Plus or later with System 6.0 or later, MacTCP or Open Transport's TCP/IP, and GameMaster 1.0.0 with the rulebooks of the games you play: its own Othello, Dots&Boxes and BattleShips, and ParlorNet's game rulebooks (`rulebooks/`). The rulebook works only with GameMaster 1.0.0 as it was released. It checks GameMaster's code first, and if the copy differs, the lobby still opens but no game starts from it.

## Using it

1. Install it with Install ParlorNet Rulebooks (http://docs.parlornet.org/gamemaster-rulebooks.html), which puts it and the game rulebooks in the folder that holds GameMaster.
2. Open GameMaster and choose File > New Game > ParlorNet.
3. The Server field holds the parlor's name and the Port field its GameMaster port, 6508. Type your name on that parlor and click Sign in. If the parlor asks for a password or a code, type it and click OK.
4. The window has three tabs below the sign-in line and its Sign out button. On **New game**, choose a game, then pick the opponent from the Opponent menu: Bot, the first choice; Wait for opponent, for whoever joins your table; one of the players online, whom the menu lists each time you open it; or Other..., which asks for the name of any player, online or not. The button beside it says what you will play: click "Othello vs the bot", for example. GameMaster opens the game, connected, and you move first.
5. When another player invites you, the New game tab says so: Accept opens the game and Decline turns the invitation down. The window asks the parlor for your invitations every five seconds.
6. **Your games** lists the games you are playing, wherever you started them, with how many on the tab. Resume opens one of ParlorNet's own games in GameMaster where it stands, as the parlor keeps it. Othello, Dots&Boxes and BattleShips cannot be taken up half way in GameMaster, so play those on the web or by telnet.
7. **All games** lists the tables where someone waits for a player, with Join, and the games other players are playing. Opening a tab asks the parlor again.
8. "Sign out" ends the sign-in and the games started through it.

Keep the ParlorNet window open while you play. Closing it, or signing out, ends the games it started. A game you took up with Resume stays at its table instead, and you can take it up again.

## How it works

GameMaster sends all of its network traffic through one small part of its code, and the rest of the program reaches that part through a table of addresses in memory. While the ParlorNet window is open, the rulebook points some of those addresses at its own code. The games it starts then travel over the window's single connection to the parlor, and every other connection still goes to GameMaster's own code, so calls you make yourself with Game > Connect work as before. Nothing is written to GameMaster or to any file, and closing the window puts the table back.

The parlor relays the sign-in to its own accounts, and a game that comes over your signed-in connection is yours. Neither your Mac's address nor its name counts.

The connection uses the parlor's client protocol, plain lines of text. Each GameMaster connection inside it is a number, and its bytes go as hexadecimal. The documentation of the ParlorNet server's GameMaster extension ([`adapters/gamemaster.py`](https://github.com/eric-cgn/parlornet/blob/main/adapters/gamemaster.py) in the ParlorNet repository) describes the lines, with TBL, which lists the tables of the Your games and All games tabs.

Once you are signed in, the rulebook tells the parlor which program you play with and on which Mac, as any ParlorNet client may (the protocol's CLI), and the parlor records both with each of your turns, for table histories, the activity feed and profiles. It sends `CLI PROG GameMaster 1.0.0 + ParlorNet 0.5`, or `CLI PROG GameMaster + ParlorNet 0.5` when GameMaster's code is not that of 1.0.0 as released, and `CLI MACH` with the Mac's model, such as `CLI MACH Macintosh SE/30`. The model comes from Gestalt's machine type (`gestaltMachineType`, 'mach'), looked up in a table of the models Apple listed for it (`src/machine.c`). A number the table lacks, or a system without Gestalt, which came with System 6.0.4, gives plain `Macintosh`. The parlor shows these words and believes nothing else from them.

Resume starts a new game in GameMaster, as Play does, and the parlor then sends that game the whole position of the table from its own records before any move. ParlorNet's game rulebooks from version 1.1 take such a position (`rulebooks/common/rb.h`). Nothing at the table changes when you resume it.

System 6 has no pop-up menu control (the standard one came with System 7), so the window draws the Opponent box itself and shows the menu with the Toolbox's PopUpMenuSelect, which every Mac from the Plus on has. Other... is a small dialog built in memory, since the rulebook carries no dialog resources.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/parlornet-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code.

## License

MIT; see LICENSE.
