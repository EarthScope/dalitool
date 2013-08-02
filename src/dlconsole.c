/***************************************************************************
 * console routines.
 *
 * Routine for supporting an interactive console for DataLink.
 ***************************************************************************/

// Add more commands, E.G. POSITION AFTER, more?

// Add READ command

// Add STREAM commmand

// Add completions for command line


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
 *  CONNECTIONS [pattern] [-v]
 *
 * Return 0 on success and non-zero on error or exit request.
 ***************************************************************************/
int
runconsole (DLCP *dlconn, DLPacket *dlpacket, void *packetdata, int verbose)
{
  static char cmd[256] = "";
  static int cmdlen = 0;
  char *line = NULL;
  int linelen = 0;
  
  char str1[256];
  char str2[256];
  char *strp[2] = {str1,str2};
  int strc;
  
  FILE *outstream = stdout;
  
  if ( ! dlpacket || ! dlconn )
    return -1;
  
  linenoiseHistorySetMaxLen (100);

  fprintf (outstream, "DataLink Console, type 'help' to list available commands\n");
  
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
      
      strc = sscanf (cmd, "%*s %s %s", str1, str2);
      
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
	  fprintf (outstream, "EXIT,QUIT,BYE               Exit console mode\n");
	  fprintf (outstream, "ID                          Print ID of server\n");
	  fprintf (outstream, "PSET <packetID>             Issue POSITION SET command to position reader\n");
	  fprintf (outstream, "MATCH [pattern]             Set MATCH expression to pattern\n");
	  fprintf (outstream, "REJECT [pattern]            Set REJECT expression to pattern\n");
	  fprintf (outstream, "STATUS [-v]>                Print server status\n");
	  fprintf (outstream, "STREAMS [-v]>               Print server streams\n");
	  fprintf (outstream, "CONNECTIONS [pattern] [-v]  Print server connections\n");
	  fprintf (outstream, "\n");
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
	  
	  if ( strc != 1 )
	    {
	      fprintf (outstream, "Unrecognized usage, try PSET <value>\n");
	      
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
		  fprintf (outstream, "Unrecognized position value: %s\n", str1);
		  
		  continue;
		}
	    }
	  
	  pktid = dl_position (dlconn, pktid, HPTERROR);
	  
	  if ( pktid > 0 )
	    {
	      fprintf (outstream, "Positioned to packet ID %lld\n", (long long int)pktid);
	    }
	  else if ( pktid == 0 )
	    {
	      fprintf (outstream, "Packet %s not found\n", str1);
	    }
	  else
	    {
	      fprintf (outstream, "Error requesting position %s\n", str1);
	    }
	}  /* End of PSET */
      else if ( ! strncasecmp (cmd, "MATCH", 5) )
	{
	  int64_t count;
	  
	  count = dl_match (dlconn, (strc>0) ? str1 : NULL);
	  
	  if ( count >= 0 )
	    {
	      fprintf (outstream, "Matched %lld streams\n", (long long int)count);
	    }
	  else
	    {
	      fprintf (outstream, "Error submitting match expression %s\n",
		       (strc>0) ? str1 : "<NONE>");
	    }
	}  /* End of MATCH */
      else if ( ! strncasecmp (cmd, "REJECT", 5) )
	{
	  int64_t count;
	  
	  count = dl_reject (dlconn, (strc>0) ? str1 : NULL);
	  
	  if ( count >= 0 )
	    {
	      fprintf (outstream, "Rejected %lld streams\n", (long long int)count);
	    }
	  else
	    {
	      fprintf (outstream, "Error submitting reject expression %s\n",
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
	  
	  if ( handlinfo (dlconn, "STATUS", formatlevel, NULL, outstream) )
	    {
	      fprintf (outstream, "Error requesting STATUS\n");
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
	  
	  if ( handlinfo (dlconn, "STREAMS", formatlevel, NULL, outstream) )
	    {
	      fprintf (outstream, "Error requesting STREAMS\n");
	    }
	}  /* End of STREAMS */
      else if ( ! strncasecmp (cmd, "CONNECTIONS", 7) )
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
	  
	  if ( handlinfo (dlconn, "CONNECTIONS", formatlevel, matchptr, outstream) )
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
