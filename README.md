# OS2 guest support for VMWare / ESXi

VMWare guest support for OS/2; loosely based on the project on [[https://github.com/wwiv/os2-guest]].

# Features

- Copy/paste text from and to host clipboard
- Mouse integration (*)
- Shutdown & reboot from the host OS

Copy / paste will transform text to and from the current code page in OS/2, but 
will of course only convert those characters that will fit in that code page. The 
interaction with the VMWare host uses UTF-8 so can code any character from OS/2 to host,
but the other way around is not always possible.
 
* Mouse integration requires the proper mouse driver that enbles "absolute mouse positioning". 
Without this driver the mouse will work in the VM but will be captured inside the window. 

# Prerequisites

To build os2-guest tools you need the Open Watson C/C++ compiler.


