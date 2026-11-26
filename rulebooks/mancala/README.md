# Mancala rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays Mancala by Kalah rules with four seeds a pit, the `kalah` ruleset of ParlorNet's `mancala/1` ([docs/types/mancala-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/mancala-1.md) in the ParlorNet repository). It plays on a ParlorNet parlor, started from the ParlorNet rulebook's window, and between two Macs with Game > Connect like any other rulebook. Its GMRZ ID string is `Mancala [ParlorNet]`.

## What you need

A Mac Plus or later with System 6.0 or later and GameMaster 1.0.0. It draws in black and white for the 512 by 342 screen.

## Playing

South moves first. Each player's own row of six pits is drawn along the bottom, numbered 1 to 6 from the left, with that player's store on the right; the other player's row runs along the top, right to left, with their store on the left. Click one of your pits to sow it: its seeds go one by one counterclockwise, into your store and along the other row, never into the other store. A last seed in your store gives you another move. A last seed in an empty pit of your own row captures it and the seeds in the pit opposite, when there are any. When either row is empty, each player adds the seeds left on their own row to their store, and the bigger store wins. The pit last played is ringed. **Resign** is beside the board when the game is connected and asks for a second click.

A game on one Mac, not connected, lets the two players take turns at it, each from their own row.

## Messages and record

The messages are the ones every ParlorNet game rulebook uses (`rulebooks/common/rb.h`, and the table in `rulebooks/checkers/README.md`): a move shows only when the other side has accepted it. A move is the pit's number, 1 to 6, counted from the mover's left, as ParlorNet's core takes it. A turn that goes on after a last seed in the store is two or more moves, each sent and accepted on its own. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`) holds `BD` and the 14 counts in sowing order, and `LM` with the seat and the pit of the last move.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNmc`, then the 14 counts in sowing order (South's pits 1 to 6, South's store, North's pits 1 to 6, North's store), the seeds a pit started with, and the last move.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/mancala-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/mancala.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
