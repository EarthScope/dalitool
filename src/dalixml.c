/***************************************************************************
 * dalixml.c
 *
 * Routines to print formatted INFO XML responses.
 *
 * Written by:
 *   Chad Trabant, IRIS Data Management Center
 *
 * modified: 2013.212
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
prtinfo_status (ezxml_t xmldoc, int verbose, FILE *prtstream)
{
  ezxml_t status, serverthreads, thread;
  char *rootname = ezxml_name(xmldoc);
  const char *flags, *type;
  const char *port;
  const char *dir, *maxrecur, *statefile, *match, *reject;
  const char *scantime, *packetrate, *byterate;
  const char *totalthreads;
  char timestr[50];
  
  if ( strcmp (rootname, "DataLink") )
    {
      dl_log (1, 0, "XML INFO root tag is not <DataLink>, invalid data\n");
      return;
    }
  
  dl_dltime2mdtimestr (dlp_time(), timestr, 0);
  fprintf (prtstream, "Current time: %s UTC\n", timestr);
  
  fprintf (prtstream, "Server ID: %s (%s)\n",
	   ezxml_attr (xmldoc, "ServerID"),
	   ezxml_attr (xmldoc, "Version"));
  
  if ( verbose >= 1 )
    {
      fprintf (prtstream, "Capabilities: %s\n", ezxml_attr (xmldoc, "Capabilities"));
    }
  
  if ( (status = ezxml_child (xmldoc, "Status")) )
    {
      if ( verbose >= 1 )
	{
	  fprintf (prtstream, "Ring size: %s, Packet size: %s\n",
		   ezxml_attr (status, "RingSize"), ezxml_attr (status, "PacketSize"));
	  fprintf (prtstream, "Memory-mapped ring: %s, Volatile ring: %s\n",
		   ezxml_attr (status, "MemoryMappedRing"), ezxml_attr (status, "VolatileRing"));
	  fprintf (prtstream, "Max packet ID: %s, Max packets: %s\n",
		   ezxml_attr (status, "MaximumPacketID"), ezxml_attr (status, "MaximumPackets"));
	}
      
      fprintf (prtstream, "\n  Started: %s, %s connections, %s streams\n",
	       ezxml_attr (status, "StartTime"), ezxml_attr (status, "TotalConnections"),
	       ezxml_attr (status, "TotalStreams"));
      fprintf (prtstream, "  Input: %s packets/sec, %s bytes/sec\n",
	       ezxml_attr (status, "RXPacketRate"), ezxml_attr (status, "RXByteRate"));
      fprintf (prtstream, "  Output: %s packets/sec, %s bytes/sec\n",
	       ezxml_attr (status, "TXPacketRate"), ezxml_attr (status, "TXByteRate"));
      fprintf (prtstream, "  Earliest Packet: %s - %s (ID %s)\n",
	       ezxml_attr (status, "EarliestPacketDataStartTime"),
	       ezxml_attr (status, "EarliestPacketDataEndTime"),
	       ezxml_attr (status, "EarliestPacketID"));
      fprintf (prtstream, "  Latest Packet: %s - %s (ID %s)\n",
	       ezxml_attr (status, "LatestPacketDataStartTime"),
	       ezxml_attr (status, "LatestPacketDataEndTime"),
	       ezxml_attr (status, "LatestPacketID"));
    }
  
  if ( verbose >= 1 && (serverthreads = ezxml_child (xmldoc, "ServerThreads")) )
    {
      fprintf (prtstream, "\n");
      fprintf (prtstream, "  Server threads:\n");
      
      for (thread = ezxml_child (serverthreads, "Thread"); thread; thread = ezxml_next(thread))
	{
	  flags = ezxml_attr (thread, "Flags");
	  type = ezxml_attr (thread, "Type");
	  
	  /* Skip initial spaces in flags */
	  while ( *flags == ' ' ) flags++;
	  
	  fprintf (prtstream, "    %s [%s]\n", (type)?type:"-", (flags)?flags:"-");
	  
	  if ( type && (! strcmp (type, "DataLink Server") || ! strcmp (type, "SeedLink Server")) )
	    {
	      port = ezxml_attr (thread, "Port");
	      fprintf (prtstream, "      Port: %s\n", (port)?port:"-");
	    }
	  else if ( type && ! strcmp (type, "Mini-SEED Scanner") )
	    {
	      dir = ezxml_attr (thread, "Directory");
	      maxrecur = ezxml_attr (thread, "MaxRecursion");
	      statefile = ezxml_attr (thread, "StateFile");
	      match = ezxml_attr (thread, "Match");
	      reject = ezxml_attr (thread, "Reject");
	      scantime = ezxml_attr (thread, "ScanTime");
	      packetrate = ezxml_attr (thread, "PacketRate");
	      byterate = ezxml_attr (thread, "ByteRate");
	      
	      fprintf (prtstream, "      Directory '%s', MaxRecursion: %s, StateFile: '%s'\n",
		       (dir)?dir:"-", (maxrecur)?maxrecur:"-", (statefile)?statefile:"-");
	      fprintf (prtstream, "      Filename Match: %s, Reject: %s\n",
		       (match)?match:"-", (reject)?reject:"-");
	      fprintf (prtstream, "      Scanning: %s seconds, %s packets/sec, %s bytes/sec\n",
		       (scantime)?scantime:"-",(packetrate)?packetrate:"-", (byterate)?byterate:"-");
	    }
	}
      
      fprintf (prtstream, "\n");
      totalthreads = ezxml_attr (serverthreads, "TotalServerThreads");
      fprintf (prtstream, "  Total server threads: %s\n", (totalthreads)?totalthreads:"-");
    }
  
}  /* End of prtinfo_status() */


/***************************************************************************
 * prtinfo_connections():
 *
 * Format the specified XML document into a connection list.
 ***************************************************************************/
void
prtinfo_connections (ezxml_t xmldoc, int verbose, FILE *prtstream)
{
  ezxml_t status, connectionlist, connection;
  char *rootname = ezxml_name(xmldoc);
  const char *selectedcount, *totalcount;
  const char *type, *host, *ip, *port;
  const char *clientid, *conntime;
  const char *match, *reject;
  const char *pktid, *pktdatastart;
  const char *streamcount;
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
  fprintf (prtstream, "Current time: %s UTC\n", timestr);
  
  fprintf (prtstream, "Server ID: %s (%s)\n",
	   ezxml_attr (xmldoc, "ServerID"),
	   ezxml_attr (xmldoc, "Version"));

  if ( verbose >= 1 )
    {
      if ( (status = ezxml_child (xmldoc, "Status")) )
        {
	  fprintf (prtstream, "  Started: %s, %s connections, %s streams\n",
                   ezxml_attr (status, "StartTime"), ezxml_attr (status, "TotalConnections"),
                   ezxml_attr (status, "TotalStreams"));
	  fprintf (prtstream, "  Input: %s packets/sec, %s bytes/sec\n",
                   ezxml_attr (status, "RXPacketRate"), ezxml_attr (status, "RXByteRate"));
	  fprintf (prtstream, "  Output: %s packets/sec, %s bytes/sec\n",
                   ezxml_attr (status, "TXPacketRate"), ezxml_attr (status, "TXByteRate"));
        }
    }

  fprintf (prtstream, "\n");
  
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
      
      fprintf (prtstream, "%s [%s:%s]\n  [%s]  %s  %s\n",
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
      
      fprintf (prtstream, "  Packet %s (%s)  Lag %s%s, %s seconds\n",
	       (pktid)?pktid:"-",
	       (pktdatastart)?pktdatastart:"-",
	       (percentlag)?percentlag:"-", (*percentlag != '-')?"%":"",
	       (latency)?latency:"-");
      
      if ( verbose >= 1 )
	{
	  streamcount = ezxml_attr (connection, "StreamCount");
	  
	  txpackets = ezxml_attr (connection, "TXPacketCount");
	  txpacketrate = ezxml_attr (connection, "TXPacketRate");
	  txbytes = ezxml_attr (connection, "TXByteCount");
	  txbyterate = ezxml_attr (connection, "TXByteRate");
	  
	  rxpackets = ezxml_attr (connection, "RXPacketCount");
	  rxpacketrate = ezxml_attr (connection, "RXPacketRate");
	  rxbytes = ezxml_attr (connection, "RXByteCount");
	  rxbyterate = ezxml_attr (connection, "RXByteRate");
	  
	  fprintf (prtstream, "  TX %s packets %s packets/sec  %s bytes %s bytes/sec\n",
		   (txpackets)?txpackets:"-",
		   (txpacketrate)?txpacketrate:"-",
		   (txbytes)?txbytes:"-",
		   (txbyterate)?txbyterate:"-");
	  
	  fprintf (prtstream, "  RX %s packets %s packets/sec  %s bytes %s bytes/sec\n",
		   (rxpackets)?rxpackets:"-",
		   (rxpacketrate)?rxpacketrate:"-",
		   (rxbytes)?rxbytes:"-",
		   (rxbyterate)?rxbyterate:"-");
	  
	  fprintf (prtstream, "  Stream count: %s\n", (streamcount)?streamcount:"-");	  
	}
      
      if ( verbose >= 2 )
	{
	  match = ezxml_attr (connection, "Match");
	  reject = ezxml_attr (connection, "Reject");
	  
	  fprintf (prtstream, "  Match:  %.60s\n", (match)?match:"-");
	  fprintf (prtstream, "  Reject: %.60s\n", (reject)?reject:"-");
	}
      
      fprintf (prtstream, "\n");
    }
  
  totalcount = ezxml_attr (connectionlist, "TotalConnections");
  selectedcount = ezxml_attr (connectionlist, "SelectedConnections");
  fprintf (prtstream, "%s of %s connections\n", (selectedcount)?selectedcount:"-",
	   (totalcount)?totalcount:"-");
  
}  /* End of prtinfo_connections() */


/***************************************************************************
 * prtinfo_streams():
 * Format the specified XML document into a stream list.
 ***************************************************************************/
void
prtinfo_streams (ezxml_t xmldoc, int verbose, FILE *prtstream)
{
  ezxml_t streamlist, stream;
  char *rootname = ezxml_name(xmldoc);
  const char *selectedcount, *totalcount;
  const char *name;
  const char *earliestid, *earlieststart;
  const char *latestid, *lateststart;
  const char *latestend, *datalatency;
  char timestr[50];
  dltime_t now;
  
  now = dlp_time();
  
  if ( strcmp (rootname, "DataLink") )
    {
      dl_log (1, 0, "XML INFO root tag is not <DataLink>, invalid data\n");
      return;
    }
  
  dl_dltime2mdtimestr (dlp_time(), timestr, 0);
  fprintf (prtstream, "Current time: %s UTC\n", timestr);
  
  fprintf (prtstream, "Server ID: %s (%s)\n",
	   ezxml_attr (xmldoc, "ServerID"),
	   ezxml_attr (xmldoc, "Version"));
  
  if ( ! (streamlist = ezxml_child (xmldoc, "StreamList")) )
    {
      dl_log (1, 0, "Cannot find StreamList element in XML response\n");
      return;
    }
  
  if ( verbose >= 1 )
    fprintf (prtstream, "    Stream ID                     Earliest Packet                       Latest Packet               Latency\n");
  else
    fprintf (prtstream, "    Stream ID                Earliest Packet             Latest Packet           Latency\n");
  
  for (stream = ezxml_child (streamlist, "Stream"); stream; stream = ezxml_next(stream))
    {
      name = ezxml_attr (stream, "Name");
      earliestid = ezxml_attr (stream, "EarliestPacketID");
      earlieststart = ezxml_attr (stream, "EarliestPacketDataStartTime");
      latestid = ezxml_attr (stream, "LatestPacketID");
      lateststart = ezxml_attr (stream, "LatestPacketDataStartTime");
      latestend = ezxml_attr (stream, "LatestPacketDataEndTime");
      datalatency = ezxml_attr (stream, "DataLatency");
      
      if ( verbose >= 1 )
	fprintf (prtstream, "%-22s %-26s (%s)  %-26s (%s)  %s seconds\n",
		 (name)?name:"-",
		 (earlieststart)?earlieststart:"-",
		 (earliestid)?earliestid:"-",
		 (lateststart)?lateststart:"-",
		 (latestid)?latestid:"-",
		 (datalatency)?datalatency:"-");
      else
	fprintf (prtstream, "%-22s %-26s  %-26s  %s seconds\n",
		 (name)?name:"-",
		 (earlieststart)?earlieststart:"-",
		 (lateststart)?lateststart:"-",
		 (datalatency)?datalatency:"-");
    }
  
  totalcount = ezxml_attr (streamlist, "TotalStreams");
  selectedcount = ezxml_attr (streamlist, "SelectedStreams");
  fprintf (prtstream, "%s of %s streams\n", (selectedcount)?selectedcount:"-",
	   (totalcount)?totalcount:"-");
  
}  /* End of prtinfo_streams() */
