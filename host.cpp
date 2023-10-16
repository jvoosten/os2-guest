/*                                                                      
 *   OS/2 Guest tools for VMWare / ESXi
 *   Copyright (C)2021, Rushfan
 *   Copyright (C) 2023, Bankai Software bv
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

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INCL_DOSDEVICES
#define INCL_DOSPROCESS
#define INCL_ERRORS
#include <os2.h>

#include "backdoor.h"
#include "host.h"
#include "log.h"



#define BACKDOOR_CMD_SPEED              0x01
#define BACKDOOR_CMD_GET_MOUSE_POS      0x04
#define BACKDOOR_CMD_GET_CLIPBOARD_LEN  0x06
#define BACKDOOR_CMD_GET_CLIPBOARD_TEXT 0x07
#define BACKDOOR_CMD_SET_CLIPBOARD_LEN  0x08
#define BACKDOOR_CMD_SET_CLIPBOARD_TEXT 0x09
#define BACKDOOR_CMD_GET_VERSION        0x0A


/**
@brief Constructor

*/
Host::Host ()
{
    m_mouseHandle = -1;
}

Host::~Host ()
{
    closeMouseDriver ();
}

/**
@brief Get VMWare / ESXi backdoor verion
@return Integer value; should be '6' for now
*/
int Host::getBackdoorVersion () const
{
    return Backdoor1 (BACKDOOR_CMD_GET_VERSION);
}


/**
@brief Tell VMWare / ESXi we have integration

Does an RPC call with a version indicating self-managed software.
*/
void Host::announceToolsInstallation()
{
  int rpc, len, reply_id;
  const int buf_len = 1000;
  char buf[buf_len];

  rpc = BackdoorRPCOpen ();
  if (rpc < 0)
  {
    log (1, "[Host] Failed to open RPC channel.");
    return;
  }

  logf (2, "Opened RPC channel %d.", rpc);
  len = BackdoorRPCSend (rpc, "tools.set.version 2147483647", &reply_id);
  if (len < 0)
  {
    logf (1, "[Host] Error from RPC: %d", len);
  }
  else
  {
    if (len < buf_len)
    {
      if (BackdoorRPCReceive (rpc, buf, len, reply_id) < 0)
      {
	log (1, "[Host] Receiving of reply failed");
      }
      else
      {
	buf[len] = '\0';
	logf (2, "[Host] Received reply from RPC: %s", buf);
      }
    }
    else
    {
      logf (1, "[Host] Reply too big (%d)", len);
    }
  }
  
  BackdoorRPCClose (rpc);
  log (2, "[Host] Closed RPC channel");
}

/**
@brief Check for mouse integration with mouse driver
@return True if mouse integration is enabled

Probes the mouse driver for ESX mouse integration, and if enabled returns true.

*/
bool Host::isMouseIntegrationEnabled ()
{
    if (!openMouseDriver())
    {
    	return false;
    }
    
    bool ret = false;
    // Ioctl variables
    ULONG category = 7; // mouse functions
    ULONG function = 0x7E; // Our ioctl function
    uint16_t my_data[3] = {0};
    ULONG data_len = 6;
    
    int rc = DosDevIOCtl (m_mouseHandle, category, function,
	NULL, 0, NULL,           		// Parameters (none)
	my_data, 6, &data_len); 		// Data (3 words, only first one is used)
    if (rc == NO_ERROR)
    {
    	logf (2, "[Host::isMouseIntegrationEnabled] return = 0x%04x", my_data[0]);
    	// bit 15 is ESX detection, bit 0 is absolute mouse position enabled
	if (my_data[0] & 0x0001)
	{
	    ret = true;
	}
    }
    
    closeMouseDriver ();
    return ret;
}


void Host::setMouseIntegration (bool on_off)
{
    if (!openMouseDriver())
    {
    	return;
    }
    
    logf (2, "[Host::setMouseIntegration] -> %s", on_off ? "on" : "off");
    
    // Ioctl variables
    ULONG category = 7; // mouse functions
    ULONG function = 0x7F; // Our ioctl function
    uint16_t my_param = 0;
    ULONG param_len = 2;
    
    my_param = on_off ? 1 : 0;  // Proper command to turn integration on or off
    int rc = DosDevIOCtl (m_mouseHandle, category, function,
	&my_param, 2, &param_len,       // Parameters (1 word)
	NULL, 0, NULL);  		// Data (none)
    if (rc != NO_ERROR)
    {
    	logf (1, "[Host::setMouseIntegration] Failed to set mouse integration, error code = %d", rc);
    }
    
    closeMouseDriver ();
}


/**
@brief Check if mouse pointer is inside the guest window
@return True if pointer is inside

Performs a backdoor call to get the mouse pointer; coordinates of
-100, -100 indicate 'out of window', but we simply check for positive
coordinates.

*/
bool Host::isPointerInGuest () const
{
    int32_t i = Backdoor1 (BACKDOOR_CMD_GET_MOUSE_POS);
    logf (3, "[Host::isPointerInGuest] Pointer pos: %x" , i);
    int32_t x = i >> 16;
    int32_t y = i & 0xffff;
    return (x >= 0 && y >= 0);
}


/**
@brief Return the clipboard contents from the host
@param str String with contents in UTF-8
@return True if something was found in the clipboard.

Note that the default encoding for VMWare / ESXi is UTF-8.
*/

bool Host::getClipboard (std::string &str)
{
    LOG_FUNCTION();

    int len = Backdoor1 (BACKDOOR_CMD_GET_CLIPBOARD_LEN);
    if (len < 0) 
    {
    	log (1, "[Host::getClipboard] Failed to get clipboard length");
    	return false;
    } 
    if (len == 0) // Not sure if this can ever happen
    {
    	str = "";
    	return true;
    }
    
    char *buf = (char *) malloc (len * sizeof (int32_t) + 10);
    if (!buf)
    {
	log (0, "[Host::getClipboard] Memory allocation failed");
    }
    else
    {
	// The buffer is read 4 bytes at a time.
	uint32_t *p = (uint32_t *)buf;
	int count = (len + sizeof (uint32_t) - 1) / sizeof (uint32_t);
	while (count > 0)
	{
	    *p++ = Backdoor1(BACKDOOR_CMD_GET_CLIPBOARD_TEXT);
	    count--;
	}
	logf (2, "[Host::getClipboard] Got %d bytes in clipboard", len);
       
	str.assign (buf, len); 
	m_oldClipboard = str;
	//free (buf);
    }
    
    return true;
}


/**
@brief Set the host clipboard
@param str Contents; should be in UTF-8 encoding

Sets the host clipboard if the new content is different from the existing
clipboard. 

*/
void Host::setClipboard (const std::string &str)
{
    LOG_FUNCTION();
  
    if (str == m_oldClipboard)
    {
    	return;
    }
    
    int len = str.size();
    logf (2, "[Host::setClipboard] Pushing %d bytes", len);
    int rc = Backdoor2 (BACKDOOR_CMD_SET_CLIPBOARD_LEN, len);
    if (rc < 0)
    {
    	log (1, "[Host::setClipboard] Not allowed to paste" );
    }
    else
    {
    	// Clipboard is also pushed 4 bytes at a time
	const uint32_t* p = (const uint32_t*) str.c_str();
	for (int i = 0; i < len; i += sizeof(uint32_t)) 
	{
	  Backdoor2 (BACKDOOR_CMD_SET_CLIPBOARD_TEXT, *p++);
	}
    }
}


bool Host::openMouseDriver()
{
    if (m_mouseHandle != -1)
    {
    	return true;
    }
  
    log (1, "[Host] Opening mouse driver");
    ULONG action = 0;
    ULONG size = 0;
    ULONG attribute = FILE_NORMAL;
    ULONG open_flags = FILE_OPEN;
    ULONG open_mode = OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE;
    APIRET rc = NO_ERROR;
  
    // Open the device to the driver */
    rc = DosOpen("MOUSE$", &m_mouseHandle, &action, 
	size, 
	attribute,
	open_flags,
	open_mode,
	(PEAOP2)NULL);
    
    if (rc != NO_ERROR)
    {
    	logf (0, "[Host] Failed to open MOUSE$ driver: rc = %d", rc);
    	return false;
    }

    return true;
}


void Host::closeMouseDriver ()
{
    if (m_mouseHandle != -1)
    {
    	log (1, "[Host] Closing mouse driver");
    	DosClose (m_mouseHandle);
    	m_mouseHandle = -1;
    }
}

