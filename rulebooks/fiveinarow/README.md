# Five in a Row rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays Five in a Row on the standard 15 by 15 board, the `freestyle` ruleset of ParlorNet's `fiveinarow/1` ([docs/types/fiveinarow-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/fiveinarow-1.md) in the ParlorNet repository). It plays on a ParlorNet parlor, started from the ParlorNet rulebook's window, and between two Macs with Game > Connect like any other rulebook. Its GMRZ ID string is `Five in a Row [ParlorNet]`.

## What you need

A Mac Plus or later with System 6.0 or later and GameMaster 1.0.0. It draws in black and white for the 512 by 342 screen.

## Playing

Black moves first, and the players take turns, one stone each, on the points where the lines cross. Click a free point to play there. A line of five or more of your stones, across, down or diagonally, wins; a full board with no such line is a draw. The last stone is marked with a dot, and the winning line is struck through. **Resign** is beside the board when the game is connected and asks for a second click.

A game on one Mac, not connected, lets the two players take turns at it.

## Messages and record

The messages are the ones every ParlorNet game rulebook uses (`rulebooks/common/rb.h`, and the table in `rulebooks/checkers/README.md`): a move shows only when the other side has accepted it. A move is a point, a column letter from `a` to `o` and a row number from 1 to 15 counted from the top, like `h8`, as ParlorNet's core takes it. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`) holds `BD` and the 225 points row by row from the top (`0` empty, `1` Black, `2` White), and `LS` and the last stone.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNfr`, then the 225 points row by row from the top (0 empty, 1 Black, 2 White), the last stone and the ends of the winning line.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/fiveinarow-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/fiveinarow.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
