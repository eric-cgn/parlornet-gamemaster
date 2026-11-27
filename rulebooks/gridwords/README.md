# Grid Words rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays Grid Words, ParlorNet's crossword game, on the 15 by 15 board of `gridwords/1`, for two players ([docs/types/gridwords-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/gridwords-1.md) in the ParlorNet repository). It plays every ruleset of the game. The parlor sends the bonus squares and the letter values of the table's own ruleset, and the rulebook draws what it is sent. Its GMRZ ID string is `Grid Words [ParlorNet]`.

It plays only on a ParlorNet parlor, started from the ParlorNet rulebook's window: the parlor keeps the bag, deals the racks, checks every word against its word list and keeps the score. A game that is not connected to a parlor says so and takes no moves.

## What you need

A Mac Plus or later with System 6.0 or later, GameMaster 1.0.0, and the ParlorNet rulebook. It draws in black and white for the 512 by 342 screen.

## Playing

The board is on the left. Its bonus squares are labeled 3W and 2W for triple and double word, and 3L and 2L for triple and double letter, wherever the table's ruleset puts them. Your rack is on the right, and each tile shows its points by the table's ruleset. Until the parlor has sent the board and the letter values, the board has no labels and the tiles have no numbers.

To play a word, click the empty square it starts on; a word cannot start on a played square. A small arrow shows the direction, and a second click turns it from across to down. Then click the tiles of your rack in order, or type their letters: only your own tiles go down, each on the next empty square along the line, past the tiles already there. The tiles on the board between, just before and just after your tiles join the word. For a blank, click it (or type a letter you do not hold) and type the letter it stands for. A refusal names the play as the board makes it: "Can't play HEX across from g4: ...". Backspace or a click on a placed tile takes tiles back; **Clear** takes them all back. **Play** (or Return) sends the word. The parlor answers with the word's points, or with the reason it refuses it, and only then does the word stay on the board.

**Exchange...** marks tiles to put back (click them, then **Exchange**), while the bag holds seven or more. **Pass** passes. **Resign** asks for a second click. The scores and the tiles left in the bag are above the rack; the last word played is framed.

## Messages and record

Besides the messages every ParlorNet game rulebook uses (`rulebooks/common/rb.h`), the parlor sends `K` and the rack (`_` for a blank, `-` for none), `S` and the two scores, and `B` and the tiles in the bag, when the game starts and after every move. Moves are the parlor's own tokens: `PLAY x y H|V letters` (the whole main word from square x y, the letters already on the board written out and `_` before a blank's letter), `EXCH letters` and `PASS`. The other player's word comes as `PLAY` with the word as it stands on the board, a blank's letter in lower case. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`), sent first when a game being played is taken up from the ParlorNet window, holds `BD` and the 225 squares row by row (`.` empty, a capital letter for a tile, a small one for a blank), `K`, `S`, `B`, and `LP x y H|V length` for the last word played.

The parlor also sends the table's board and letter values, once when the game starts and again before every whole position. `SQ` comes with the 225 squares row by row from the top left: `.` for a plain square, `d` and `t` for double and triple letter, and `D` and `T` for double and triple word. `LV` comes with the points of A to Z, 26 numbers from 0 to 10 separated by spaces. A line that is not well formed leaves the board or the points as they were.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNgw`, then the 225 squares row by row from the top (0 empty, a capital letter for a tile, a small one for a blank), the rack, the scores, the bag and the play being built. Last come the table's 225 squares and the 26 letter values as the parlor last sent them, so a saved game is drawn with its own board. The record's format is 2. A record of format 1, from an earlier version of the rulebook, opens as a new game.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/gridwords-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/gridwords.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
