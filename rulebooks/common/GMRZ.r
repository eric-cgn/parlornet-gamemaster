/* GameMaster's GMRZ resource: what GameMaster reads of a rulebook (Developer's Kit manual, section 3.3).
   The four strings are Pascal strings in fields of 64, 128, 128 and 64 bytes. Each is written as its length
   and its text, because Rez's fixed-length pstring stores the field's length, not the text's. */
type 'GMRZ' {
	byte; string[63];           /* the game's name */
	byte; string[127];          /* author */
	byte; string[127];          /* copyright */
	byte; string[63];           /* ID string, matched with the other side's */
	hex integer;                /* rulebook version */
	hex integer;                /* oldest rulebook version it connects to */
	hex integer;                /* GameMaster version it was written for */
	hex integer;                /* oldest GameMaster it runs with */
	longint;                    /* memory for the rulebook */
	longint;                    /* memory for each game */
	hex byte;                   /* no save, no connect, no restart, no swap, no idle, no sleep query, 0, 0 */
	byte;
	integer;
};
