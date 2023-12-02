/***************************************************************************
 * common.c
 *
 * Common routines.
 ***************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libdali.h>
#include <libmseed.h>

#include "common.h"
#include "dalixml.h"

static void msr_print_samples (MS3Record *msr, int psamples);

/***************************************************************************
 * packet_handler:
 *
 * Process a received packet based on packet type.  Print simple
 * packet parameters and optionally more details.
 *
 * If printdetail is 1 packet-specific details will be printed, if
 * printdetails is greater than 1 more details will be printed
 * (e.g. sample values for MSEED).
 *
 * If outfp is not NULL, the packet data will be written to the
 * specified FILE stream.
 *
 * Returns 0 on success and non-zero on error.
 ***************************************************************************/
int
packet_handler (DLPacket *dlpacket, void *packetdata,
                int ppackets, int psamples, FILE *outfp)
{
  dltime_t now;
  char timestr[60];
  char type[10];
  int rv;

  now = dlp_time ();

  /* Print basic packet details */
  dl_dltime2seedtimestr (dlpacket->datastart, timestr, 1);
  dl_log (0, 0, "%s (%lld), %s, %d (data: %.1f sec, feed: %.1f sec)\n",
          dlpacket->streamid, dlpacket->pktid, timestr, dlpacket->datasize,
          ((double)(now - dlpacket->dataend) / DLTMODULUS),
          ((double)(now - dlpacket->pkttime) / DLTMODULUS));

  /* Print packet and sample details if requested */
  if (ppackets > 0)
  {
    /* Parse packet type from stream ID */
    if (dl_splitstreamid (dlpacket->streamid, NULL, NULL, NULL, NULL, type))
    {
      dl_log (2, 0, "Error parsing stream ID: %s\n", dlpacket->streamid);
      return -1;
    }

    /* Handle MSEED packet types */
    if (!strncmp (type, "MSEED", sizeof (type)))
    {
      MS3Record *msr = 0;

      rv = msr3_parse (packetdata, dlpacket->datasize, &msr, MSF_UNPACKDATA, psamples);

      if (rv != MS_NOERROR)
      {
        dl_log (2, 0, "Cannot parse Mini-SEED record: %s\n", ms_errorstr (rv));
      }
      else
      {
        /* Print more packet details */
        msr3_print (msr, ppackets - 1);

        if (psamples)
          msr_print_samples (msr, psamples);
      }

      msr3_free (&msr);
    }
    /* This would be the place to add support for other packet types */
    /* Otherwise this is an unrecognized packet type */
    else
    {
      dl_log (2, 0, "Unrecognized packet type: %s\n", type);
    }
  }

  /* Write packet to dumpfile if defined */
  if (outfp)
  {
    if (fwrite (packetdata, dlpacket->datasize, 1, outfp) == 0)
      dl_log (2, 0, "fwrite(): error writing packet data to output file\n");
  }

  return 0;
} /* End of packet_handler() */

/***************************************************************************
 * info_handler:
 *
 * Print formatted INFO XML data.
 *
 * Returns 0 on success and -1 on errors.
 ***************************************************************************/
int
info_handler (char *infotype, char *infodata, int infolen, int formatlevel)
{
  ezxml_t xmldoc;

  /* Parse XML string into EzXML representation */
  if ((xmldoc = ezxml_parse_str (infodata, infolen)) == NULL)
  {
    dl_log (2, 0, "info_handler(): XML parse error\n");

    return -1;
  }

  /* Print formatted information */
  if (!strncasecmp (infotype, "STATUS", 6))
  {
    prtinfo_status (xmldoc, formatlevel, stdout);
  }
  else if (!strncasecmp (infotype, "CONNECTIONS", 11))
  {
    prtinfo_connections (xmldoc, formatlevel, stdout);
  }
  else if (!strncasecmp (infotype, "STREAMS", 7))
  {
    prtinfo_streams (xmldoc, formatlevel, stdout);
  }
  else
  {
    dl_log (2, 0, "info_handler(): unrecognized INFO type: %s\n", infotype);
  }

  ezxml_free (xmldoc);

  return 0;
} /* End of info_handler() */

/***************************************************************************
 * msr_print_samples:
 *
 * Print samples in the MSRecord with a simple format.
 ***************************************************************************/
static void
msr_print_samples (MS3Record *msr, int psamples)
{
  int line, col, cnt, samplesize;
  int lines = (msr->numsamples / 6) + 1;
  void *sptr;

  if ((samplesize = ms_samplesize (msr->sampletype)) == 0)
  {
    dl_log (2, 0, "Unrecognized sample type: %c\n", msr->sampletype);
  }

  if (msr->sampletype == 'a')
    dl_log (0, 0, "ASCII Data:\n%.*s\n", (int)msr->numsamples, (char *)msr->datasamples);
  else
    for (cnt = 0, line = 0; line < lines; line++)
    {
      for (col = 0; col < 6; col++)
      {
        if (cnt < msr->numsamples)
        {
          sptr = (char *)msr->datasamples + (cnt * samplesize);

          if (msr->sampletype == 'i')
            dl_log (0, 0, "%10d  ", *(int32_t *)sptr);

          else if (msr->sampletype == 'f')
            dl_log (0, 0, "%10.8g  ", *(float *)sptr);

          else if (msr->sampletype == 'd')
            dl_log (0, 0, "%10.10g  ", *(double *)sptr);

          cnt++;
        }
      }
      dl_log (0, 0, "\n");

      /* If only printing the first 6 samples break out here */
      if (psamples == 1)
        break;
    }

  return;
} /* End of msr_print_samples() */
