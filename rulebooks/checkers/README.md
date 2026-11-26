# Checkers rulebook for GameMaster

A rulebook for GameMaster 1.0.0, the Macintosh game program of 1991, that plays checkers by American rules, the `american` ruleset of ParlorNet's `checkers/1` ([docs/types/checkers-1.md](https://github.com/eric-cgn/parlornet/blob/main/docs/types/checkers-1.md) in the ParlorNet repository). It plays on a ParlorNet parlor, started from the ParlorNet rulebook's window, and between two Macs with Game > Connect like any other rulebook. Its GMRZ ID string is `Checkers [ParlorNet]`.

## What you need

A Mac Plus or later with System 6.0 or later and GameMaster 1.0.0. It draws in black and white for the 512 by 342 screen.

## Playing

Black moves first. Each player's own pieces are drawn at the bottom of the board. Click a piece, then the square it moves to. For a line of jumps, click each square it lands on; clicking the piece again lets it go. A capture is compulsory, a line of jumps goes on to its end, and a man that reaches the far row is crowned and ends the move there. A side with no piece or no move loses, and forty moves each with no capture and no man moving is a draw. The last move played is framed. **Offer draw** and **Resign** are beside the board when the game is connected; Resign asks for a second click.

A game on one Mac, not connected, lets the two players take turns at it.

## Messages

A move shows on the board only when the other side has accepted it. On a parlor that is the server, which plays it at the table and refuses anything the rules do not allow. Between two Macs it is the other copy of this rulebook, which checks it by the same rules. The messages are the ones every ParlorNet game rulebook uses (`rulebooks/common/rb.h`):

| Message | Meaning |
|---|---|
| `M move` | the sender plays this move |
| `A move` | the move is accepted; the sender of `M` shows it now |
| `R reason` | the last move or offer is refused |
| `D`, `DY`, `DN` | a draw offered, accepted, declined |
| `G` | the sender resigns |
| `E result` | the game is over, in the parlor's words (`win 0 captured`, `draw agreed`, `resign 1`) |
| `POS seat turn offer` ... `POE` | from a parlor only: the whole position, with the seat this Mac plays, the seat to move and a draw offer (the offering seat plus 1, or 0). The game's own lines come between, and the board is drawn again at `POE` |

The parlor sends a whole position first when a game already being played is taken up from the ParlorNet window's Your games (Resume), and again whenever the Mac's board has fallen behind, for example after a move played for this seat on the web. Checkers' own lines are `BD` and the 32 squares as the record keeps them, and `L` with the quiet moves and their limit.

A move is its squares in the standard numbering, joined by `-` for a step and `x` for jumps (`11-15`, `15x22x31`), the form ParlorNet's core takes.

## The game record

The game handle holds a 1024-byte header that all of ParlorNet's game rulebooks share (`RBHead` in `rulebooks/common/rb.h`: the code `PNck`, the layout, the seat this Mac plays, whose turn it is, whether the game is over), then the 32 squares (`.`, `x` and `X` for Black's men and kings, `o` and `O` for White's), the count of quiet moves, and the move being made by clicks. GameMaster uploads it when a game is connected; the parlor takes only a new game.

## Building it

`tools/build.sh` builds it with the Retro68 toolchain in Docker, into `build/checkers-rulebook.bin`, which goes into the installer. The code is one flat code resource (`PROC` 128) of plain 68000 code, from `src/checkers.c` and `rulebooks/common/rb.c`.

## License

MIT; see LICENSE.
