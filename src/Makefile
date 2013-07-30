
# Standard compiler parameters
CFLAGS += -I../libdali -I../ezxml -I../libmseed

# GCC specific parameters
GCC = gcc
GCCFLAGS = -O2 -Wall -I../libdali -I../ezxml -I../libmseed

LDFLAGS = -L../libdali -L../ezxml -L../libmseed
LDLIBS  = -ldali -lezxml -lmseed

# For SunOS/Solaris uncomment the following line
#LDLIBS = -ldali -lezxml -lmseed -lsocket -lnsl -lrt

BIN  = ../dalitool

OBJS = dlconsole.o dalixml.o dalitool.o

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

static: $(OBJS)
	$(CC) $(CFLAGS) -static -o $(BIN) $(OBJS) $(LDFLAGS) $(LDLIBS)

cc:
	@$(MAKE) "CC=$(CC)" "CFLAGS=$(CFLAGS)"

gcc:
	@$(MAKE) "CC=$(GCC)" "CFLAGS=$(GCCFLAGS)"

debug:
	$(MAKE) "CC=$(CC)" "CFLAGS=-g $(CFLAGS)"

gccdebug:
	$(MAKE) "CC=$(GCC)" "CFLAGS=-g $(GCCFLAGS)"

clean:
	rm -f $(OBJS) $(BIN)

install:
	@echo
	@echo "No install method, copy the binary and man page yourself"
	@echo

