/* The Grid Words rulebook's resources: its code (PROC 128, built by CMakeLists.txt), the GMRZ resource
   GameMaster reads, the window's one item (the play area), and the version. */
#include "Dialogs.r"
#include "MacTypes.r"
#include "GMRZ.r"

data 'PROC' (128, "Grid Words", locked) {
	$$read("GridWords.flt")
};

resource 'GMRZ' (128) {
	10, "Grid Words", 0, "", 11, "MIT license", 22, "Grid Words [ParlorNet]",
	0x0120, 0x0100, 0x0100, 0x0100,
	30000, 3000,
	0x00, 0, 0
};

resource 'DITL' (128, purgeable) {
	{
		{ 0, 0, 236, 432 }, UserItem { enabled }
	}
};

resource 'vers' (1) {
	0x01, 0x20, release, 0x00, verUS,
	"1.2.0",
	"1.2.0, MIT license"
};
