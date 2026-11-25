# ParlorNet rulebooks for GameMaster

This repository holds ParlorNet's rulebooks for GameMaster 1.0.0, the Macintosh game program of 1991, and Install ParlorNet Rulebooks, the installer that puts them on a Mac. The ParlorNet rulebook connects GameMaster to a ParlorNet parlor: its window signs the player in, starts games against the bot or another player, takes invitations, and takes up games already being played. The seven game rulebooks play Checkers, Mancala, Five in a Row, Grid Words, Rummy Tile, Cannon Fire and Backgammon on a parlor, and the first three also play between two Macs with GameMaster's Game > Connect.

They are part of [ParlorNet](https://github.com/eric-cgn/parlornet), a server for period game programs. The ParlorNet repository holds the server, its GameMaster extension, its docs and its docs site, which serves the installer. GameMaster itself is not included here.

| Folder | What it is | Version |
|---|---|---|
| `rulebooks/parlornet` | the ParlorNet rulebook: sign-in, the lobby, and every game over one connection to the parlor | 0.5 |
| `rulebooks/checkers` | Checkers, American rules (`Checkers [ParlorNet]`) | 1.1.1 |
| `rulebooks/mancala` | Mancala, Kalah rules with four seeds a pit (`Mancala [ParlorNet]`) | 1.1.1 |
| `rulebooks/fiveinarow` | Five in a Row on a 15 by 15 board (`Five in a Row [ParlorNet]`) | 1.1.1 |
| `rulebooks/gridwords` | Grid Words, the crossword game, on a parlor only (`Grid Words [ParlorNet]`) | 1.1.1 |
| `rulebooks/rummytile` | Rummy Tile, tile rummy for two, on a parlor only (`Rummy Tile [ParlorNet]`) | 1.1.1 |
| `rulebooks/cannonfire` | Cannon Fire, two tanks shelling each other, on a parlor only (`Cannon Fire [ParlorNet]`) | 1.1.1 |
| `rulebooks/backgammon` | Backgammon with the doubling cube, on a parlor only (`Backgammon [ParlorNet]`) | 1.1.1 |
| `rulebooks/common` | the code and the messages the game rulebooks share (`rb.c`, `rb.h`), and GameMaster's `GMRZ` resource type (`GMRZ.r`), which every rulebook here uses | |
| `rulebooks/installer` | Install ParlorNet Rulebooks: the EUP description (`rulebooks.ini`) and its first screen (`About.txt`) | 1.4 |
| `release` | the READMEs that come with the downloads | |

Each rulebook's own README says how it plays, the messages it sends and how its game record is kept.

## What they run on

A Mac Plus or later with System 6.0 or later, and GameMaster 1.0.0. Playing on a parlor also needs MacTCP or Open Transport's TCP/IP. The installer needs System 6.0.3 or later. The rulebooks draw in black and white for the 512 by 342 screen, and they are plain 68000 code, so they run on every Mac that runs GameMaster. The ParlorNet rulebook works only with GameMaster 1.0.0 as it was released: it checks GameMaster's code first, and if the copy differs, the lobby still opens but no game starts from it.

## The ParlorNet server they work with

The rulebooks talk to the GameMaster extension of the ParlorNet server (`adapters/gamemaster.py` in the ParlorNet repository), which listens on port 6508. These versions work with the server as of November 2026:

- The ParlorNet rulebook 0.5 opens its connection with `PARLORNET/1`, the first version of ParlorNet's client protocol, and the extension carries every GameMaster connection inside that one connection. From 0.4 the rulebook also names the program and the Mac with the protocol's `CLI PROG` and `CLI MACH`; the extension accepts a rulebook that does not. Version 0.5 adds Backgammon to the games its window starts.
- The game rulebooks 1.1.1 carry version `0x0111` in their `GMRZ` resource and connect to versions from `0x0100` on. Backgammon starts at 1.1.1 with the others, and plays only with an extension that knows its ID string, `Backgammon [ParlorNet]`. The extension plays a rulebook whose oldest connectable version is at most 1.0.0 and whose own version is at least 1.0.0, and it sends a whole position (`POS` ... `POE`) to rulebooks from 1.1.0, which is what lets a game be resumed.
- GameMaster's own Othello, Dots&Boxes and BattleShips rulebooks play through the ParlorNet rulebook too.

A change to the messages the rulebooks and the extension exchange needs a change on both sides, so a new rulebook version and the server change that goes with it are released together.

## Getting it

The [game rulebooks page](http://docs.parlornet.org/gamemaster-rulebooks.html) of ParlorNet's docs site has Install ParlorNet Rulebooks over plain HTTP, which a period browser can fetch, as `parlornet-rulebooks-installer.hqx` (BinHex) and `.bin` (MacBinary). The [ParlorNet rulebook page](http://docs.parlornet.org/parlornet-rulebook.html) describes the lobby. The same files are on this repository's GitHub Releases with their SHA-256 and MD5 sums, along with each rulebook on its own as `.hqx`, `.bin` and an 800K HFS disk image (`.dsk`) for emulators, and the two READMEs.

## Installing

Decode the installer with StuffIt Expander or BinHex 4.0 and open it. Its first screen says what it installs. Install Location must name the folder that holds GameMaster; if it does not, click Choose... and pick that folder. Click Install, then open GameMaster and choose File > New Game > ParlorNet. Custom Install picks which rulebooks to install. GameMaster's own rulebooks stay as they are.

A single rulebook can also be decoded from its `.hqx` or `.bin`, or copied from its `.dsk`, into the folder that holds GameMaster.

## Building

You need Docker, Python 3, git and a C compiler. The rulebooks are built with [Retro68](https://github.com/autc04/Retro68) (GCC for classic Mac OS) from its Docker image, pinned by digest in the build script, and the installer with [EUP](https://github.com/eric-cgn/eup), which the build script fetches at a pinned commit (release 1.4) and builds with its Makefile. From the top of the repository:

```sh
tools/build.sh
```

It writes `build/parlornet-rulebook.bin`, `.hqx` and `.dsk`, the same for each game rulebook (`build/checkers-rulebook.bin`, ..., `build/backgammon-rulebook.bin`), and `build/parlornet-rulebooks-installer.bin` and `.hqx`. The dates in the files are set to the release date by `tools/patcher_dates.py`, and the installer's by `SOURCE_DATE_EPOCH`, so the same source always gives the same files; the `.hqx` files are made from the `.bin` files by `tools/binhex4.py`. It also puts the release files in `build/release/`, with `SHA256SUMS` and `MD5SUMS`. `RETRO68_IMAGE` names another toolchain image, and `EUP` an `eup` command to use instead of the pinned one.

`python3 -m unittest discover -s tests` checks the sources, the versions, the build script and the release READMEs, and the built files once `tools/build.sh` has run.

## Branches and releases

Changes go into `dev` by pull request, squashed; `main` takes only merges from `dev`, which is how a release is made. Every pull request and every push to `main` builds and tests (`.github/workflows/ci.yml`), and a pull request into `main` fails unless it comes from `dev`. Every push to `dev` publishes a `dev` pre-release that replaces the last one, with files named for the installer's version, `-dev-` and the commit, such as `parlornet-rulebooks-installer-1.4-dev-46a0304.hqx` (`.github/workflows/dev.yml`).

A push to `main` whose installer version (the one in `rulebooks/installer/rulebooks.ini`) has no release yet runs the release (`.github/workflows/release.yml`). It builds the files the same way, runs the tests, tags the commit `v<version>` (`v1.4` for Install ParlorNet Rulebooks 1.4), and publishes the files as a GitHub Release with their SHA-256 and MD5 sums. ParlorNet's docs site takes the installer and the READMEs from that release, at a tag it pins, and checks each file's SHA-256 before it serves it.

## License and notices

The rulebooks and the build tools are ParlorNet's own code, under the MIT license; see [LICENSE](LICENSE). Each rulebook's folder has the same license.

The installer is made with EUP, which is under the MIT license too. The installer program inside it carries parts of newlib, Multiversal Interfaces, and the Retro68 and GCC runtimes, and their notices are in the `licences` folder of [EUP's repository](https://github.com/eric-cgn/eup). The game rulebooks page and the installer's README say the same.

GameMaster is by Rhys Hollow, with Peter N. Lewis and Quinn. These rulebooks are not affiliated with them and contain none of GameMaster.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).
