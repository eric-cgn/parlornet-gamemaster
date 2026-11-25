/*
 * The Mac's model, for the parlor's record of what each turn was played on.
 */
#ifndef MACHINE_H
#define MACHINE_H

/* The model's name from Gestalt's gestaltMachineType ('mach'), such as "Macintosh SE/30", or "Macintosh"
   for a value not in the table or a system without Gestalt. At most 39 characters and the 0. */
const char *MachineName(void);

#endif
