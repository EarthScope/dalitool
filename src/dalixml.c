/***************************************************************************
 * dalixml.c
 *
 * Routines to print formatted INFO XML responses.
 *
 * Written by:
 *   Chad Trabant, IRIS Data Management Center
 *
 * modified: 2008.080
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
      printf ("Capabilities: %s\n",
	      ezxml_attr (xmldoc, "Capabilities"));      
    }
  
  if ( (status = ezxml_child (xmldoc, "Status")) )
    {
      printf ("\n  Started: %s, %s connections, %s streams\n",
	      ezxml_attr (status, "StartTime"), ezxml_attr (status, "TotalConnections"),
	      ezxml_attr (status, "TotalStreams"));
      printf ("  Input: %s packets/sec, %s bytes/sec\n",
	      ezxml_attr (status, "RXPacketRate"), ezxml_attr (status, "RXByteRate"));
      printf ("  Output: %s packets/sec, %s bytes/sec\n",
	      ezxml_attr (status, "TXPacketRate"), ezxml_attr (status, "TXByteRate"));
      printf ("  Earliest Packet: %s (%s)\n",
	      ezxml_attr (status, "EarliestPacketDataTime"), ezxml_attr (status, "EarliestPacketID"));
      printf ("  Latest Packet: %s (%s)\n",
	      ezxml_attr (status, "LatestPacketDataTime"), ezxml_attr (status, "LatestPacketID"));
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
  const char *type, *host, *port;
  const char *clientid, *conntime;
  const char *match, *reject;
  const char *pktid, *pktdatatime;
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
      port = ezxml_attr (connection, "Port");
      clientid = ezxml_attr (connection, "ClientID");
      conntime = ezxml_attr (connection, "ConnectionTime");
      
      printf ("%s:%s (%s)  %s  %s\n",
	      (host)?host:"-",
	      (port)?port:"-",
	      (type)?type:"-",
	      (clientid)?clientid:"-",
	      (conntime)?conntime:"-");
      
      pktid = ezxml_attr (connection, "PacketID");
      pktdatatime = ezxml_attr (connection, "PacketDataTime");
      percentlag = ezxml_attr (connection, "PercentLag");
      latency = ezxml_attr (connection, "Latency");
      
      printf ("  Packet %s (%s)  Lag %s%s, %s seconds\n",
	      (pktid)?pktid:"-",
	      (pktdatatime)?pktdatatime:"-",
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
  const char *earliestid, *earliesttime;
  const char *latestid, *latesttime;
  char timestr[50];
  char latencystr[50];
  dltime_t now;
  dltime_t datatime;
  
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
      earliesttime = ezxml_attr (stream, "EarliestPacketDataTime");
      latestid = ezxml_attr (stream, "LatestPacketID");
      latesttime = ezxml_attr (stream, "LatestPacketDataTime");
      
      if ( latesttime )
	{
	  datatime = dl_timestr2dltime ((char *)latesttime);
	  snprintf (latencystr, sizeof(latencystr), "%.1f", (double)(now - datatime) / DLTMODULUS);
	}
      else
	{
	  strncpy (latencystr, "-", sizeof(latencystr));
	}
      
      if ( verbose >= 1 )
	printf ("%-22s %-26s (%s)  %-26s (%s)  %s seconds\n",
		(name)?name:"-",
		(earliesttime)?earliesttime:"-",
		(earliestid)?earliestid:"-",
		(latesttime)?latesttime:"-",
		(latestid)?latestid:"-",
		latencystr);
      else
	printf ("%-22s %-26s  %-26s  %s seconds\n",
		(name)?name:"-",
		(earliesttime)?earliesttime:"-",
		(latesttime)?latesttime:"-",
		latencystr);
    }
  
  totalcount = ezxml_attr (streamlist, "TotalStreams");
  selectedcount = ezxml_attr (streamlist, "SelectedStreams");
  printf ("%s of %s streams\n", (selectedcount)?selectedcount:"-",
	  (totalcount)?totalcount:"-");
  
}  /* End of prtinfo_streams() */
