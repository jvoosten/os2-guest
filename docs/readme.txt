OS/2 Guest support for VMWare @VMTOOLS_VERSION@

OS/2 Guest support for VMWare provides the following guest services for OS/2
guests running under VMWare/ESXI:

* Clipboard Syncronization - the contents of the text clipboard are shared between
  the guest and the host. Text is converted to and from the host encoding as best as
  possible.

* Mouse pointer integration - The mouse pointer can leave the guest VM
  without needing to ungrab it (Alt-Control). Also when the mouse pointer
  is grabbed by the guest, usually when you click inside the VM, the mouse
  pointer should be in the right location for the guest and not the previous
  location.

  Note: This requires AMOUSE version ..... 
  
* Reboot / shutdown support from the host environment. This will properly reboot 
  or shutdown OS/2 when choosing that option from your host window, in stead of the
  'hard' power off or reset.
  
Installation
============

1) Put VMTOOLSD.EXE in a directory on your machines, for example 
   C:\usr\local\bin or C:\bin.

2) Add a line in C:\startup.cmd to launch VMTOOLSD.EXE using "start /PM".
   You don't need to specify any parameter. Example:
   
   start /PM C:\BIN\VMTOOLSD.EXE

Options
=======

-D# (where # is 0-4, higher value means more debugging)
-LXXX (where XXX is the name of the log file)

Example:

	START /PM vmtoolsd.exe -D3 -LC:\logs\vmtoolsd.log

The default log level is 1; level 0 only logs errors. 
By default no log file is written.

Known Limitations
=================
* The mouse pointer synchronization requires the AMOUSE driver version .... 
* Clipboard integration requires VMTOOLSD.EXE to be started from startup.cmd
  using "START /PM", or run from a command session prompt the same way.
*    
  
See https://github.com/jvoosten/os2-guest/issues for the full list.


Soure Code and Help
===================
OS/2 Guest support for VMWare is Free and Open Sourec Software (FOSS), the
source is available on https://github.com/jvoosten/os2-guest


