/* The Rummy Tile rulebook's resources: its code (PROC 128, built by CMakeLists.txt), the GMRZ resource
   GameMaster reads, the window's one item (the play area), and the version. */
#include "Dialogs.r"
#include "MacTypes.r"
#include "GMRZ.r"

data 'PROC' (128, "Rummy Tile", locked) {
	$$read("RummyTile.flt")
};

resource 'GMRZ' (128) {
	10, "Rummy Tile", 0, "", 11, "MIT license", 22, "Rummy Tile [ParlorNet]",
	0x0111, 0x0100, 0x0100, 0x0100,
	30000, 3000,
	0x00, 0, 0
};

resource 'DITL' (128, purgeable) {
	{
		{ 0, 0, 256, 500 }, UserItem { enabled }
	}
};

resource 'vers' (1) {
	0x01, 0x11, release, 0x00, verUS,
	"1.1.1",
	"1.1.1, MIT license"
};
