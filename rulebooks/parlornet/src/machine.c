/*
 * The Mac's model.
 *
 * Gestalt came with System 6.0.4. Its gestaltMachineType selector ('mach') gives
 * a number for the model, and the table below names every number Apple's
 * Gestalt.h defines for it. Where one number stands for several models (the
 * Performas sold under another number, or the Power Macintosh 5200 and 5260),
 * the table gives the first of them. An emulator answers with the model it
 * plays. A number missing here, or a system older than 6.0.4, gives "Macintosh".
 *
 * Checked against Apple's CarbonCore/Gestalt.h ("Copyright 1988-2011 by Apple
 * Inc.", as in the Mac OS X 10.15 SDK): its gestaltMachineType enum has the same
 * 105 numbers as this table, gestaltClassic (1) to gestaltMacOSCompatibility
 * (1206). Retro68's pinned image ships only the Multiversal Interfaces, whose
 * Gestalt.h defines none of these numbers but gestaltMacQuadra610 (53), and not
 * Apple's Universal Interfaces.
 */
#include <Gestalt.h>
#include <OSUtils.h>
#include <Traps.h>

#include "machine.h"

typedef struct {
	short id;
	const char *name;
} Model;

/* by gestaltMachineType, in order */
static const Model gModels[] = {
	{ 1, "Macintosh 128K" },
	{ 2, "Macintosh XL" },
	{ 3, "Macintosh 512Ke" },
	{ 4, "Macintosh Plus" },
	{ 5, "Macintosh SE" },
	{ 6, "Macintosh II" },
	{ 7, "Macintosh IIx" },
	{ 8, "Macintosh IIcx" },
	{ 9, "Macintosh SE/30" },
	{ 10, "Macintosh Portable" },
	{ 11, "Macintosh IIci" },
	{ 12, "Power Macintosh 8100/120" },
	{ 13, "Macintosh IIfx" },
	{ 17, "Macintosh Classic" },
	{ 18, "Macintosh IIsi" },
	{ 19, "Macintosh LC" },
	{ 20, "Macintosh Quadra 900" },
	{ 21, "PowerBook 170" },
	{ 22, "Macintosh Quadra 700" },
	{ 23, "Macintosh Classic II" },
	{ 24, "PowerBook 100" },
	{ 25, "PowerBook 140" },
	{ 26, "Macintosh Quadra 950" },
	{ 27, "Macintosh LC III" },
	{ 29, "PowerBook Duo 210" },
	{ 30, "Macintosh Centris 650" },
	{ 32, "PowerBook Duo 230" },
	{ 33, "PowerBook 180" },
	{ 34, "PowerBook 160" },
	{ 35, "Macintosh Quadra 800" },
	{ 36, "Macintosh Quadra 650" },
	{ 37, "Macintosh LC II" },
	{ 38, "PowerBook Duo 250" },
	{ 39, "Workgroup Server 9150/80" },
	{ 40, "Power Macintosh 8100/110" },
	{ 41, "Power Macintosh 5200" },
	{ 42, "Power Macintosh 6200" },
	{ 44, "Macintosh IIvi" },
	{ 45, "Macintosh IIvm" },
	{ 47, "Power Macintosh 7100/80" },
	{ 48, "Macintosh IIvx" },
	{ 49, "Macintosh Color Classic" },
	{ 50, "PowerBook 165c" },
	{ 52, "Macintosh Centris 610" },
	{ 53, "Macintosh Quadra 610" },
	{ 54, "PowerBook 145" },
	{ 55, "Power Macintosh 8100/100" },
	{ 56, "Macintosh LC 520" },
	{ 57, "Workgroup Server 9150/120" },
	{ 58, "Power Macintosh 6400" },
	{ 60, "Macintosh Centris 660AV" },
	{ 62, "Macintosh Performa 460" },
	{ 65, "Power Macintosh 8100/80" },
	{ 67, "Power Macintosh 9500" },
	{ 68, "Power Macintosh 7500" },
	{ 69, "Power Macintosh 8500" },
	{ 71, "PowerBook 180c" },
	{ 72, "PowerBook 520" },
	{ 74, "Power Macintosh 5400" },
	{ 75, "Power Macintosh 6100/60" },
	{ 77, "PowerBook Duo 270c" },
	{ 78, "Macintosh Quadra 840AV" },
	{ 80, "Macintosh Performa 550" },
	{ 84, "PowerBook 165" },
	{ 85, "PowerBook 190" },
	{ 88, "Macintosh TV" },
	{ 89, "Macintosh LC 475" },
	{ 92, "Macintosh LC 575" },
	{ 94, "Macintosh Quadra 605" },
	{ 98, "Macintosh Quadra 630" },
	{ 99, "Macintosh LC 580" },
	{ 100, "Power Macintosh 6100/66" },
	{ 102, "PowerBook Duo 280" },
	{ 103, "PowerBook Duo 280c" },
	{ 104, "Macintosh LC 475 (PowerPC card)" },
	{ 105, "Macintosh LC 575 (PowerPC card)" },
	{ 106, "Macintosh Quadra 630 (PowerPC card)" },
	{ 108, "Power Macintosh 7200" },
	{ 109, "Power Macintosh 7300" },
	{ 112, "Power Macintosh 7100/66" },
	{ 115, "PowerBook 150" },
	{ 116, "Macintosh Quadra 700 (PowerPC card)" },
	{ 117, "Macintosh Quadra 900 (PowerPC card)" },
	{ 118, "Macintosh Quadra 950 (PowerPC card)" },
	{ 119, "Macintosh Centris 610 (PowerPC card)" },
	{ 120, "Macintosh Centris 650 (PowerPC card)" },
	{ 121, "Macintosh Quadra 610 (PowerPC card)" },
	{ 122, "Macintosh Quadra 650 (PowerPC card)" },
	{ 123, "Macintosh Quadra 800 (PowerPC card)" },
	{ 124, "PowerBook Duo 2300" },
	{ 126, "PowerBook 500 (PowerPC card)" },
	{ 128, "PowerBook 5300" },
	{ 306, "PowerBook 3400" },
	{ 307, "PowerBook 2400" },
	{ 310, "PowerBook 1400" },
	{ 312, "PowerBook G3 Series" },
	{ 313, "PowerBook G3" },
	{ 314, "PowerBook G3 Series" },
	{ 406, "Macintosh (New World ROM)" },
	{ 510, "Power Macintosh G3" },
	{ 512, "Power Macintosh 5500" },
	{ 513, "Power Macintosh 6500" },
	{ 514, "Power Macintosh 4400/160" },
	{ 515, "Power Macintosh 4400" },
	{ 1206, "Mac OS X (Classic)" }
};

enum { kModels = sizeof gModels / sizeof gModels[0] };

const char *MachineName(void)
{
	long mach;
	short lo = 0, hi = kModels - 1, mid;

	/* _Gestalt is an OS trap; on a system without it, it is _Unimplemented */
	if (NGetTrapAddress(0xA1AD, kOSTrapType) == NGetTrapAddress(0xA89F, kToolboxTrapType)
			|| Gestalt(gestaltMachineType, &mach) != noErr)
		return "Macintosh";
	while (lo <= hi) {
		mid = (lo + hi) / 2;
		if (gModels[mid].id == mach)
			return gModels[mid].name;
		if (gModels[mid].id < mach)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return "Macintosh";
}
