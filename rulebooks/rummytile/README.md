# Rummy Tile rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays Rummy Tile, ParlorNet's tile rummy, by the `standard` ruleset of `rummytile/1`, for two players ([docs/types/rummytile-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/rummytile-1.md) in the ParlorNet repository). Its GMRZ ID string is `Rummy Tile [ParlorNet]`.

It plays only on a ParlorNet parlor, started from the ParlorNet rulebook's window: the parlor keeps the pool, deals the racks and checks every step and every turn. A game that is not connected to a parlor says so and takes no moves.

## What you need

A Mac Plus or later with System 6.0 or later, GameMaster 1.0.0, and the ParlorNet rulebook. It draws in black and white for the 512 by 342 screen.

## The tiles

Each tile shows its number and, under it, its color's letter: R red, B blue, K black, Y yellow, and J for a joker. The colors also differ in the drawing: a red tile is plain, a blue one has a dotted frame inside, a black one is drawn in reverse, a yellow one has a double frame.

## Playing

The table is at the top, its sets in rows; a set that is not valid yet has a broken line under it, and a set made or changed this turn a solid one. Your rack is below, and the panel beside it says what ending the turn now would do.

On your turn, click tiles of your rack to select them, then click a set on the table to add them to it, or **New set**. Click a tile on the table to select it, then click another set to move it there, or **New set**, **Split** (the set is split before that tile), **Join** and then another set (its set joins onto that one), or **To rack** (only a tile you played this turn). Each step goes to the parlor at once, and the table you see is the one the parlor keeps. **Undo** takes the last step back, **Start over** the whole turn. **End turn** ends it when it counts; otherwise the button says **Draw a tile**, and ending the turn takes it back and draws one. **Resign** asks for a second click.

## Messages and record

Besides the messages every ParlorNet game rulebook uses (`rulebooks/common/rb.h`), each step is sent as the parlor's own token (`R7B7K7-n`, `R7-3`, `3.2-5`, `3.2-n`, `3.2-r`, `3-5`, `3/4`, `reset`), and `UND` and `END`. After each of them, and whenever the turn passes, the parlor sends the whole state: `Z`, then `ST` and each set, `H` and the rack, `TP` (what ending the turn would do), `P` (the pool), `C` (the racks' sizes), `I` (the first melds) and `W` (whose turn it is), which ends it. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`) is the same state between those two lines.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNrt`, then the number of sets on the table, the sets, the rack and the rest of the state the parlor sent.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/rummytile-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/rummytile.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
