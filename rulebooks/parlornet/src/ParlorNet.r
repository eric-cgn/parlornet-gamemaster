/* The ParlorNet rulebook's resources: its code (PROC 128, built by CMakeLists.txt),
   the GMRZ resource GameMaster reads, the window's one item, and the version. */
#include "Dialogs.r"
#include "MacTypes.r"
#include "GMRZ.r"

data 'PROC' (128, "ParlorNet", locked) {
	$$read("ParlorNet.flt")
};

resource 'GMRZ' (128) {
	9, "ParlorNet", 0, "", 11, "MIT license", 9, "ParlorNet",
	0x0050, 0x0020, 0x0100, 0x0100,
	40000, 2000,
	0xF0, 0, 0
};

resource 'DITL' (128, purgeable) {
	{
		{ 0, 0, 256, 400 }, UserItem { enabled }
	}
};

resource 'vers' (1) {
	0x00, 0x50, development, 0x00, verUS,
	"0.5",
	"0.5, MIT license"
};
