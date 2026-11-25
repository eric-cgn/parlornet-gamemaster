/* The Checkers rulebook's resources: its code (PROC 128, built by CMakeLists.txt), the GMRZ resource
   GameMaster reads, the window's one item (the play area), and the version. */
#include "Dialogs.r"
#include "MacTypes.r"
#include "GMRZ.r"

data 'PROC' (128, "Checkers", locked) {
	$$read("Checkers.flt")
};

resource 'GMRZ' (128) {
	8, "Checkers", 0, "", 11, "MIT license", 20, "Checkers [ParlorNet]",
	0x0111, 0x0100, 0x0100, 0x0100,
	30000, 2000,
	0x00, 0, 0
};

resource 'DITL' (128, purgeable) {
	{
		{ 0, 0, 220, 352 }, UserItem { enabled }
	}
};

resource 'vers' (1) {
	0x01, 0x11, release, 0x00, verUS,
	"1.1.1",
	"1.1.1, MIT license"
};
