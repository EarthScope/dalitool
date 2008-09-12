/***************************************************************************
 * dalixml.c
 *
 * Routines to print formatted INFO XML responses.
 *
 * Written by:
 *   Chad Trabant, IRIS Data Management Center
 *
 * modified: 2008.256
 ***************************************************************************/

#include <stdio.h>
#include <string.h>

#include <libdali.h>

#include "dalixml.h"


/***************************************************************************
 * prtinfo_status():
 *
 * Format the specified XML document into an identification summary.
 ***************************************************************************/
void
prtinfo_status (ezxml_t xmldoc, int verbose)
{
  ezxml_t status;
  char *rootname = ezxml_name(xmldoc);
  char timestr[50];
  
  if ( strcmp (rootname, "DataLink") )
    {
      dl_log (1, 0, "XML INFO root tag is not <DataLink>, invalid data\n");
      return;
    }
  
  dl_dltime2mdtimestr (dlp_time(), timestr, 0);
  printf ("Current time: %s UTC\n", timestr);
  
  printf ("Server ID: %s (%s)\n",
	  ezxml_attr (xmldoc, "ServerID"),
	  ezxml_attr (xmldoc, "Version"));
  
  if ( verbose >= 1 )
    {
      printf ("Capabilities: %s\n", ezxml_attr (xmldoc, "Capabilities"));
    }
  
  if ( (status = ezxml_child (xmldoc, "Status")) )
    {
      if ( verbose >= 1 )
	{
	  printf ("Ring size: %s, Packet size: %s\n",
		  ezxml_attr (status, "RingSize"), ezxml_attr (status, "PacketSize"));
	  printf ("Memory-mapped ring: %s, Volatile ring: %s\n",
		  ezxml_attr (status, "MemoryMappedRing"), ezxml_attr (status, "VolatileRing"));
	  printf ("Max packet ID: %s, Max packets: %s\n",
		  ezxml_attr (status, "MaximumPacketID"), ezxml_attr (status, "MaximumPackets"));
	}
      
      printf ("\n  Started: %s, %s connections, %s streams\n",
	      ezxml_attr (status, "StartTime"), ezxml_attr (status, "TotalConnections"),
	      ezxml_attr (status, "TotalStreams"));
      printf ("  Input: %s packets/sec, %s bytes/sec\n",
	      ezxml_attr (status, "RXPacketRate"), ezxml_attr (status, "RXByteRate"));
      printf ("  Output: %s packets/sec, %s bytes/sec\n",
	      ezxml_attr (status, "TXPacketRate"), ezxml_attr (status, "TXByteRate"));
      printf ("  Earliest Packet: %s - %s (%s)\n",
	      ezxml_attr (status, "EarliestPacketDataStartTime"),
	      ezxml_attr (status, "EarliestPacketDataEndTime"),
	      ezxml_attr (status, "EarliestPacketID"));
      printf ("  Latest Packet: %s - %s (%s)\n",
	      ezxml_attr (status, "LatestPacketDataStartTime"),
	      ezxml_attr (status, "LatestPacketDataEndTime"),
	      ezxml_attr (status, "LatestPacketID"));
    }
  
}  /* End of prtinfo_status() */


/***************************************************************************
 * prtinfo_connections():
 *
 * Format the specified XML document into a connection list.
 ***************************************************************************/
void
prtinfo_connections (ezxml_t xmldoc, int verbose)
{
  ezxml_t status, connectionlist, connection;
  char *rootname = ezxml_name(xmldoc);
  const char *selectedcount, *totalcount;
  const char *type, *host, *ip, *port;
  const char *clientid, *conntime;
  const char *match, *reject;
  const char *pktid, *pktdatastart;
  const char *txpackets, *txpacketrate, *txbytes, *txbyterate;
  const char *rxpackets, *rxpacketrate, *rxbytes, *rxbyterate;
  const char *latency, *percentlag;
  char timestr[50];
  dltime_t now;
  
  now = dlp_time();
  
  if ( strcmp (rootname, "DataLink") )
    {
      dl_log (1, 0, "XML INFO root tag is not <DataLink>, invalid data\n");
      return;
    }
  
  dl_dltime2mdtimestr (dlp_time(), timestr, 0);
  printf ("Current time: %s UTC\n", timestr);
  
  printf ("Server ID: %s (%s)\n",
	  ezxml_attr (xmldoc, "ServerID"),
	  ezxml_attr (xmldoc, "Version"));

  if ( verbose >= 1 )
    {
      if ( (status = ezxml_child (xmldoc, "Status")) )
        {
           printf ("  Started: %s, %s connections, %s streams\n",
                   ezxml_attr (status, "StartTime"), ezxml_attr (status, "TotalConnections"),
                   ezxml_attr (status, "TotalStreams"));
           printf ("  Input: %s packets/sec, %s bytes/sec\n",
                   ezxml_attr (status, "RXPacketRate"), ezxml_attr (status, "RXByteRate"));
           printf ("  Output: %s packets/sec, %s bytes/sec\n",
                   ezxml_attr (status, "TXPacketRate"), ezxml_attr (status, "TXByteRate"));
        }
    }

  printf ("\n");
  
  if ( ! (connectionlist = ezxml_child (xmldoc, "ConnectionList")) )
    {
      dl_log (1, 0, "Cannot find StreamList element in XML response\n");
      return;
    }
  
  for (connection = ezxml_child (connectionlist, "Connection"); connection; connection = ezxml_next(connection))
    {
      type = ezxml_attr (connection, "Type");
      host = ezxml_attr (connection, "Host");
      ip = ezxml_attr (connection, "IP");
      port = ezxml_attr (connection, "Port");
      clientid = ezxml_attr (connection, "ClientID");
      conntime = ezxml_attr (connection, "ConnectionTime");
      
      printf ("%s [%s:%s]\n  [%s]  %s  %s\n",
	      (host)?host:"-",
	      (ip)?ip:"-",
	      (port)?port:"-",
	      (type)?type:"-",
	      (clientid)?clientid:"-",
	      (conntime)?conntime:"-");
      
      pktid = ezxml_attr (connection, "PacketID");
      pktdatastart = ezxml_attr (connection, "PacketDataStartTime");
      percentlag = ezxml_attr (connection, "PercentLag");
      latency = ezxml_attr (connection, "Latency");
      
      printf ("  Packet %s (%s)  Lag %s%s, %s seconds\n",
	      (pktid)?pktid:"-",
	      (pktdatastart)?pktdatastart:"-",
	      (percentlag)?percentlag:"-", (*percentlag != '-')?"%":"",
	      (latency)?latency:"-");
      
      if ( verbose >= 1 )
	{
	  txpackets = ezxml_attr (connection, "TXPacketCount");
	  txpacketrate = ezxml_attr (connection, "TXPacketRate");
	  txbytes = ezxml_attr (connection, "TXByteCount");
	  txbyterate = ezxml_attr (connection, "TXByteRate");
	  
	  rxpackets = ezxml_attr (connection, "RXPacketCount");
	  rxpacketrate = ezxml_attr (connection, "RXPacketRate");
	  rxbytes = ezxml_attr (connection, "RXByteCount");
	  rxbyterate = ezxml_attr (connection, "RXByteRate");
	  
	  printf ("  TX %s packets %s packets/sec  %s bytes %s bytes/sec\n",
		  (txpackets)?txpackets:"-",
		  (txpacketrate)?txpacketrate:"-",
		  (txbytes)?txbytes:"-",
		  (txbyterate)?txbyterate:"-");
	  
	  printf ("  RX %s packets %s packets/sec  %s bytes %s bytes/sec\n",
		  (rxpackets)?rxpackets:"-",
		  (rxpacketrate)?rxpacketrate:"-",
		  (rxbytes)?rxbytes:"-",
		  (rxbyterate)?rxbyterate:"-");
	}
      
      if ( verbose >= 2 )
	{
	  match = ezxml_attr (connection, "Match");
	  reject = ezxml_attr (connection, "Reject");
	  
	  printf ("  Match:  %.60s\n", (match)?match:"-");
	  printf ("  Reject: %.60s\n", (reject)?reject:"-");
	}
      
      printf ("\n");
    }
  
  totalcount = ezxml_attr (connectionlist, "TotalConnections");
  selectedcount = ezxml_attr (connectionlist, "SelectedConnections");
  printf ("%s of %s connections\n", (selectedcount)?selectedcount:"-",
	  (totalcount)?totalcount:"-");
  
}  /* End of prtinfo_connections() */


/***************************************************************************
 * prtinfo_streams():
 * Format the specified XML document into a stream list.
 ***************************************************************************/
void
prtinfo_streams (ezxml_t xmldoc, int verbose)
{
  ezxml_t streamlist, stream;
  char *rootname = ezxml_name(xmldoc);
  const char *selectedcount, *totalcount;
  const char *name;
  const char *earliestid, *earlieststart;
  const char *latestid, *lateststart;
  const char *latestend;
  char timestr[50];
  char latencystr[50];
  dltime_t now;
  dltime_t endtime;
  
  now = dlp_time();
  
  if ( strcmp (rootname, "DataLink") )
    {
      dl_log (1, 0, "XML INFO root tag is not <DataLink>, invalid data\n");
      return;
    }
  
  dl_dltime2mdtimestr (dlp_time(), timestr, 0);
  printf ("Current time: %s UTC\n", timestr);
  
  printf ("Server ID: %s (%s)\n",
	  ezxml_attr (xmldoc, "ServerID"),
	  ezxml_attr (xmldoc, "Version"));
  
  if ( ! (streamlist = ezxml_child (xmldoc, "StreamList")) )
    {
      dl_log (1, 0, "Cannot find StreamList element in XML response\n");
      return;
    }
  
  if ( verbose >= 1 )
    printf ("    Stream ID                     Earliest Packet                       Latest Packet               Latency\n");
  else
    printf ("    Stream ID                Earliest Packet             Latest Packet           Latency\n");
  
  for (stream = ezxml_child (streamlist, "Stream"); stream; stream = ezxml_next(stream))
    {
      name = ezxml_attr (stream, "Name");
      earliestid = ezxml_attr (stream, "EarliestPacketID");
      earlieststart = ezxml_attr (stream, "EarliestPacketDataStartTime");
      latestid = ezxml_attr (stream, "LatestPacketID");
      lateststart = ezxml_attr (stream, "LatestPacketDataStartTime");
      latestend = ezxml_attr (stream, "LatestPacketDataEndTime");
      
      if ( latestend )
	{
	  endtime = dl_timestr2dltime ((char *)latestend);
	  snprintf (latencystr, sizeof(latencystr), "%.1f", (double)(now - endtime) / DLTMODULUS);
	}
      else
	{
	  strncpy (latencystr, "-", sizeof(latencystr));
	}
      
      if ( verbose >= 1 )
	printf ("%-22s %-26s (%s)  %-26s (%s)  %s seconds\n",
		(name)?name:"-",
		(earlieststart)?earlieststart:"-",
		(earliestid)?earliestid:"-",
		(lateststart)?lateststart:"-",
		(latestid)?latestid:"-",
		latencystr);
      else
	printf ("%-22s %-26s  %-26s  %s seconds\n",
		(name)?name:"-",
		(earlieststart)?earlieststart:"-",
		(lateststart)?lateststart:"-",
		latencystr);
    }
  
  totalcount = ezxml_attr (streamlist, "TotalStreams");
  selectedcount = ezxml_attr (streamlist, "SelectedStreams");
  printf ("%s of %s streams\n", (selectedcount)?selectedcount:"-",
	  (totalcount)?totalcount:"-");
  
}  /* End of prtinfo_streams() */
