
# Use 'wmake -f Makefile.wat'

.BEFORE
	@set INCLUDE=.;$(%watcom)\H;$(%watcom)\H\NT
	@set LIB=.;$(%watcom)\LIB386

cc     = wcc386
cflags = -zq
lflags = OPT quiet OPT map LIBRARY ..\libdali\libdali.lib LIBRARY ..\ezxml\libezxml.lib LIBRARY ..\libmseed\libmseed.lib LIBRARY ws2_32.lib
cvars  = $+$(cvars)$- -DWIN32

BIN = ..\dalitool.exe

INCS = -I..\libdali -I..\ezxml -I..\libmseed

all: $(BIN)

$(BIN):	linenoise.obj common.obj dlconsole.obj dalixml.obj dalitool.obj
	wlink $(lflags) name $(BIN) file {linenoise.obj common.obj dlconsole.obj dalixml.obj dalitool.obj}

# Source dependencies:
linenoise.obj:	linenoise.c linenoise.h
common.obj:	common.c common.h
dlconsole.obj:	dlconsole.c dlconsole.h
dalitxml.obj:	dalixml.c dalixml.h
dalitool.obj:	dalitool.c dsarchive.h dlconsole.h dalixml.h

# How to compile sources:
.c.obj:
	$(cc) $(cflags) $(cvars) $(INCS) $[@ -fo=$@

# Clean-up directives:
clean:	.SYMBOLIC
	del *.obj *.map $(BIN)
