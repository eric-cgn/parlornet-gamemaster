# Grid Words rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays Grid Words, ParlorNet's crossword game, by the `classic` ruleset of `gridwords/1` on the standard 15 by 15 board, for two players ([docs/types/gridwords-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/gridwords-1.md) in the ParlorNet repository). Its GMRZ ID string is `Grid Words [ParlorNet]`.

It plays only on a ParlorNet parlor, started from the ParlorNet rulebook's window: the parlor keeps the bag, deals the racks, checks every word against its word list and keeps the score. A game that is not connected to a parlor says so and takes no moves.

## What you need

A Mac Plus or later with System 6.0 or later, GameMaster 1.0.0, and the ParlorNet rulebook. It draws in black and white for the 512 by 342 screen.

## Playing

The board is on the left. The premium squares are labeled 3W and 2W (triple and double word; the center is a 2W) and 3L and 2L (triple and double letter). Your rack is on the right, each tile with its points.

To play a word, click the empty square it starts on; a word cannot start on a played square. A small arrow shows the direction, and a second click turns it from across to down. Then click the tiles of your rack in order, or type their letters: only your own tiles go down, each on the next empty square along the line, past the tiles already there. The tiles on the board between, just before and just after your tiles join the word. For a blank, click it (or type a letter you do not hold) and type the letter it stands for. A refusal names the play as the board makes it: "Can't play HEX across from g4: ...". Backspace or a click on a placed tile takes tiles back; **Clear** takes them all back. **Play** (or Return) sends the word. The parlor answers with the word's points, or with the reason it refuses it, and only then does the word stay on the board.

**Exchange...** marks tiles to put back (click them, then **Exchange**), while the bag holds seven or more. **Pass** passes. **Resign** asks for a second click. The scores and the tiles left in the bag are above the rack; the last word played is framed.

## Messages and record

Besides the messages every ParlorNet game rulebook uses (`rulebooks/common/rb.h`), the parlor sends `K` and the rack (`_` for a blank, `-` for none), `S` and the two scores, and `B` and the tiles in the bag, when the game starts and after every move. Moves are the parlor's own tokens: `PLAY x y H|V letters` (the whole main word from square x y, the letters already on the board written out and `_` before a blank's letter), `EXCH letters` and `PASS`. The other player's word comes as `PLAY` with the word as it stands on the board, a blank's letter in lower case. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`), sent first when a game being played is taken up from the ParlorNet window, holds `BD` and the 225 squares row by row (`.` empty, a capital letter for a tile, a small one for a blank), `K`, `S`, `B`, and `LP x y H|V length` for the last word played.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNgw`, then the 225 squares row by row from the top (0 empty, a capital letter for a tile, a small one for a blank), the rack, the scores, the bag and the play being built.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/gridwords-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/gridwords.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
