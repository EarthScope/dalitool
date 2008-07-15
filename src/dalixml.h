
#ifndef DALIXML_H
#define DALIXML_H

#include <ezxml.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern void prtinfo_status (ezxml_t xmldoc, int verbose);
extern void prtinfo_connections (ezxml_t xmldoc, int verbose);
extern void prtinfo_streams (ezxml_t xmldoc, int verbose);

#ifdef __cplusplus
}
#endif

#endif  /* DALIXML_H */
