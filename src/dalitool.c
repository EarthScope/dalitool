/***************************************************************************
 *
 * A DataLink client for data stream inspection and server information.
 *
 * Written by Chad Trabant
 *   EarthScope Data Services
 ***************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WIN32
#include <signal.h>
#endif

#include <libdali.h>
#include <libmseed.h>

#include "common.h"
#include "dalixml.h"
#include "dlconsole.h"

#define PACKAGE "dalitool"
#define VERSION "2023.335"

static char verbose        = 0; /* Flag to control general verbosity */
static char console        = 0; /* Flag to control interactive console session */
static char ppackets       = 0; /* Flag to control printing of data packets */
static char psamples       = 0; /* Flag to control printing of data samples */
static char formatinfo     = 0; /* Flag to control formatting of INFO XML */
static int repeatint       = 0; /* Repeat interval for INFO requests */
static char formatlevel    = 0; /* Flag to control formatted output verbosity */
static char *statefile     = 0; /* State file for saving/restoring the seq. no. */
static char *matchpattern  = 0; /* Source ID matching expression */
static char *rejectpattern = 0; /* Source ID rejecting expression */
static char *clientpattern = 0; /* Client matching expression */
static char *infotype      = 0; /* INFO type to request */
static char *outfile       = 0; /* The output file */
static FILE *outfp         = 0; /* Output file descriptor */

static DLCP *dlconn; /* connection parameters */

/* Functions internal to this source file */
static int parameter_proc (int argcount, char **argvec);
static char *getoptval (int argcount, char **argvec, int argopt);
static void print_stderr (const char *message);
static void usage (void);

#ifndef WIN32
static void term_handler (int sig);
#endif

int
main (int argc, char **argv)
{
  DLPacket dlpacket;
  char packetdata[MAXPACKETSIZE];
  char *infobuf = 0;
  int infolen;

  dltime_t current;
  dltime_t next;

#ifndef WIN32
  /* Signal handling, use POSIX calls with standardized semantics */
  struct sigaction sa;

  sa.sa_flags = SA_RESTART;
  sigemptyset (&sa.sa_mask);

  sa.sa_handler = term_handler;
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGQUIT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);

  sa.sa_handler = SIG_IGN;
  sigaction (SIGHUP, &sa, NULL);
  sigaction (SIGPIPE, &sa, NULL);
#endif

  /* Process given parameters (command line and parameter file) */
  if (parameter_proc (argc, argv) < 0)
  {
    dl_log (2, 0, "Parameter processing failed\n\n");
    dl_log (2, 0, "Try '-h' for detailed help\n");
    return -1;
  }

  /* Connect to server */
  if (dl_connect (dlconn) < 0)
  {
    dl_log (2, 0, "Error connecting to server\n");
    return -1;
  }

  /* Reposition connection */
  if (dlconn->pktid > 0)
  {
    if (dl_position (dlconn, dlconn->pktid, dlconn->pkttime) < 0)
      return -1;
  }

  /* Send match pattern if supplied */
  if (matchpattern)
  {
    if (dl_match (dlconn, matchpattern) < 0)
      return -1;
  }

  /* Send reject pattern if supplied */
  if (rejectpattern)
  {
    if (dl_reject (dlconn, rejectpattern) < 0)
      return -1;
  }

  /* Request INFO and print returned XML either formatted nicely or not */
  if (infotype)
  {
    /* Initialize next request time stamp */
    next = dlp_time ();

    while (!dlconn->terminate)
    {
      current = dlp_time ();

      /* Submit INFO request and process response if at the next time stamp */
      if (current >= next)
      {
        if ((infolen = dl_getinfo (dlconn, infotype, clientpattern, &infobuf, 0)) < 0)
        {
          dl_log (2, 0, "Problem requesting INFO from server\n");
          return -1;
        }

        /* Print formatted INFO */
        if (formatinfo)
          info_handler (infotype, infobuf, infolen, formatlevel);
        /* Print raw XML */
        else
          printf ("%.*s\n", infolen, infobuf);

        if (infobuf)
          free (infobuf);

        /* Update the next time stamp to the repeat interval */
        next += (repeatint * DLTMODULUS);

        /* If the next time stamp is not in the future move up */
        if (next < current)
          next = current + (repeatint * DLTMODULUS);

        /* If repeating add a newline to separate output */
        if (repeatint)
          printf ("\n");
      }
      /* Otherwise sleep for 1/10 second */
      else
      {
        dlp_usleep (100000);
      }

      /* If not repeating jump out */
      if (!repeatint)
        break;
    }
  }
  /* Enter interactive console mode */
  else if (console)
  {
    if (runconsole (dlconn, &dlpacket, packetdata, sizeof (packetdata), verbose))
    {
      dl_log (2, 0, "Error running console()\n");
    }
  }
  /* Otherwise collect packets in STREAMing mode */
  else
  {
    /* Collect packets in streaming mode */
    while (dl_collect (dlconn, &dlpacket, packetdata, sizeof (packetdata), 0) == DLPACKET)
    {
      packet_handler (&dlpacket, packetdata, ppackets, psamples, (outfile && outfp) ? outfp : NULL);
    }
  }

  /* Shutdown */
  if (dlconn->link != -1)
    dl_disconnect (dlconn);

  if (outfp)
    fclose (outfp);

  if (statefile)
    dl_savestate (dlconn, statefile);

  return 0;
} /* End of main() */

/***************************************************************************
 * parameter_proc:
 *
 * Process the command line parameters.
 *
 * Returns 0 on success, and -1 on failure
 ***************************************************************************/
static int
parameter_proc (int argcount, char **argvec)
{
  char *address = 0;
  int keepalive = -1;
  int optind;

  /* Process all command line arguments */
  for (optind = 1; optind < argcount; optind++)
  {
    if (strcmp (argvec[optind], "-V") == 0)
    {
      fprintf (stderr, "%s version: %s\n", PACKAGE, VERSION);
      exit (0);
    }
    else if (strcmp (argvec[optind], "-h") == 0)
    {
      usage ();
      exit (0);
    }
    else if (strncmp (argvec[optind], "-v", 2) == 0)
    {
      verbose += strspn (&argvec[optind][1], "v");
    }
    else if (strncmp (argvec[optind], "-c", 2) == 0)
    {
      console = 1;
    }
    else if (strncmp (argvec[optind], "-p", 2) == 0)
    {
      ppackets += strspn (&argvec[optind][1], "p");
    }
    else if (strcmp (argvec[optind], "-d") == 0)
    {
      psamples = 1;
    }
    else if (strcmp (argvec[optind], "-D") == 0)
    {
      psamples = 2;
    }
    else if (strcmp (argvec[optind], "-k") == 0)
    {
      keepalive = strtoul (getoptval (argcount, argvec, optind++), NULL, 10);
    }
    else if (strcmp (argvec[optind], "-o") == 0)
    {
      outfile = getoptval (argcount, argvec, optind++);
    }
    else if (strcmp (argvec[optind], "-x") == 0)
    {
      statefile = getoptval (argcount, argvec, optind++);
    }
    else if (strcmp (argvec[optind], "-m") == 0)
    {
      matchpattern = getoptval (argcount, argvec, optind++);
    }
    else if (strcmp (argvec[optind], "-r") == 0)
    {
      rejectpattern = getoptval (argcount, argvec, optind++);
    }
    else if (strcmp (argvec[optind], "-i") == 0)
    {
      infotype   = getoptval (argcount, argvec, optind++);
      formatinfo = 0;
    }
    else if (strcmp (argvec[optind], "-I") == 0)
    {
      infotype   = "STATUS";
      formatinfo = 1;
    }
    else if (strcmp (argvec[optind], "-S") == 0)
    {
      infotype   = "STREAMS";
      formatinfo = 1;
    }
    else if (strcmp (argvec[optind], "-C") == 0)
    {
      infotype   = "CONNECTIONS";
      formatinfo = 1;
    }
    else if (strncmp (argvec[optind], "-f", 2) == 0)
    {
      formatlevel += strspn (&argvec[optind][1], "f");
    }
    else if (strncmp (argvec[optind], "-", 1) == 0)
    {
      fprintf (stderr, "Unknown option: %s\n", argvec[optind]);
      exit (1);
    }
    else if (!address)
    {
      address = argvec[optind];
    }
    else if (!repeatint)
    {
      repeatint = strtoul (argvec[optind], NULL, 10);
    }
    else
    {
      fprintf (stderr, "Unknown option: %s\n", argvec[optind]);
      exit (1);
    }
  }

  /* Make sure a server was specified */
  if (!address)
  {
    fprintf (stderr, "No DataLink server specified\n\n");
    fprintf (stderr, "%s version %s\n\n", PACKAGE, VERSION);
    fprintf (stderr, "Usage: %s [options] [host][:][port] [int]\n\n", PACKAGE);
    fprintf (stderr, "Try '-h' for detailed help\n");
    exit (1);
  }

  /* Allocate and initialize a new connection description */
  dlconn = dl_newdlcp (address, argvec[0]);

  /* Set keepalive parameter, allow for valid value of 0 */
  if (keepalive >= 0)
    dlconn->keepalive = keepalive;

  /* Initialize the verbosity for the dl_log function */
  dl_loginit (verbose, NULL, NULL, NULL, NULL);

  /* Open output file if requested */
  if (outfile)
  {
    if (!strcmp (outfile, "-"))
    {
      /* Re-direct all messages to standard error */
      dl_loginit (verbose, &print_stderr, NULL, &print_stderr, NULL);

      outfp = stdout;
      setvbuf (stdout, NULL, _IONBF, 0);
    }
    else if ((outfp = fopen (outfile, "a+b")) != NULL)
    {
      setvbuf (outfp, NULL, _IONBF, 0);
    }
    else
    {
      dl_log (2, 0, "cannot open output file: %s\n", outfile);
      exit (1);
    }
  }

  /* Report the program version */
  dl_log (1, 1, "%s version: %s\n", PACKAGE, VERSION);

  /* Make sure we print basic packet details if printing samples */
  if (psamples && ppackets == 0)
    ppackets = 1;

  /* Load the match stream list from a file if the argument starts with '@' */
  if (matchpattern && *matchpattern == '@')
  {
    char *filename = matchpattern + 1;

    if (!(matchpattern = dl_read_streamlist (dlconn, filename)))
    {
      dl_log (2, 0, "Cannot read matching list file: %s\n", filename);
      exit (1);
    }
  }

  /* Load the reject stream list from a file if the argument starts with '@' */
  if (rejectpattern && *rejectpattern == '@')
  {
    char *filename = rejectpattern + 1;

    if (!(rejectpattern = dl_read_streamlist (dlconn, filename)))
    {
      dl_log (2, 0, "Cannot read rejecting list file: %s\n", filename);
      exit (1);
    }
  }

  /* Set up for INFO CONNECTIONS request and matching */
  if (matchpattern && infotype && !strncasecmp (infotype, "CONNECTIONS", 11))
  {
    clientpattern = matchpattern;
    matchpattern  = 0;
  }

  /* Recover from the state file and reposition */
  if (statefile)
  {
    if (dl_recoverstate (dlconn, statefile) < 0)
    {
      dl_log (2, 0, "Error reading state file\n");
      exit (1);
    }
  }

  return 0;
} /* End of parameter_proc() */

/***************************************************************************
 * getoptval:
 *
 * Return the value to a command line option; checking that the value is
 * itself not an option (starting with '-') and is not past the end of
 * the argument list.
 *
 * argcount: total arguments in argvec
 * argvec: argument list
 * argopt: index of option to process, value is expected to be at argopt+1
 *
 * Returns value on success and exits with error message on failure
 ***************************************************************************/
static char *
getoptval (int argcount, char **argvec, int argopt)
{
  if (argvec == NULL || argvec[argopt] == NULL)
  {
    fprintf (stderr, "getoptval(): NULL option requested\n");
    exit (1);
  }

  /* Special case of '-o -' usage */
  if ((argopt + 1) < argcount && strcmp (argvec[argopt], "-o") == 0)
    if (strcmp (argvec[argopt + 1], "-") == 0)
      return argvec[argopt + 1];

  if ((argopt + 1) < argcount && *argvec[argopt + 1] != '-')
    return argvec[argopt + 1];

  fprintf (stderr, "Option %s requires a value\n", argvec[argopt]);
  exit (1);

  return NULL; /* To stop compiler warnings about no return */
} /* End of getoptval() */

/***************************************************************************
 * print_stderr:
 *
 * Print the given message to standard error.
 ***************************************************************************/
static void
print_stderr (const char *message)
{
  fprintf (stderr, "%s", message);
  return;
}

#ifndef WIN32
/***************************************************************************
 * term_handler:
 *
 * Signal handler routine to set the termination flag.
 ***************************************************************************/
static void
term_handler (int sig)
{
  dl_terminate (dlconn);
}
#endif

/***************************************************************************
 * usage:
 *
 * Print the usage message and exit.
 ***************************************************************************/
static void
usage (void)
{
  fprintf (stderr, "%s version %s\n\n", PACKAGE, VERSION);
  fprintf (stderr, "Usage: %s [options] [host][:][port] [repeat]\n\n", PACKAGE);
  fprintf (stderr,
           " ## General program options ##\n"
           " -V              report program version\n"
           " -h              show this usage message\n"
           " -v              be more verbose, multiple flags can be used\n"
           " -c              console mode, provide an interactive prompt\n"
           " -p              print details of data packets, multiple flags can be used\n"
           " -d              print first 6 samples of each data packet\n"
           " -D              print all samples of each data packet\n"
           " -m match        specify stream ID matching pattern\n"
           " -r reject       specify stream ID rejecting pattern\n"
           " -k interval     send keepalive packets this often (seconds)\n"
           " -x sfile        save/restore state information to this file\n"
           " -o outfile      write all received packets to this file\n"
           "\n"
           " ## Data server information ##\n"
           " -i type         send info request, type is one of the following:\n"
           "                   STATUS, STREAMS, CONNECTIONS\n"
           "                   the returned raw XML is displayed when using this option\n"
           " -I              print formatted server id, version and status\n"
           " -S              print formatted stream list (if supported by server)\n"
           " -C              print formatted connection list (if supported by server)\n"
           " -f              increase level of details included in formatted output\n"
           "\n"
           " [host][:][port] Address of the DataLink server in host:port format\n"
           "                   Default host is 'localhost' and default port is '16000'\n"
           "\n"
           " [repeat]        Specify a repeat interval in seconds for INFO requests\n"
           "\n");

} /* End of usage() */
