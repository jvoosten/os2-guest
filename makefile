#########################################################
# Makefile: OS/2 Guest Tools for VMWare
#

VERSION = 2.2.0
CC 	= wcl386
AS 	= wasm

# debug info and warning level 3
CFLAGS		= -d3 -w3 -xs
ASFLAGS 	= -d3
LDFLAGS 	= debug all
SYSTEM		= os2v2
SYSTEM_PM	= os2v2_pm

# Binary and Objects for vmtoolsd
VMTOOLSD_EXE	= vmtoolsd.exe
VMTOOLSD_OBJS	= backdoor.obj	&
		log.obj		&
		guest.obj 	&
		host.obj 	&
		vmtoolsd.obj

#########################################################
# Makefile rules 

.cpp.obj: .AUTODEPEND
	$(CC) $(CFLAGS) -DVERSION="$(VERSION)" -c $<

.asm.obj:
	$(AS) $(ASFLAGS) $<

all:	config.h $(VMTOOLSD_EXE) $(VMTOOLSCTL_EXE) vmtest.exe vmon.exe vmoff.exe

.BEFORE
config.h: config.h.in 
	sed s/@VMTOOLS_VERSION@/$(VERSION)/ $< > $@

vmtoolsd.exe: config.h $(VMTOOLSD_OBJS)
	wlink system $(SYSTEM_PM) $(LDFLAGS) name $@ &
	 file {$(VMTOOLSD_OBJS)} library libconv library libuls

vmoff.exe:	vmoff.obj backdoor.obj
	wlink system $(SYSTEM) $(LDFLAGS) name $@ &
	  file {vmoff.obj backdoor.obj}

vmon.exe:	vmon.obj backdoor.obj
	wlink system $(SYSTEM) $(LDFLAGS) name $@ &
	  file {vmon.obj backdoor.obj}

# A test tool
vmtest.exe:     vmtest.obj backdoor.obj guest.obj host.obj log.obj
	wlink system $(SYSTEM) $(LDFLAGS) name vmtest.exe &
	 file {vmtest.obj backdoor.obj guest.obj host.obj log.obj} library libconv library libuls

DIST_DIR	= .\os2-guest-$(VERSION)
DIST_ZIP	= os2-guest-$(VERSION).zip
DIST_README	= $(DIST_DIR)\readme.txt
README		= docs\readme.txt

$(DIST_DIR): 
	mkdir $(DIST_DIR)

$(DIST_README): $(README)
	sed s/@VMTOOLS_VERSION@/$(VERSION)/ $< > $@

dist: config.h $(VMTOOLSD_EXE) vmon.exe vmoff.exe $(DIST_DIR) $(DIST_README) .SYMBOLIC
	-del $(DIST_DIR)\*.exe
	-del $(DIST_DIR)\*.txt
	copy $(VMTOOLSD_EXE) $(DIST_DIR)
	copy vmon.exe $(DIST_DIR)
	copy vmoff.exe $(DIST_DIR)
	sed s/@VMTOOLS_VERSION@/$(VERSION)/ $(README) > $(DIST_README)
	zip -r $(DIST_ZIP) $(DIST_DIR)\*

clean: .SYMBOLIC
	-del config.h
	-del *.obj
	-del $(VMTOOLSD_EXE)
	-del vmtest.exe
	-del vmon.exe vmoff.exe
	-rm -fr $(DIST_DIR)
	-del $(DIST_ZIP)

