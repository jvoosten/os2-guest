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

#include <stdlib.h>
#include <string.h>

#include "guest.h"

#include "log.h"

// Use stdint.h vs. cstdint so types are in global namespace, not std::
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>
#include <cstring>

#define INCL_PM
#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>


Guest::Guest() {
  PTIB ptib;
  PPIB ppib;
  DosGetInfoBlocks(&ptib, &ppib);
  ppib->pib_ultype = 3;

  last_point_.x = -1;
  last_point_.y = -1;
}

bool Guest::initialize ()
{
  // TODO(rushfan): Check for errors here.
  hab_ = WinInitialize(0L);
  if (!hab_) {
    log(0, "Failed to create HAB\r\n");
    return false;
  }
  hmq_ = WinCreateMsgQueue(hab_, 0);
  if (!hmq_) {
    log(0, "Failed to create HMQ\r\n");
    return false;
  }

  screen_max_y_ = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
  logf(2, "Screen size max y: [%d]\r\n", screen_max_y_);
  
  /* Create codepage conversion objects */
  int ret = UniCreateUconvObject ((UniChar *)L"", &local_ucs);
  if (ret != ULS_SUCCESS)
  {
     log (0, "Failed to create local UniConv object\r\n");
     return false;
  }
  ret = UniCreateUconvObject ((UniChar *)L" IBM-1208" , &utf8_ucs);
  if (ret != ULS_SUCCESS)
  {
     log (0, "Failed to create UTF-8 UniConv object\r\n");
     return false;
  }
  
  return true;
}

Guest::~Guest() {
  // Cleanup
  UniFreeUconvObject (local_ucs);
  UniFreeUconvObject (utf8_ucs);
  WinDestroyMsgQueue(hmq_);
  WinTerminate(hab_);
}

  
/** Gets the guest pointer position */
guest_point Guest::pointer() {
  POINTL pos;
  WinQueryPointerPos(HWND_DESKTOP, &pos);

  guest_point guest_pos;
  guest_pos.x = pos.x;
  guest_pos.y = pos.y;

  if (last_point_.x != pos.x && last_point_.y != pos.y) {
    logf(1, "new pos: [%d, %d]; old: [%d,%d]", pos.x, pos.y, last_point_.x, last_point_.y);
    last_point_.x = pos.x;
    last_point_.y = pos.y;
  } else {
    logf(4, "remaining at last point: [%d, %d]", pos.x, pos.y);
  }

  return guest_pos;
}


/** Sets the guest pointer position */
bool Guest::pointer(const guest_point& pos) {
  if (!WinSetPointerPos(HWND_DESKTOP, pos.x, pos.y)) {
    logf(0, "failed to set pointer pos: %d", WinGetLastError(hab_));
  }
  if (last_point_.x != pos.x && last_point_.y != pos.y) {
    logf(1, "new pos: [%d, %d]; old: [%d,%d]", pos.x, pos.y, last_point_.x, last_point_.y);
    last_point_.x = pos.x;
    last_point_.y = pos.y;
  } else {
    logf(4, "remaining at last point: [%d, %d]", pos.x, pos.y);
  }
  return true;
}

bool Guest::pointer_visible(bool visible) {
  logf(1, "Guest::pointer_visible(%s)", visible ? "true": "false");
  WinShowPointer(HWND_DESKTOP, visible ? TRUE : FALSE);
  return true;
}

/** Sets the guest clipboard contents or releases b if that fails. */
bool Guest::clipboard(const char* b) {
  if (WinOpenClipbrd(hab_)) {
    log(3, "opened clipboard");
    WinEmptyClipbrd(hab_);
    WinSetClipbrdData(hab_, (ULONG) b, CF_TEXT, CFI_POINTER);
    WinCloseClipbrd(hab_);
    return true;
  }
  log(0, "Failed to open Clipboard");
  DosFreeMem((PVOID) b);
  return false;
}


/** 
\brief Gets the guest clipboard contents or NULL if none exist 
\return A string in UTF-8 encoding

The text is internally transformed from the current codepage to UTF-8.
Also CR/LF pairs are converted to single LF's, Unix style.

*/
const char *Guest::clipboard() {
  LOG_FUNCTION();
  if (!WinOpenClipbrd(hab_)) {
    log(2, "Failed to open Clipboard");
    return NULL;
  }
  
  const char *ret = NULL;
  log(2, "WinOpenClipbrd: succeed");
  ULONG fmtInfo = 0;
  if (WinQueryClipbrdFmtInfo(hab_, CF_TEXT, &fmtInfo)) 
  {
    log(3, "Has text in clipboard");
    const char *org = (const char *)WinQueryClipbrdData(hab_, CF_TEXT);
    if (org) 
    {
      int rc = 0;
      int len = 0;
      char *copy = NULL;
      UniChar *utmp = NULL;
      
      len = strlen (org);
      copy = (char *) malloc (len * 3 + 10); // Allocate enough for the output UTF-8 string
      utmp = (UniChar *) malloc (len * 2 + 10); // Intermediate UCS-2 buffer
      if (!copy || !utmp)
      {
	log (1, "Failed to allocate memory for conversion\r\n");
      }
      else
      {
	// Step one: convert 0D/0A to just 0A for copy/paste
	const char *s = org;
	char *d = copy;
	while (*s)
	{
	  if (*s != '\r')
	  {
	    *d = *s;
	    d++;
	  }
	  s++;
	}
	*d = '\0'; // Finish string
	
	// Step two: convert to UCS-2
	rc = UniStrToUcs (local_ucs, utmp, copy, len);
	if (rc != ULS_SUCCESS)
	{
	  log (0, "Failed conversion to UCS\r\n");
	}
	else
	{
	  // Step 3: back to UTF, into the 'copy'  buffer
	  rc = UniStrFromUcs (utf8_ucs, copy, utmp, len * 3);
	  if (rc != ULS_SUCCESS)
	  {
	    log (0, "Failed conversion to UTF-8\r\n");
	  }
	  else
	  {
	    ret = copy;
	  }
	}
      }
      
#if 0
      // Some hexprint debug
      char buf[250];
      s = ret;
      d = buf;
      int count = 0;
      buf[0] = '\0';
      while (*s)
      {
	d += sprintf (d, "%02x ", (int)(*s));
	s++;
	len--;
	count++;
	if (count == 16)
	{
	  log (3, buf);
	  count = 0;
	  d = buf;
	}
      }
      if (count > 0)
      {
	log (3, buf);
      }
#endif
      
      log(1, "contents assigned");
      free (utmp);
      if (!ret)
      {
        free (copy);
      }
    }
  }
  log(4, "Closing Clipboard");
  WinCloseClipbrd(hab_);
  return ret;
}

guest_point Guest::host_to_guest(const host_point& hp) {
  guest_point r;
  r.x = hp.x;
  r.y = screen_max_y_ - hp.y;
  
  return r;
}

host_point Guest::guest_to_host(const guest_point& gp) {
  host_point r;
  r.x = gp.x;
  r.y = screen_max_y_ - gp.y;

  return r;
}



