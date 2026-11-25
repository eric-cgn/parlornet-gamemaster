# Contributing to ParlorNet's rulebooks for GameMaster

Contributions are welcome: a fix, a test, a new game rulebook, a report of the rulebooks on a Mac or a system they have not been tried on, or a correction to the docs. The rulebooks are part of [ParlorNet](https://github.com/eric-cgn/parlornet); changes to the server, its GameMaster extension, its docs site or the rulebooks' pages there go to the ParlorNet repository, and its [CONTRIBUTING.md](https://github.com/eric-cgn/parlornet/blob/main/CONTRIBUTING.md) applies here too. A new game rulebook needs the game on the server first ([Adding a game](https://github.com/eric-cgn/parlornet/blob/main/docs/games-howto.md)), and a change to the messages a rulebook and the extension exchange needs a matching change to the extension.

Pull requests go to the `dev` branch; `main` takes only merges from `dev`, as releases (see Branches and releases in the README). Before you send a change, build it with `tools/build.sh` and run `python3 -m unittest discover -s tests`. A rulebook that changes gets a new version in its `vers` and `GMRZ` resources, and the installer a new version in `rulebooks/installer/rulebooks.ini`.

## Names, trademarks and terms

Many of the games ParlorNet plays are old and free for anyone to make, but some of their best-known versions carry trademarks: in their names, their logos, and sometimes in the look of their boards and pieces. ParlorNet uses names of its own for its games (Grid Words, Rummy Tile, Cannon Fire) and draws them in its own way. Please do the same in anything you contribute: describe a game in your own words, use ParlorNet's names for it, and don't copy a commercial product's names, artwork or look.

Many period programs also came with terms: a license, a shareware notice, a service's terms of use. Some of those terms rule out the kind of study or change that working with ParlorNet would need, and some programs can't be part of ParlorNet at all because of them. Before working with GameMaster or any other program, read the terms it came with, and work only in ways those terms allow. Don't accept terms on someone else's behalf, and don't contribute anything learned in a way the terms forbid.

This project won't accept contributions that infringe trademarks, break agreements, or otherwise run afoul of legal or social obligations, including the respect owed to the people and communities who made and still keep these games alive. Where exactly those lines fall depends on the program, the terms and the circumstances, and the project's maintainers make the final call. If you're not sure, ask before you start, or leave it out. A contribution can be declined or removed later if it turns out to cross one of these lines.

## Other people's software

The rulebooks are written from GameMaster's published rulebook interface. They never include code, files or artwork from GameMaster or its own rulebooks, and nothing here hosts, bundles or redistributes them. Code under an open-source license may be read for facts about a protocol or a file format, but the rulebooks' code is their own, under the MIT license. The installer is made with EUP, whose installer program carries other people's code under their own notices (see the README); a change that adds anything else of someone else's to the release files needs its license to allow that and its notice to go with it.
