# Cannon Fire rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays Cannon Fire, ParlorNet's artillery game, by the `classic` ruleset of `cannonfire/1`, for two players ([docs/types/cannonfire-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/cannonfire-1.md) in the ParlorNet repository). Its GMRZ ID string is `Cannon Fire [ParlorNet]`.

It plays only on a ParlorNet parlor, started from the ParlorNet rulebook's window: the parlor makes the landscape and the wind and flies every shell. A game that is not connected to a parlor says so and takes no moves.

## What you need

A Mac Plus or later with System 6.0 or later, GameMaster 1.0.0, and the ParlorNet rulebook. It draws in black and white for the 512 by 342 screen.

## Playing

The landscape fills the top of the window, with tank A drawn white and tank B drawn black, each with its letter and its gun. On your turn, set the angle (0 points right, 90 straight up, 180 left) and the power (0 to 100) with the buttons, or with the arrow keys: left and right turn the gun, up and down change the power. **Fire** (or Return) fires. Before firing you may drive once, **Drive left** or **Drive right**, four times a game. The last shell's path is dotted and its burst ringed; the panel gives both tanks' hit points and the wind. **Resign** asks for a second click.

## Messages and record

Besides the messages every ParlorNet game rulebook uses (`rulebooks/common/rb.h`), moves are the parlor's own tokens, `FIRE angle power` and `MOVE L|R n`. The parlor sends `F seat` and the shell's path (`TRJ x y ...`) for every shot, and the state from its view: `Z` (the terrain), `K` (a tank), `AIM` (a tank's last aim: the view's `A` line, renamed because `A` answers a move), `L` (moves left), `N` (the wind), `H` (the last burst), `B`, `T` (points) and `W` (whose turn it is), which ends it. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`) is the same state between those two lines, and the aim is then taken from the parlor's again.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNcf`, then the arena's width, the terrain, the tanks, the aim and the last shell's path.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/cannonfire-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/cannonfire.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
