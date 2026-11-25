/*
 * MacTCP: one stream, synchronous calls. Reading never waits: TCPStatus says how
 * much has arrived and TCPRcv takes only that.
 *
 * The name resolver is the 'dnrp' code resource of the MacTCP (or TCP/IP)
 * control panel, called as Apple's resolver glue calls it.
 */
#include <Files.h>
#include <Devices.h>
#include <Memory.h>
#include <Resources.h>
#include <OSUtils.h>
#include <Events.h>
#include <Gestalt.h>
#include <Traps.h>
#include <string.h>

#include "tcp.h"

/* ---- MacTCP (its parameter block and codes are in Retro68's interfaces) ------------------------------ */

typedef struct {
	wdsEntry e;
	uint16_t end;
} WDS;

#ifndef kOnSystemDisk
#define kOnSystemDisk (-32768)
#endif

enum { kRcvBuffer = 8192 };

static short gRef;
static Ptr gStream, gRcvBuff;

static OSErr Call(TCPiopb *pb, short code)
{
	pb->ioCRefNum = gRef;
	pb->csCode = code;
	pb->tcpStream = (StreamPtr)gStream;
	return PBControlSync((ParmBlkPtr)pb);
}

Boolean TcpIsOpen(void)
{
	return gStream != NULL;
}

OSErr TcpOpen(unsigned long addr, unsigned short port)
{
	TCPiopb pb;
	OSErr err;

	if (gStream)
		TcpClose();
	if (!gRef && (err = OpenDriver((ConstStringPtr)"\p.IPP", &gRef)) != noErr)
		return err;
	gRcvBuff = NewPtr(kRcvBuffer);
	if (!gRcvBuff)
		return memFullErr;
	memset(&pb, 0, sizeof pb);
	pb.csParam.create.rcvBuff = gRcvBuff;
	pb.csParam.create.rcvBuffLen = kRcvBuffer;
	err = Call(&pb, TCPCreate);
	if (err != noErr) {
		DisposePtr(gRcvBuff);
		gRcvBuff = NULL;
		return err;
	}
	gStream = (Ptr)pb.tcpStream;
	memset(&pb, 0, sizeof pb);
	pb.csParam.open.commandTimeoutValue = 20;
	pb.csParam.open.remoteHost = addr;
	pb.csParam.open.remotePort = port;
	err = Call(&pb, TCPActiveOpen);
	if (err != noErr)
		TcpClose();
	return err;
}

OSErr TcpSend(const void *data, unsigned short len)
{
	TCPiopb pb;
	WDS wds;

	if (!gStream)
		return -23008;     /* connectionDoesntExist */
	wds.e.length = len;
	wds.e.ptr = (Ptr)data;
	wds.end = 0;
	memset(&pb, 0, sizeof pb);
	pb.csParam.send.pushFlag = true;
	pb.csParam.send.wdsPtr = (Ptr)&wds;
	return Call(&pb, TCPSend);
}

long TcpRecv(void *buf, unsigned short max)
{
	TCPiopb pb;
	unsigned short n;
	unsigned char state;

	if (!gStream)
		return -1;
	memset(&pb, 0, sizeof pb);
	if (Call(&pb, TCPStatus) != noErr)
		return -1;
	n = pb.csParam.status.amtUnreadData;
	state = pb.csParam.status.connectionState;
	if (n == 0)
		return (state == 8 || state == 10 || state == 12) ? 0 : -1;   /* established, or still sending */
	if (n > max)
		n = max;
	memset(&pb, 0, sizeof pb);
	pb.csParam.receive.commandTimeoutValue = 2;
	pb.csParam.receive.rcvBuff = (Ptr)buf;
	pb.csParam.receive.rcvBuffLen = n;
	if (Call(&pb, TCPRcv) != noErr)
		return -1;
	return pb.csParam.receive.rcvBuffLen;
}

void TcpClose(void)
{
	TCPiopb pb;

	if (gStream) {
		memset(&pb, 0, sizeof pb);
		Call(&pb, TCPAbort);
		memset(&pb, 0, sizeof pb);
		Call(&pb, TCPRelease);
		gStream = NULL;
	}
	if (gRcvBuff) {
		DisposePtr(gRcvBuff);
		gRcvBuff = NULL;
	}
}

/* ---- the name resolver --------------------------------------------------------------------------- */

typedef struct {
	long rtnCode;
	char cname[255];
	char pad;
	unsigned long addr[4];
} HostInfo;

typedef OSErr (*DnrProc)(long selector, ...);

/* MacTCP's AddressXlation.h: cacheFault (-23042) means the name is not in the cache and the answer comes
   later through the result procedure; it is not an error. */
enum { kOpenResolver = 1, kStrToAddr = 3, kCacheFault = -23042 };

static Handle gDnr;
static volatile Boolean gDnrDone;

static pascal void DnrResult(HostInfo *info, Ptr userData)
{
	gDnrDone = true;
}

/* The resolver lives in the control panel of type 'cdev' and creator 'ztcp' (MacTCP, and Open Transport's
   TCP/IP) or 'mtcp', in the System Folder (System 6) or its Control Panels folder (System 7). */
static short OpenPanel(short vRef, long dirID)
{
	HParamBlockRec pb;
	Str63 name;
	short i;
	static const OSType creators[] = { 'ztcp', 'mtcp' };
	short c;

	for (c = 0; c < 2; c++) {
		for (i = 1; i < 500; i++) {
			memset(&pb, 0, sizeof pb);
			pb.fileParam.ioNamePtr = name;
			pb.fileParam.ioVRefNum = vRef;
			pb.fileParam.ioFDirIndex = i;
			pb.fileParam.ioDirID = dirID;
			if (PBHGetFInfoSync(&pb) != noErr)
				break;
			if (pb.fileParam.ioFlFndrInfo.fdType == 'cdev' && pb.fileParam.ioFlFndrInfo.fdCreator == creators[c])
				return dirID ? HOpenResFile(vRef, dirID, name, fsRdPerm) : OpenRFPerm(name, vRef, fsRdPerm);
		}
	}
	return -1;
}

static OSErr OpenResolver(void)
{
	SysEnvRec env;
	short ref = -1, cur = CurResFile();
	long attr;

	if (gDnr)
		return noErr;
	SysEnvirons(1, &env);
	ref = OpenPanel(env.sysVRefNum, 0);
	/* Gestalt came with System 6.0.4, FindFolder with System 7: ask only where the trap is there */
	if (ref == -1 && NGetTrapAddress(0xA1AD, kOSTrapType) != NGetTrapAddress(0xA89F, kToolboxTrapType)
	    && Gestalt(gestaltFindFolderAttr, &attr) == noErr && (attr & 1)) {
		short vRef;
		long dirID;
		if (FindFolder(kOnSystemDisk, kControlPanelFolderType, false, &vRef, &dirID) == noErr)
			ref = OpenPanel(vRef, dirID);
	}
	if (ref != -1)
		UseResFile(ref);
	gDnr = Get1IndResource('dnrp', 1);
	if (!gDnr && ref == -1)
		gDnr = GetIndResource('dnrp', 1);
	if (gDnr) {
		DetachResource(gDnr);
		MoveHHi(gDnr);
		HLock(gDnr);
	}
	if (ref != -1)
		CloseResFile(ref);
	UseResFile(cur);
	if (!gDnr)
		return resNotFound;
	return ((DnrProc)*gDnr)(kOpenResolver, (char *)NULL);
}

static Boolean ParseDotted(const unsigned char *s, unsigned long *addr)
{
	unsigned long a = 0, part = 0;
	short i, dots = 0, digits = 0;

	for (i = 1; i <= s[0]; i++) {
		if (s[i] >= '0' && s[i] <= '9') {
			part = part * 10 + (s[i] - '0');
			if (part > 255 || ++digits > 3)
				return false;
		} else if (s[i] == '.' && digits) {
			a = (a << 8) | part;
			part = digits = 0;
			dots++;
		} else
			return false;
	}
	if (dots != 3 || !digits)
		return false;
	*addr = (a << 8) | part;
	return true;
}

OSErr TcpResolve(const unsigned char *host, unsigned long *addr)
{
	static HostInfo info;
	char cname[256];
	OSErr err;
	unsigned long until;

	if (ParseDotted(host, addr))
		return noErr;
	if ((err = OpenResolver()) != noErr)
		return err;
	memcpy(cname, host + 1, host[0]);
	cname[host[0]] = 0;
	memset(&info, 0, sizeof info);
	gDnrDone = false;
	err = ((DnrProc)*gDnr)(kStrToAddr, cname, &info, DnrResult, (char *)NULL);
	if (err == kCacheFault) {
		until = TickCount() + 60 * 30;
		while (!gDnrDone && TickCount() < until)
			SystemTask();
		err = gDnrDone ? (OSErr)info.rtnCode : -23041;   /* nameSyntaxErr stands in for a timeout */
	}
	if (err == noErr)
		*addr = info.addr[0];
	return err;
}
