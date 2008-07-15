#
# Wmake file - For Watcom's wmake
# Use 'wmake -f Makefile.wat'

all: .SYMBOLIC
	cd libdali
	wmake -f Makefile.wat
        cd ..\ezxml
        wmake -f Makefile.wat
	cd ..\libmseed
	wmake -f Makefile.wat
	cd ..\src
	wmake -f Makefile.wat
	cd ..

clean: .SYMBOLIC
	cd libdali
	wmake -f Makefile.wat clean
        cd ..\ezxml
        wmake -f Makefile.wat clean
	cd ..\libmseed
	wmake -f Makefile.wat clean
	cd ..\src
	wmake -f Makefile.wat clean
	cd ..
