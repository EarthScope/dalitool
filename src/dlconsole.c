/***************************************************************************
 * console routines.
 *
 * Routines for supporting an interactive console for DataLink.
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <libmseed.h>
#include <libdali.h>

#include "linenoise.h"
#include "common.h"
#include "dlconsole.h"

static void completion (const char *buf, linenoiseCompletions *lc);
static int fetchinfo ( DLCP *dlconn, char *infotype,
		       int formatlevel, char *clientpattern );


/***************************************************************************
 * runconsole:
 *
 * Collect a command from the console, execute with the DataLink
 * server and process the return value.
 *
 * Commands recongnized:
 *  EXIT, QUIT, BYE
 *  CONNECT [host:port]
 *  ID
 *  PSET <packetid>
 *  PAFTER <datatime>
 *  MATCH [pattern]
 *  REJECT [pattern]
 *  STATUS [-v]
 *  STREAMS [-v]
 *  CONNECTIONS [pattern] [-v]
 *  READ <packetid>
 *  STREAM
 *
 * Return 0 on success and non-zero on error or exit request.
 ***************************************************************************/
int
runconsole (DLCP *dlconn, DLPacket *dlpacket, void *packetdata,
	    size_t maxdatasize, int verbose)
{
  static char cmd[256] = "";
  static int cmdlen = 0;
  char *line = NULL;
  int linelen = 0;
  char cbuf = '\0';
  
  char str1[256];
  char str2[256];
  char *strp[2] = {str1,str2};
  int strc;
  
  if ( ! dlpacket || ! dlconn )
    return -1;
  
  /* Set the completion callback (invoked for <Tab>) and max history */
  linenoiseSetCompletionCallback (completion);
  linenoiseHistorySetMaxLen (100);
  
  fprintf (stdout, "DataLink Console, type 'help' to list available commands\n");
  
  while ( (line = linenoise ("DL> ")) != NULL )
    {
      linelen = strlen (line);
      
      /* Store entered command, otherwise the previous command is repeated */
      if ( linelen > 0 )
	{
	  strncpy (cmd, line, sizeof(cmd));
	  cmdlen = linelen;
	}
      
      if ( cmdlen <= 0 )
	continue;
      
      free (line);
      
      /* Parse command options if specified */
      strc = sscanf (cmd, "%*s %s %s", str1, str2);
      
      /* Check for closed connection and re-connect if needed.
       * Half-open (silently broken) connections cannot be detected
       * until a command is issued. */
      if ( dlconn->link == -1 || recv (dlconn->link, &cbuf, 1, MSG_PEEK) == 0 )
	{
	  fprintf (stdout, "Re-connecting to server... ");
	  
	  /* Close/cleanup existing connection if needed */
	  if ( dlconn->link != -1 )
	    {
	      dl_disconnect (dlconn);
	    }
	  
	  /* Connect to server */
	  if ( dl_connect (dlconn) < 0 )
	    {
	      fprintf (stdout, "Error connecting to server\n");
	    }
	  else
	    {
	      fprintf (stdout, "Connected to %s\n", dlconn->addr);
	    }
	}
      
      /* Check for recognized commands and process */
      if ( ! strncasecmp (cmd, "EXIT", 4) ||
	   ! strncasecmp (cmd, "QUIT", 4) ||
	   ! strncasecmp (cmd, "BYE", 3) )
	{
	  fprintf (stdout, "Goodbye\n");
	  break;
	}
      else if ( ! strncasecmp (cmd, "HELP", 4) )
	{
	  fprintf (stdout, "The following commands are supported:\n");
	  fprintf (stdout, "EXIT,QUIT,BYE               Exit console mode\n");
	  fprintf (stdout, "CONNECT [host:port]         Connect to server, optionally using address\n");
	  fprintf (stdout, "ID                          Print ID of server\n");
	  fprintf (stdout, "PSET <packetID>             Set position to packet ID (or LATEST or EARLIEST)\n");
	  fprintf (stdout, "PAFTER <datatime>           Set position to packet after datatime\n");
	  fprintf (stdout, "MATCH [pattern]             Set MATCH expression to pattern, or unset if none\n");
	  fprintf (stdout, "REJECT [pattern]            Set REJECT expression to pattern, or unset if none\n");
	  fprintf (stdout, "STATUS [-v]                 Print server status\n");
	  fprintf (stdout, "STREAMS [-v]                Print server streams\n");
	  fprintf (stdout, "CONNECTIONS [pattern] [-v]  Print server connections\n");
	  fprintf (stdout, "READ <packetid>             Read and print packet details\n");
	  fprintf (stdout, "STREAM                      Collect packets in streaming mode\n");
	  fprintf (stdout, "\n");
	}  /* End of HELP */
      else if ( ! strncasecmp (cmd, "ID", 2) )
	{
	  char sendstr[255];      /* Buffer for command strings */
	  char respstr[255];      /* Buffer for server response */
	  int respsize;
	  
	  /* Send ID command including client ID and collect response */
	  snprintf (sendstr, sizeof(sendstr), "ID %s",
		    (dlconn->clientid) ? dlconn->clientid : "");
	  
	  respsize = dl_sendpacket (dlconn, sendstr, strlen (sendstr), NULL, 0,
				    respstr, sizeof(respstr));
	  
	  if ( respsize > 11 )
	    {
	      /* Make sure the response string is terminated */
	      respstr[respsize] = '\0';
	      
	      /* Print response without "ID " prefix */
	      fprintf (stdout, "%s\n", respstr+3);
	    }
	  else
	    {
	      if ( respsize > 0 )
		fprintf (stdout, "Unrecognized ID response from server: %255s\n", respstr);
	      else
		fprintf (stdout, "Error requesting ID from server\n");
	    }
	}  /* End of ID */
      else if ( ! strncasecmp (cmd, "PSET", 4) )
	{
	  int64_t pktid;
	  
	  if ( strc != 1 )
	    {
	      fprintf (stdout, "Unrecognized usage, try PSET <value>\n");
	      
	      continue;
	    }
	  else if ( ! strncasecmp (str1, "EARLIEST", 8) )
	    {
	      pktid = LIBDALI_POSITION_EARLIEST;
	    }
	  else if ( ! strncasecmp (str1, "LATEST", 6) )
	    {
	      pktid = LIBDALI_POSITION_LATEST;
	    }
	  else
	    {
	      pktid = strtoll (str1, NULL, 10);
	      
	      if ( pktid == 0 && errno == EINVAL )
		{
		  fprintf (stdout, "Unrecognized position value: %s\n", str1);
		  
		  continue;
		}
	    }
	  
	  pktid = dl_position (dlconn, pktid, HPTERROR);
	  
	  if ( pktid > 0 )
	    {
	      fprintf (stdout, "Positioned to packet ID %lld\n", (long long int)pktid);
	    }
	  else if ( pktid == 0 )
	    {
	      fprintf (stdout, "Packet %s not found\n", str1);
	    }
	  else
	    {
	      fprintf (stdout, "Error requesting position %s\n", str1);
	    }
	}  /* End of PSET */
      else if ( ! strncasecmp (cmd, "PAFTER", 4) )
	{
	  dltime_t datatime;
	  int64_t pktid;
	  
	  if ( strc != 1 )
	    {
	      fprintf (stdout, "Unrecognized usage, try PAFTER <datatime>\n");
	      
	      continue;
	    }
	  else
	    {
	      datatime = dl_timestr2dltime (str1);
	      
	      if ( datatime == DLTERROR )
		{
		  fprintf (stdout, "Unrecognized data time: %s\n", str1);
		  
		  continue;
		}
	    }
	  
	  pktid = dl_position_after (dlconn, datatime);
	  
	  if ( pktid > 0 )
	    {
	      fprintf (stdout, "Positioned to packet ID %lld\n", (long long int)pktid);
	    }
	  else if ( pktid == 0 ) // This may not be true
	    {
	      fprintf (stdout, "Packet %s not found\n", str1);
	    }
	  else
	    {
	      fprintf (stdout, "Error requesting position %s\n", str1);
	    }
	}  /* End of PAFTER */
      else if ( ! strncasecmp (cmd, "MATCH", 5) )
	{
	  int64_t count;
	  
	  count = dl_match (dlconn, (strc>0) ? str1 : NULL);
	  
	  if ( count >= 0 )
	    {
	      fprintf (stdout, "Matched %lld streams\n", (long long int)count);
	    }
	  else
	    {
	      fprintf (stdout, "Error submitting match expression %s\n",
		       (strc>0) ? str1 : "<NONE>");
	    }
	}  /* End of MATCH */
      else if ( ! strncasecmp (cmd, "REJECT", 5) )
	{
	  int64_t count;
	  
	  count = dl_reject (dlconn, (strc>0) ? str1 : NULL);
	  
	  if ( count >= 0 )
	    {
	      fprintf (stdout, "Rejected %lld streams\n", (long long int)count);
	    }
	  else
	    {
	      fprintf (stdout, "Error submitting reject expression %s\n",
		       (strc>0) ? str1 : "<NONE>");
	    }
	}  /* End of REJECT */
      else if ( ! strncasecmp (cmd, "STATUS", 6) )
	{
	  int formatlevel = 0;
	  
	  if ( strc > 0 )
	    {
	      if ( strncmp (str1, "-v", 2) == 0 )
		{
		  formatlevel += strspn (str1+1, "v");
		}
	    }
	  
	  if ( fetchinfo (dlconn, "STATUS", formatlevel, NULL) )
	    {
	      fprintf (stdout, "Error requesting STATUS\n");
	    }
	}  /* End of STATUS */
      else if ( ! strncasecmp (cmd, "STREAMS", 7) )
	{
	  int formatlevel = 0;
	  
	  if ( strc > 0 )
	    {
	      if ( strncmp (str1, "-v", 2) == 0 )
		{
		  formatlevel += strspn (str1+1, "v");
		}
	    }
	  
	  if ( fetchinfo (dlconn, "STREAMS", formatlevel, NULL) )
	    {
	      fprintf (stdout, "Error requesting STREAMS\n");
	    }
	}  /* End of STREAMS */
      else if ( ! strncasecmp (cmd, "CONNECTIONS", 11) )
	{
	  int formatlevel = 0;
	  char *matchptr = NULL;
	  int si;
	  
	  for ( si = 0; si < strc; si++ )
	    {
	      if ( strncmp (strp[si], "-v", 2) == 0 )
		{
		  formatlevel += strspn (strp[si]+1, "v");
		}
	      else
		{
		  matchptr = strp[si];
		}
	    }
	  
	  if ( fetchinfo (dlconn, "CONNECTIONS", formatlevel, matchptr) )
	    {
	      fprintf (stdout, "Error requesting CONNECTIONS\n");
	    }
	}  /* End of CONNECTIONS */
      else if ( ! strncasecmp (cmd, "READ", 4) )
	{
	  int64_t pktid;
	  int datasize;
	  
	  if ( strc != 1 )
	    {
	      fprintf (stdout, "Unrecognized usage, try READ <packetID>\n");
	      
	      continue;
	    }
	  else
	    {
	      pktid = strtoll (str1, NULL, 10);
	      
	      if ( pktid == 0 && errno == EINVAL )
		{
		  fprintf (stdout, "Unrecognized packet value: %s\n", str1);
		  
		  continue;
		}
	    }
	  
	  datasize = dl_read (dlconn, pktid, dlpacket, packetdata, maxdatasize);
	  
	  if ( datasize > 0 )
	    {
	      packet_handler (dlpacket, packetdata, 0, 0, NULL);
	    }
	  else if ( datasize == 0 )
	    {
	      fprintf (stdout, "Packet data too large for buffer\n");
	    }
	}  /* End of READ */
      else if ( ! strncasecmp (cmd, "STREAM", 6) )
	{
	  fd_set readset;          /* File descriptor set for select() */
	  struct timeval timeout;  /* Timeout throttle for select() */
	  int endflag = 0;
	  int rv;
	  
	  timeout.tv_sec = 0;

	  fprintf (stdout, "Streaming from current position (press Enter to stop)\n");

	  for (;;)
	    {
	      timeout.tv_usec = 0;
	      
	      /* Check for packet from DataLink server */
	      rv = dl_collect_nb (dlconn, dlpacket, packetdata, maxdatasize, endflag);
	      
	      /* Forward any recieved packet to the client */
	      if ( rv == DLPACKET )
		{
		  packet_handler (dlpacket, packetdata, 0, 0, NULL);
		}
	      else if ( rv == DLNOPACKET )
		{
		  /* Timeout 1/10 second to throttle the loop */
		  timeout.tv_usec = 100000;
		}
	      /* Otherwise the connection was shutdown or an error occurred */
	      else
		{
		  break;
		}
	      
	      /* Check for input on stdin, if any input is available break out of the loop.
	       * Use select timeout to throttle connection when no packets received. */
	      
	      FD_ZERO(&readset);
	      FD_SET(fileno(stdin), &readset);
	      
	      rv = select (fileno(stdin)+1, &readset, NULL, NULL, &timeout);
	      
	      if ( rv == -1 )
		{
		  if ( errno != EINTR )
		    fprintf (stdout, "Error with select: %s\n", strerror(errno));
		  
		  break;
		}
	      
	      /* If input is available on stdin, read/ignore a single character and set end flag */
	      if ( rv > 0 && FD_ISSET(fileno(stdin), &readset) )
		{
		  getc(stdin);
		  endflag = 1;
		}
	    } /* End of STREAM collection loop */
	}  /* End of STREAM */
      else if ( ! strncasecmp (cmd, "CONNECT", 7) )
	{
	  /* Use server address if specified */
	  if ( strc >= 1 )
	    {
	      strncpy (dlconn->addr, str1, sizeof(dlconn->addr));
	    }
	  
	  /* Close existing connection if needed */
	  if ( dlconn->link != -1 )
	    {
	      dl_disconnect (dlconn);
	    }
	  
	  /* Connect to server */
	  if ( dl_connect (dlconn) < 0 )
	    {
	      fprintf (stdout, "Error connecting to server\n");
	    }
	  else
	    {
	      fprintf (stdout, "Connected to %s\n", dlconn->addr);
	    }
	}  /* End of CONNECT */
      else if ( cmdlen > 0 )
	{
	  fprintf (stdout, "Unrecognized command: %s\n", cmd);
	  continue;
	}
      
      /* Add recognized command to history if not a repeat line */
      if ( linelen > 0 )
	linenoiseHistoryAdd(cmd);
      
      /* Print current time stamp if verbose */
      if ( verbose )
	{
	  time_t now;
	  struct tm *tp;
	  
	  now = time(NULL);
	  tp = localtime (&now);
	  fprintf (stdout, "%04d-%02d-%02d %02d:%02d:%02d\n",
		   tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday,
		   tp->tm_hour, tp->tm_min, tp->tm_sec);
	}
    }
  
  return 0;
}  /* End of consolecmd() */


/***************************************************************************
 * completion:
 *
 * Command completion callback for linenoise, invoked when user preses <Tab>
 *
 ***************************************************************************/
static void
completion (const char *buf, linenoiseCompletions *lc)
{
  if (buf[0] == 'e') {
    linenoiseAddCompletion(lc,"exit");
  }
  else if (buf[0] == 'q') {
    linenoiseAddCompletion(lc,"quit");
  }
  else if (buf[0] == 'b') {
    linenoiseAddCompletion(lc,"bye");
  }
  else if (buf[0] == 'p') {
    linenoiseAddCompletion(lc,"pset ");
    linenoiseAddCompletion(lc,"pafter ");
  }
  else if (buf[0] == 'm') {
    linenoiseAddCompletion(lc,"match ");
  }
  else if (buf[0] == 'r') {
    linenoiseAddCompletion(lc,"reject ");
    linenoiseAddCompletion(lc,"read ");
  }
  else if (buf[0] == 's') {
    linenoiseAddCompletion(lc,"status");
    linenoiseAddCompletion(lc,"streams");
    linenoiseAddCompletion(lc,"stream");
  }
  else if (buf[0] == 'c') {
    linenoiseAddCompletion(lc,"connections");
    linenoiseAddCompletion(lc,"connect");
  }
}


/***************************************************************************
 * fetchinfo:
 *
 * Submit INFO command and print formatted response.
 *
 * Return 0 on success and non-zero on error.
 ***************************************************************************/
static int
fetchinfo ( DLCP *dlconn, char *infotype, int formatlevel, char *clientpattern )
{
  char *infodata = NULL;
  int infolen = 0;
  int rv;
  
  if ( ! dlconn || ! infotype )
    return -1;
  
  if ( (infolen = dl_getinfo (dlconn, infotype, clientpattern, &infodata, 0)) < 0 )
    return -1;
  
  rv = info_handler (infotype, infodata, infolen, formatlevel);
  
  if ( infodata )
    free (infodata);
  
  return rv;
}  /* End of fetchinfo() */
