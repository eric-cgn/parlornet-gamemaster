# Backgammon rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays backgammon with the doubling cube, the `standard` ruleset of ParlorNet's `backgammon/1` ([docs/types/backgammon-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/backgammon-1.md) in the ParlorNet repository). Its GMRZ ID string is `Backgammon [ParlorNet]`.

It plays only on a ParlorNet parlor, started from the ParlorNet rulebook's window: the parlor rolls every die and judges every turn. A game that is not connected to a parlor says so and takes no moves.

## What you need

A Mac Plus or later with System 6.0 or later, GameMaster 1.0.0, and the ParlorNet rulebook 0.5 or later, whose window starts Backgammon. It draws in black and white for the 512 by 342 screen.

## The board

Your checkers are at the bottom, with your home board on the right: you move from the top left round to the bottom right and bear off into the tray at the right. White's checkers are drawn white and Black's black; the colors go by seat, and whoever wins the opening roll moves first. A point holding more than five checkers shows four, and in the fifth place a label with the number of checkers not drawn (+2 for six, +4 for eight); the bar does the same. Checkers on the bar sit on the middle bar, yours below the middle and the other player's above it, and checkers borne off are slabs in the tray, yours from the bottom. The cube sits on the bar showing 64 until someone doubles; then it moves into the tray on its owner's side with its value. The dice of the turn are on the mover's side of the board, four for doubles, and each is drawn faint once it is played. Small marks at the tips of the points show the last turn played, hollow where a checker left and filled where it went.

**Home left** draws the mirror image, with your home board and the tray on the left; **Home right** puts them back. The points and the moves stay the same. H on the keyboard does the same. The parlor keeps the choice with your name and draws your next game that way.

## Playing

When your turn starts with the cube in reach, click **Roll**, or **Double** to offer the cube at twice its value; with the cube out of reach the parlor rolls for you. Then click a checker and the point it goes to, or the tray to bear off. A picked-up checker shows a dot in its middle, and the points it can reach a white checker with a dotted rim, and a click on a point only one checker can reach moves that one. A checker on the bar must come in first; click the point where it enters. The rulebook allows only the steps the rules do: each die once, as many dice as the roll can be played with, and the larger die when only one of the two can be played. **Undo** takes back the last step, and **Done** sends the whole turn once nothing more can be played. The parlor checks it again, plays it, and the board shows the turn when the parlor's answer comes. A turn it refuses comes back with the reason, and the board is as it was before the turn.

When the other player doubles, **Take** plays on at the new value and gives you the cube, and **Drop** gives up the game at the old value. When the other player offers to resign, **Accept** ends the game with the points offered and **Reject** plays on. **Resign** in the panel asks for a second click. The panel also says what you rolled when no step could be played (the parlor passes the turn), and the pip counts.

On the keyboard, R rolls, D or Return sends the turn, U or Delete takes a step back, and H turns the board round.

## Messages and record

Besides the messages every ParlorNet game rulebook uses (`rulebooks/common/rb.h`), the Mac sends a turn as `M` and its steps from the parlor's point numbers (`M 13-9 9-4`, `M bar-22 13-7`, `M 6-off 5-off`), and `M roll`, `M double`, `M take`, `M drop`, `M accept` and `M reject` as the parlor's own words; `M end` ends a turn whose steps were all played on the parlor already. `H left` or `H right` tells the parlor which side the player keeps the home board on.

After every change the parlor sends the whole state:

| Line | Meaning |
|---|---|
| `BD` and 28 numbers | the points 1 to 24, White's checkers counted up and Black's down, then the checkers on the bar and borne off, White's first |
| `CU value owner` | the cube, its owner `0`, `1` or `-` in the middle |
| `DI d ...` | the dice still to play, in the parlor's order |
| `DR d d` | the dice rolled this turn |
| `N n` | the steps the roll needs |
| `CA a b` | 1 where that seat may double when its turn starts |
| `RS seat points kind` | a resignation offered and not answered yet |
| `LM seat d d steps` | the last turn played, its dice and steps (`*` for a hit), or `pass` |
| `Q phase` | `roll`, `move`, `take`, `resign` or `over` |
| `W seat` | whose turn it is; it ends the state |

The answer `A` to the Mac's move comes after the state that move leads to. Before the first state the parlor sends `H` with the side the player keeps. A whole position from the parlor (`POS` ... `POE`, as in `rulebooks/checkers/README.md`) is the same state between those two lines.

The game handle holds the 1024-byte header the game rulebooks share, with the code `PNbg`, then the 24 points (the starting position in a new game), the bar and the tray, the rest of the state the parlor sent, the side of the home board, and the steps made by clicks and not sent yet.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/backgammon-rulebook.bin`, `.hqx` and `.dsk`. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/backgammon.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
