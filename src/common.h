
#ifndef COMMON_H
#define COMMON_H

#include <libdali.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern int packet_handler (DLPacket *dlpacket, void *packetdata,
			   int ppackets, int psamples, FILE *outfp);

extern int info_handler (char *infotype, char *infodata, int infolen,
			 int formatlevel);

#ifdef __cplusplus
}
#endif

#endif  /* COMMON_H */
