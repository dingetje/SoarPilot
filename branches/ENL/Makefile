#Makefile for gcc PalmPilot crosscompiler m68k-palmos-gcc package.
# Change the strings from APP to BUILDPRC to match your own
# system configuration.
# Place the file into the developping directory.
# "make clean" removes all generated files, even the ".prc".
# "make doc" generates source code documentation, requires doxygen
APP             =soaring
ICONTEXT        ="SoarPilot"
APPID           =Soar
RCP             =$(APP).rcp
PRC             =$(APP).prc
SRC             =$(wildcard *.c)
OBJS            =$(wildcard *.o)
GRC             =$(APP).grc
BIN             =$(APP).bin

CC              =m68k-palmos-gcc
AS              =m68k-palmos-gcc
PILRC           =pilrc
TXT2BITM        =txt2bitm
OBJRES          =m68k-palmos-obj-res
BUILDPRC        =build-prc
MULTIGEN        =m68k-palmos-multigen
DOXYGEN			="C:/Program Files/doxygen/bin/doxygen.exe"

# uncomment this if you want to build a gdb debuggable version
# -On: n=Optimization-level (0=none), -g: needed for debugging
#CFLAGS          =-O0 -g $(DEFINES) $(INCLUDES)
#CFLAGS          =-O0 -g -palmos3.1 $(DEFINES) $(INCLUDES)
#CFLAGS          =-O2 -palmos4.0 -Wall -Werror $(DEFINES) $(INCLUDES)
#CFLAGS          =-O2 -palmos3.5 -Wall -Werror $(DEFINES) $(INCLUDES)
#CFLAGS          =-O2 -palmos4.0 -Wall -Werror $(DEFINES) $(INCLUDES)
CFLAGS          =-O2 -palmos5r3 -Wall -Werror $(DEFINES) $(INCLUDES)
#CFLAGS          =-O2 -palmos5 -Wall -Werror $(DEFINES) $(INCLUDES)
ASFLAGS         =-c

all:            $(PRC)

$(PRC):        $(APP) bin.stamp ;
	$(BUILDPRC) --backup $(APP).def $(APP) *.bin $(LINKFILES) 
	ls -l *.prc

#	$(CC) $(CFLAGS) -o $@ $^ /usr/m68k-palmos/lib/libnfm.a 
#	$(CC) $(CFLAGS) -o $@ $^ libnfm.a
#	$(CC) $(CFLAGS) -o $@ $^ -lnfm
#	$(CC) $(CFLAGS) -o $@ $^ -lnfm -lPalmOSGlue
$(APP):       $(SRC:.c=.o) $(APP)-sections.o $(APP)-sections.ld ;
	$(CC) $(CFLAGS) -o $@ $^ -lnfm 

bin.stamp:    $(RCP) ;
	$(PILRC) $^ $(BINDIR)
	touch $@

%.o:  %.c ;
	$(CC) $(CFLAGS) -c $< -o $@
   #               touch $<
   # enable this line if you want to compile EVERY time.

$(APP)-sections.o: $(APP)-sections.s ;
	$(CC) -c $(APP)-sections.s

#	multigen $(APP).def
$(APP)-sections.s $(APP)-sections.ld: $(APP).def ;
	$(MULTIGEN) $(APP).def

clean:
	rm -rf *.o $(APP) *.bin *.grc *.stamp *-sections.*

cleant:
	rm -rf *.o $(APP) *.bin *.grc *.stamp *-sections.*
	rm -rf *~

# .PHONY = always build target (don’t look for it in the filesystem)
.PHONY: doc	
doc:
	$(DOXYGEN) Doxyfile
