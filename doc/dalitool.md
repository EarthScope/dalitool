# <p >dalitool 
###  DataLink client for data stream inspection and server queries</p>

1. [Name](#)
1. [Synopsis](#synopsis)
1. [Description](#description)
1. [Options](#options)
1. [Examples](#examples)
1. [Author](#author)

## <a id='synopsis'>Synopsis</a>

<pre >
dalitool [options] [host][:][port] [repeat]
</pre>

## <a id='description'>Description</a>

<p ><b>dalitool</b> connects to a <u>DataLink</u> server and either queries the server for information or collects selected data streams and prints information about the packets received.  All received packets can optionally be written to a single file.</p>

<p ><b>dalitool</b> also has an interactive command-line console mode where DataLink commands are sent to the server and the response is printed. Console mode is initiated with the <b>-c</b> option.</p>

<p >Results of server information queries can either be printed as raw XML as returned by the server or as formatted output.  If a repeat interval is specified with an information query the query will be repeated at the given interval in seconds.</p>

## <a id='options'>Options</a>

<b>-V</b>

<p style="padding-left: 30px;">Report program version and exit.</p>

<b>-h</b>

<p style="padding-left: 30px;">Print program usage and exit.</p>

<b>-v</b>

<p style="padding-left: 30px;">Be more verbose.  This flag can be used multiple times ("-v -v" or "-vv") for more verbosity.</p>

<b>-c</b>

<p style="padding-left: 30px;">Connect to specified DataLink server and enter console mode, an interactive command and response prompt.  Type 'help' to list available commands.</p>

<b>-p</b>

<p style="padding-left: 30px;">Print details of received packets.  This flag can be used multiple times ("-p -p" or "-pp") for more detail.</p>

<b>-d</b>

<p style="padding-left: 30px;">Print the first 6 sample values for each received time-series packet, implies at least one -p flag.</p>

<b>-D</b>

<p style="padding-left: 30px;">Print all sample values for each received time-series packet, implies at least one -p flag.</p>

<b>-m </b><u>match</u>

<p style="padding-left: 30px;">Specify a matching expression to send to the server.  This regular expression is used to either limit the stream packets collected or the streams or clients reported in information queries.  For packets this expression is matched against the stream ID, nominally in the form 'NET_STA_LOC_CHAN/TYPE'.  For clients this expression is matched against the client address (host:port) string and the client ID, a successful match against either means the client will be included in the results.  If the expression begins with an '@' character it is assumed to be a file containing a list of expressions for matching.</p>

<b>-r </b><u>reject</u>

<p style="padding-left: 30px;">Specify a rejecting expression to send to the server.  This regular expression is used to limit the stream packets collected and is logically opposite of the matching expression.  This expression is matched against the stream ID, nominally in the form 'NET_STA_LOC_CHAN/TYPE'.  If the expression begins with an '@' character it is assumed to be a file containing a list of expressions for rejecting.</p>

<b>-k </b><u>interval</u>

<p style="padding-left: 30px;">Specify keepalive packet interval (in seconds) at which keepalive packets are sent to the server.  Keepalive packets are only sent if nothing is received within the interval.</p>

<b>-x </b><u>statefile</u>

<p style="padding-left: 30px;">During client shutdown the last received packet ID and packet creation time will be saved in this file.  If this file exists upon startup the information will be used to resume the data collection from the point at which it was stopped.  In this way the client can be stopped and started without data loss, assuming the data are still available on the server.</p>

<b>-o </b><u>outfile</u>

<p style="padding-left: 30px;">If specified, all received packets will be appended to this file.  The file is created if it does not exist.  A special mode for this option is to send all received packets to standard output when the outfile is specified as '-'.  In this case all diagnostic program output will be redirected to standard error.</p>

<b>-i </b><u>type</u>

<p style="padding-left: 30px;">Send an information request; the returned raw XML response is printed. Supported information types are: STATUS, STREAMS and CONNECTIONS.  The results of STREAMS and CONNNECTIONS queries can be limited to specific streams or connections using the <b>-m</b> and <b>-r</b> options. .PP Formatted information requests can be made with these options:</p>

<pre style="padding-left: 30px;">
-I   print server status and either exit or repeat at interval
-Q   print stream list and either exit or repeat at interval
-C   print connection list and either exit or repeat at interval
</pre>

<b>-f</b>

<p style="padding-left: 30px;">Increase the level of detail printed for formatted INFO responses. This flag can be used multiple times ("-f -f" or "-ff") for increased levels.</p>

<b></b><u>[host][:][port]</u>

<p style="padding-left: 30px;">A required argument, specifies the address of the DataLink server in host:port format.  Either the host, port or both can be omitted.  If host is omitted then localhost is assumed, i.e. ':16000' implies 'localhost:16000'.  If the port is omitted then 16000 is assumed, i.e. 'localhost' implies 'localhost:16000'.  If only ':' is specified 'localhost:16000' is assumed.</p>

<b></b><u>[repeat]</u>

<p style="padding-left: 30px;">A repeat interval in seconds for server information queries.</p>

## <a id='examples'>Examples</a>

<p style="padding-left: 30px;">The following would connect to a DataLink server at data.host.edu port 16000 and print details of all received packets.  Since no match or reject patterns were specified the server will send all data.</p>

<p style="padding-left: 30px;"><b>>dalitool -p data.host.edu:16000</b></p>

<p style="padding-left: 30px;">The '-m' and '-r' arguments can be used to limit the data packets, in the following packets with stream IDs beginning with "US" are matched and packets with stream IDs beginning with "US_DMGT" are rejected. This would effectively select the US network data streams with the exception of station DGMT.  Port 16000 will be assumed as none is specified.</p>

<p style="padding-left: 30px;"><b>>dalitool -v -p -m '^US.*' -r '^US_DGMT.*' data.host.edu</b></p>



<p style="padding-left: 30px;">The following requests and formats a list of current streams in the server.</p>

<p style="padding-left: 30px;"><b>>dalitool -S data.host.edu</b></p>

<p style="padding-left: 30px;">An information request can be continuously repeated by supplying a repeat interval, for example to print the connection list every 5 seconds:</p>

<p style="padding-left: 30px;"><b>>dalitool -C data.host.edu 5</b></p>

## <a id='author'>Author</a>

<pre >
Chad Trabant
IRIS Data Management Center
</pre>


(man page 2013/10/04/)
