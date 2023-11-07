

#define INCL_DOSDEVICES
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR   /* File manager values */
#define INCL_DOSNLS
#define INCL_DOSPROCESS

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uconv.h>

#include "backdoor.h" 
#include "host.h"
#include "log.h"


int main(int arg, char *argv[])
{
    Host host;
    std::string cmd;
    
    set_loglevel (3);
    
    host.initialize ();
    host.announceToolsInstallation ();
    printf ("CPU speed = %d.\n" , Backdoor1 (1));	
    printf ("VMWare backdoor version = %d.\n" , host.getBackdoorVersion ());	

    host.setCapability ("statechange");
    host.setCapability ("softpowerop_retry");

    for (int i = 0; i < 100; i++)
    {
    	DosSleep (250);
    	host.getHostCommand (cmd);
    	if (cmd.size() > 0)
    	{
    	    printf("Command = '%s'\n", cmd.c_str());
    	}
    	
    	if ("reset"  == cmd)
    	{
    	    host.replyHost ("OK ATR toolbox");
    	}
    	else if ("ping"  == cmd)
    	{
    	    host.replyHost ("OK "); // we're here
    	}
    }
    	
  
    return 0;
}

