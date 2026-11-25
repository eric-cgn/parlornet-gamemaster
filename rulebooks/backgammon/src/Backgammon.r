/* The Backgammon rulebook's resources: its code (PROC 128, built by CMakeLists.txt), the GMRZ resource
   GameMaster reads, the window's one item (the play area), and the version. */
#include "Dialogs.r"
#include "MacTypes.r"
#include "GMRZ.r"

data 'PROC' (128, "Backgammon", locked) {
	$$read("Backgammon.flt")
};

resource 'GMRZ' (128) {
	10, "Backgammon", 0, "", 11, "MIT license", 22, "Backgammon [ParlorNet]",
	0x0111, 0x0100, 0x0100, 0x0100,
	30000, 2000,
	0x00, 0, 0
};

resource 'DITL' (128, purgeable) {
	{
		{ 0, 0, 256, 496 }, UserItem { enabled }
	}
};

resource 'vers' (1) {
	0x01, 0x11, release, 0x00, verUS,
	"1.1.1",
	"1.1.1, MIT license"
};
