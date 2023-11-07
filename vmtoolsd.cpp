/*                                                                      
 *   OS/2 Guest Tools for VMWare
 *   Copyright (C)2021, Rushfan
 *
 *   Licensed  under the  Apache License, Version  2.0 (the "License");
 *   you may not use this  file  except in compliance with the License.
 *   You may obtain a copy of the License at                          
 *                                                                     
 *               http://www.apache.org/licenses/LICENSE-2.0           
 *                                                                     
 *   Unless  required  by  applicable  law  or agreed to  in  writing,
 *   software  distributed  under  the  License  is  distributed on an
 *   "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,
 *   either  express  or implied.  See  the  License for  the specific
 *   language governing permissions and limitations under the License.
 */
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSMEMMGR
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_ERRORS
#define INCL_KBD
#define INCL_PM

#include <os2.h>

#include "guest.h"
#include "host.h"
#include "log.h"


int vmtools_daemon() 
{
    LOG_FUNCTION();
    
    Host host;
    Guest guest;

    if (!host.initialize ())
    {
    	log (0, "Host initialization failed, exiting");
    	return 2;
    }
    
    if (!guest.initialize ())
    {
    	log (0, "Guest initialization failed, exiting");
    	return 2;
    }
    
    if (host.getBackdoorVersion () != 6)
    {
    	log (0, "Not a supported backdoor version");
    	return 1;
    }
    
    host.announceToolsInstallation ();
    
    if (!host.isMouseIntegrationEnabled ())
    {
    	host.setMouseIntegration (true);
    }

    bool pointer_status = false;
    bool old_pointer_status = false;
    for (;;) {
    	DosSleep(250);
    	
    	// Whenever the mouse pointer leaves or enters the window, we 
    	// update the clipboards of host and guest
    	pointer_status = host.isPointerInGuest ();
    	if (pointer_status != old_pointer_status)
    	{
    	    std::string s;
    	    if (pointer_status)
    	    {
    	    	log (1, "Window enter event");
    	    	if (host.getClipboard (s))
    	    	{
    	    	    guest.setClipboard (s);
    	    	}
    	    }
    	    else
    	    {
    	    	log (1, "Window leave event");
    	    	if (guest.getClipboard (s))
    	    	{
    	    	    host.setClipboard (s);
    	    	}
    	    }
    	    old_pointer_status = pointer_status;
    	}
    }

    host.setMouseIntegration (false);
    return 0;
}


/**
@brief Check if there is an instance running
@return 0 On success (we're first), 1 otherwise

Uses a named semaphore to check if there is already an instance running.
*/
static int check_first_instance ()
{
    const char *sem_name = "\\SEM32\\VMToolsd";
    HEV semaphore;
    int rc;
    
    rc = DosCreateEventSem (sem_name, &semaphore, 0, FALSE);
    if (rc != 0)
    {
    	// Something went wrong...
    	if (rc == ERROR_DUPLICATE_NAME)
    	{
    	    return 1;
    	}
    	else
    	{
    	    return 2;
    	}
    }
    return 0;
}

int main(int argc, char* argv[]) 
{
  printf ("vmtools: OS/2 Guest tools for ESXI/VMWare.\r\n\r\n");

  if (check_first_instance ())
  {
      printf ("Tools already running, exiting.\r\n");
      return 1;
  }
  
  // Interactive mode

  for (int i = 1; i < argc; i++) 
  {
    const char* arg = argv[i];
    const int alen = strlen(arg);
    if (!alen) 
    {
      continue;
    }
    if (arg[0] == '-' || arg[0] == '/') 
    {
      if (alen < 2) 
      {
	continue;
      }
      // Process switch.  If this should not go to a daemon
      // then exit while processing the command.
      char schar = (char)toupper(*(arg+1));
      const char* sval = (arg+2);
      switch (schar) 
      {
	case 'D': 
	  if (sval) 
	  {
	    set_loglevel(atoi(sval));
	  }
	  break;
	case 'L': 
	  if (sval) 
	  {
	    set_logfile(sval);
	  }
	  break;
      }
      continue;
    }
  } 

  return vmtools_daemon();
}


