ParlorNet game rulebooks for GameMaster 1.1
===========================================

These rulebooks let GameMaster 1.0.0, the Macintosh game program of 1991, play
ParlorNet's games. Each plays on a ParlorNet parlor, against the bot or
another player; Checkers, Mancala and Five in a Row also play between two
Macs like any other rulebook. The ParlorNet rulebook starts the games on a
parlor.

GameMaster and its own rulebooks are not included.

Web page: http://docs.parlornet.org/gamemaster-rulebooks.html


The games
---------

ParlorNet      the lobby: sign in, start games, take invitations
Checkers       Checkers, American rules
Mancala        Mancala, Kalah rules with four seeds a pit
Five in a Row  Five in a Row on a 15 by 15 board: five or more in a line wins
Grid Words     the crossword game, classic rules, standard board; on a parlor
               only, which deals the tiles and checks the words
Rummy Tile     tile rummy for two, standard rules; on a parlor only
Cannon Fire    two tanks shelling each other across a landscape; on a parlor
               only, which flies the shells
Backgammon     backgammon with the doubling cube; on a parlor only, which
               rolls the dice and checks every turn

Othello, Dots&Boxes and BattleShips play on a parlor with GameMaster's own
rulebooks of those names, started from the ParlorNet rulebook's window like
the others. A parlor plays GameMaster only through the ParlorNet rulebook.


Requirements
------------

A Mac Plus or later with System 6.0 or later, and GameMaster 1.0.0. On a
parlor, MacTCP or Open Transport's TCP/IP and the ParlorNet rulebook. The
installer needs System 6.0.3 or later.


Installing
----------

The installer puts the ParlorNet rulebook and every game rulebook in the
folder that holds GameMaster. Custom Install picks which.

1. Open the installer. Its first screen says what it installs; click
   Continue.
2. Install Location names the folder the rulebooks go into. If it is not the
   folder that holds GameMaster, click Choose... and pick that folder. The
   installer installs only into a folder that holds GameMaster.
3. Click Install, then Quit when it says the rulebooks are installed.


Playing
-------

1. Open GameMaster and choose File > New Game > ParlorNet. Sign in, choose a
   game and an opponent from the Opponent menu, and click the button, for
   example "Checkers vs the bot". GameMaster opens the game, connected.
2. A game you are already playing, started on the web, by telnet or in
   GameMaster, can be taken up in GameMaster: click Resume beside it on the
   ParlorNet window's Your games tab. The parlor sends the whole position,
   the board and your rack or tiles, the scores and whose turn it is, and the
   game goes on from there.
3. Click where you move: in Checkers the piece, then each square it lands
   on; in Mancala one of your pits, along the bottom row; in Five in a Row a
   point. In Grid Words click the square the word starts on (again for
   down), then the tiles of your rack, or type the letters, and click Play;
   Exchange... and Pass are beside the rack. In Rummy Tile select tiles of
   your rack or one of the table, then click a set, or New set, Split, Join
   or To rack; End turn (or Draw a tile) ends it. In Cannon Fire set the
   angle and the power with the buttons or the arrow keys, then Fire (or
   Return); Drive left and Drive right move the tank first, once a turn.
   In Backgammon click Roll (or Double), then a checker and the point it
   goes to, or the tray to bear off; Undo takes a step back and Done sends
   the turn. Take and Drop answer a double. Home left puts your home board
   on the left (H on the keyboard), and the parlor remembers it.
   Your move shows on the board once the parlor has checked and accepted
   it; a move it refuses comes back with the reason.
4. Resign is beside the board, and in Checkers Offer draw. Resign asks for a
   second click.
5. In Checkers, Mancala and Five in a Row, against a friend's Mac with the
   same rulebook, start a new game and use GameMaster's Game > Connect, as
   with any rulebook. The other Mac checks each move the same way.


The files
---------

The installer comes as three files, and you need only one of them.

parlornet-rulebooks-installer.hqx  BinHex, for a Mac that decodes BinHex
                                   with StuffIt Expander or BinHex 4.0
parlornet-rulebooks-installer.bin  MacBinary, for a Mac or a download
                                   program that reads MacBinary
parlornet-rulebooks-installer.dsk  an 800K disk image holding the
                                   installer, for an emulator or for
                                   writing a floppy disk


GameMaster
----------

GameMaster is by Rhys Hollow, with Peter N. Lewis and Quinn. These rulebooks
are not affiliated with them and contain none of GameMaster.


The installer
-------------

The installer is made with EUP (https://github.com/eric-cgn/eup). It carries
parts of newlib, Multiversal Interfaces, and the Retro68 and GCC runtimes;
their notices are in EUP's licenses folder.


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
