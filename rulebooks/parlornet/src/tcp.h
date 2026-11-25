/*
 * One TCP connection through MacTCP (or Open Transport's MacTCP interface), with
 * synchronous calls only, and the name resolver. Written from the interfaces
 * that Apple documented for MacTCP, which Retro68 does not ship.
 */
#ifndef TCP_H
#define TCP_H

#include <Types.h>

OSErr TcpResolve(const unsigned char *host, unsigned long *addr);   /* a name or a dotted address */
OSErr TcpOpen(unsigned long addr, unsigned short port);
OSErr TcpSend(const void *data, unsigned short len);
long TcpRecv(void *buf, unsigned short max);                         /* bytes read now, or < 0 when closed */
void TcpClose(void);
Boolean TcpIsOpen(void);

#endif
