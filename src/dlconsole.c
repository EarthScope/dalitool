/***************************************************************************
 * console routines.
 *
 * Routine for supporting an interactive console for DataLink.
 ***************************************************************************/

#include <errno.h>
#include <libmseed.h>
#include <libdali.h>

#include "linenoise.h"
#include "dlconsole.h"
#include "dalixml.h"

static int handlinfo ( DLCP *dlconn, char *infotype, int formatlevel,
		       char *clientpattern, FILE *outstream );


/***************************************************************************
 * runconsole:
 *
 * Collect a command from the console, execute with the DataLink
 * server and process the return value.
 *
 * Commands recongnized:
 *  EXIT, QUIT, BYE
 *  ID
 *  PSET <id>
 *  MATCH [pattern]
 *  REJECT [pattern]
 *  STATUS [-v]
 *  STREAMS [-v]
 *
 * Return 0 on success and non-zero on error or exit request.
 ***************************************************************************/
int
runconsole (DLCP *dlconn, DLPacket *dlpacket, void *packetdata, int verbose)
{
  static char cmd[256] = "";
  char *line = NULL;
  int cmdlen = 0;
  int linelen = 0;
  
  char svalue[256];
  long long int llvalue;
  int rv;
  
  FILE *outstream = stdout;
  
  if ( ! dlpacket || ! dlconn )
    return -1;
  
  linenoiseHistorySetMaxLen (100);
  
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
      
      /* Check for recognized commands and process */
      if ( ! strncasecmp (cmd, "EXIT", 4) ||
	   ! strncasecmp (cmd, "QUIT", 4) ||
	   ! strncasecmp (cmd, "BYE", 3) )
	{
	  fprintf (outstream, "Goodbye\n");
	  break;
	}
      else if ( ! strncasecmp (cmd, "HELP", 4) )
	{
	  fprintf (outstream, "The following commands are supported:\n");
	  fprintf (outstream, "EXIT,QUIT,BYE        Exit console mode\n");
	  fprintf (outstream, "ID                   Print ID of server\n");
	  fprintf (outstream, "PSET <packetID>      Issue POSITION SET command to position reader\n");
	  fprintf (outstream, "MATCH [pattern]      Set MATCH expression to pattern\n");
	  fprintf (outstream, "REJECT [pattern]     Set REJECT expression to pattern\n");
	  fprintf (outstream, "STATUS [-v]>         Print server status\n");
	  fprintf (outstream, "STREAMS [-v]>        Print server streams\n");
	  fprintf (outstream, "\n");
	}  /* End of HELP */
      else if ( ! strncasecmp (cmd, "ID", 2) )
	{
	  char sendstr[255];            /* Buffer for command strings */
	  char respstr[255];            /* Buffer for server response */  
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
	      fprintf (outstream, "%s\n", respstr+3);
	    }
	  else
	    {
	      if ( respsize > 0 )
		fprintf (outstream, "Unrecognized ID response from server: %255s\n", respstr);
	      else
		fprintf (outstream, "Error requesting ID from server\n");
	    }
	}  /* End of ID */
      else if ( ! strncasecmp (cmd, "PSET", 4) )
	{
	  int64_t pktid;
	  
	  if ( sscanf (cmd, "%*s %s", svalue) != 1 )
	    {
	      fprintf (outstream, "Unrecognized usage, try PSET <value>\n");
	      
	      continue;
	    }
	  else if ( ! strncasecmp (svalue, "EARLIEST", 8) )
	    {
	      llvalue = LIBDALI_POSITION_EARLIEST;
	    }
	  else if ( ! strncasecmp (svalue, "LATEST", 6) )
	    {
	      llvalue = LIBDALI_POSITION_LATEST;
	    }
	  else
	    {
	      llvalue = strtoll (svalue, NULL, 10);
	      
	      if ( llvalue == 0 && errno == EINVAL )
		{
		  fprintf (outstream, "Unrecognized position value: %s\n", svalue);
		  
		  continue;
		}
	    }
	  
	  pktid = dl_position (dlconn, llvalue, HPTERROR);
	  
	  if ( pktid > 0 )
	    {
	      fprintf (outstream, "Positioned to packet ID %lld\n", (long long int)pktid);
	    }
	  else if ( pktid == 0 )
	    {
	      fprintf (outstream, "Packet %s not found\n", svalue);
	    }
	  else
	    {
	      fprintf (outstream, "Error requesting position %s\n", svalue);
	    }
	}  /* End of PSET */
      else if ( ! strncasecmp (cmd, "MATCH", 5) )
	{
	  int64_t count;
	  
	  rv = sscanf (cmd, "%*s %s", svalue);
	  
	  count = dl_match (dlconn, (rv>0) ? svalue : NULL);
	  
	  if ( count >= 0 )
	    {
	      fprintf (outstream, "Matched %lld streams\n", (long long int)count);
	    }
	  else
	    {
	      fprintf (outstream, "Error submitting match expression %s\n",
		       (rv>0) ? svalue : "<NONE>");
	    }
	}  /* End of MATCH */
      else if ( ! strncasecmp (cmd, "REJECT", 5) )
	{
	  int64_t count;
	  
	  rv = sscanf (cmd, "%*s %s", svalue);
	  
	  count = dl_reject (dlconn, (rv>0) ? svalue : NULL);
	  
	  if ( count >= 0 )
	    {
	      fprintf (outstream, "Rejected %lld streams\n", (long long int)count);
	    }
	  else
	    {
	      fprintf (outstream, "Error submitting reject expression %s\n",
		       (rv>0) ? svalue : "<NONE>");
	    }
	}  /* End of REJECT */
      else if ( ! strncasecmp (cmd, "STATUS", 6) )
	{
	  int formatlevel = 0;
      
	  if ( sscanf (cmd, "%*s %s", svalue) > 0 )
	    {
	      if ( strncmp (svalue, "-v", 2) == 0 )
		{
		  formatlevel += strspn (svalue+1, "v");
		}
	    }
      
	  if ( handlinfo (dlconn, "STATUS", formatlevel, NULL, outstream) )
	    {
	      fprintf (outstream, "Error requesting STATUS\n");
	    }
	}  /* End of STATUS */
      else if ( ! strncasecmp (cmd, "STREAMS", 7) )
	{
	  int formatlevel = 0;
      
	  if ( sscanf (cmd, "%*s %s", svalue) > 0 )
	    {
	      if ( strncmp (svalue, "-v", 2) == 0 )
		{
		  formatlevel += strspn (svalue+1, "v");
		}
	    }
	  
	  if ( handlinfo (dlconn, "STREAMS", formatlevel, NULL, outstream) )
	    {
	      fprintf (outstream, "Error requesting STREAMS\n");
	    }
	}  /* End of STREAMS */
      else if ( ! strncasecmp (cmd, "CONNECTIONS", 7) )
	{
	  int formatlevel = 0;
	  
	  if ( sscanf (cmd, "%*s %s", svalue) > 0 )
	    {
	      if ( strncmp (svalue, "-v", 2) == 0 )
		{
		  formatlevel += strspn (svalue+1, "v");
		}
	    }
	  
	  if ( handlinfo (dlconn, "CONNECTIONS", formatlevel, NULL, outstream) )
	    {
	      fprintf (outstream, "Error requesting CONNECTIONS\n");
	    }
	}  /* End of CONNECTIONS */
      else if ( cmdlen > 0 )
	{
	  fprintf (outstream, "Unrecognized command: %s\n", cmd);
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
 * handleinfo:
 *
 * Submit INFO command and print response.
 *
 * Return 0 on success and non-zero on error.
 ***************************************************************************/
static int
handlinfo ( DLCP *dlconn, char *infotype, int formatlevel,
	    char *clientpattern, FILE *outstream )
{
  ezxml_t xmldoc;
  char *infodata = NULL;
  int infolen = 0;
  
  if ( ! dlconn || ! infotype )
    return -1;
  
  if ( (infolen = dl_getinfo (dlconn, infotype, clientpattern, &infodata, 0)) < 0 )
    return -1;
  
  /* Parse XML string into EzXML representation */
  if ( (xmldoc = ezxml_parse_str (infodata, infolen)) == NULL )
    {
      fprintf (outstream, "Error parsing XML response\n");
      return -1;
    }
  
  /* Print formatted information */
  if ( ! strncasecmp (infotype, "STATUS", 6) )
    {
      prtinfo_status (xmldoc, formatlevel, outstream);
    }
  else if ( ! strncasecmp (infotype, "CONNECTIONS", 11) )
    {
      prtinfo_connections (xmldoc, formatlevel, outstream);
    }
  else if ( ! strncasecmp (infotype, "STREAMS", 7) )
    {
      prtinfo_streams (xmldoc, formatlevel, outstream);
    }
  else
    {
      fprintf (outstream, "Unrecognized INFO type: %s\n", infotype);
    }
  
  ezxml_free (xmldoc);
  
  if ( infodata )
    free (infodata);
  
  return 0;
}  /* End of handleinfo() */
