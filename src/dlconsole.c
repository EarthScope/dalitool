/***************************************************************************
 * console routines.
 *
 * Routine for supporting an interactive console for DataLink.
 ***************************************************************************/

#include <errno.h>
#include <libmseed.h>
#include <libdali.h>

#include "dlconsole.h"

/***************************************************************************
 * consolecmd:
 *
 * Collect a command from the console, execute with the DataLink
 * server and process the return value.
 *
 * Commands recongnized:
 *  BYE or QUIT
 *  PSET <id>
 *
 * Return 0 on success and non-zero on error or exit request.
 ***************************************************************************/
int
consolecmd (DLCP *dlconn, DLPacket *dlpacket, void *packetdata)
{
  char cmd[256];
  char *input = NULL;
  char *cp = NULL;
  ssize_t cmdlen = 0;
  size_t inputlen = 0;

  char svalue[256];
  long long int llvalue;
  //int rv;
  
  FILE *instream = stdin;
  FILE *outstream = stdout;
  
  if ( ! dlpacket || ! dlconn )
    return 1;
  
  fprintf (outstream, "> ");
  
  if ( (cmdlen = getline (&input, &inputlen, instream)) < 0 )
    {
      if ( ! feof (instream) )
	dl_log (2, 0, "Error with getline\n");
      
      return -1;
    }
  
  /* Trim input string at first newline */
  if ( cmdlen > 0 && (cp = strchr (input, '\n')) )
    {
      *cp = '\0';
      cmdlen = strlen (input);
    }
  
  if ( cmdlen > 0 )
    strncpy (cmd, input, sizeof(cmd));
  else
    cmdlen = strlen (cmd);

  dl_log (1, 2, "Processing command: %s (%d)\n", cmd, cmdlen);
  
  if ( ! strncasecmp (cmd, "EXIT", 4) ||
       ! strncasecmp (cmd, "QUIT", 4) ||
       ! strncasecmp (cmd, "BYE", 3) )
    {
      fprintf (outstream, "Goodbye\n");
      
      return 1;
    }
  else if ( ! strncasecmp (cmd, "HELP", 4) )
    {
      fprintf (outstream, "The following commands are supported:\n");
      fprintf (outstream, "EXIT,QUIT,BYE        Exit console mode\n");
      fprintf (outstream, "PSET <packetID>      Issue POSITION SET command with given packet ID\n"); 
      fprintf (outstream, "\n");
    }
  else if ( ! strncasecmp (cmd, "PSET", 4) )
    {
      int64_t pktid;
      
      if ( sscanf (cmd, "%*s %s", svalue) != 1 )
	{
	  fprintf (outstream, "Unrecognized usage, try PSET <value>\n");
	  
	  return 0;
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
	      
	      return 0;
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
    }
  else
    {
      fprintf (outstream, "Unrecognized command: %s\n", cmd);
    }
  
  return 0;
}  /* End of consolecmd() */
