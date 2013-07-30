
#ifndef DLCONSOLE_H
#define DLCONSOLE_H

#include <libdali.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern int consolecmd (DLCP *dlconn, DLPacket *dlpacket, void *packetdata);

#ifdef __cplusplus
}
#endif

#endif  /* DLCONSOLE_H */
