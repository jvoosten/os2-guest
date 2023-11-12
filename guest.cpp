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

#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unidef.h>

#define INCL_PM
#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>

#include "guest.h"
#include "log.h"

Guest::Guest()
{
    m_hab = 0;
    m_hmq = 0;
}

bool Guest::initialize ()
{
    bool ret = true;
    /* Create connection to windows system and a message queue. We don't actually
       use the queue but somehow it is needed for the clipboard.
     */
    m_hab = WinInitialize (0L);
    if (!m_hab)
    {
    	log (0, "[Guest] Failed to create HAB");
    	ret = false;
    }
    m_hmq = WinCreateMsgQueue (m_hab, 0);
    if (!m_hmq)
    {
     	log (0, "[Guest] Failed to create HMQ");
     	ret = false;
    }

    /* Create codepage conversion objects */
    int rc = UniCreateUconvObject ((UniChar *)L"@sub=yes", &m_local_ucs);
    if (rc != ULS_SUCCESS)
    {
    	log (0, "[Guest] Failed to create local UniConv object");
    	ret = false;
    }
    rc = UniCreateUconvObject ((UniChar *)L" IBM-1208" , &m_utf8_ucs);
    if (rc != ULS_SUCCESS)
    {
    	log (0, "[Guest] Failed to create UTF-8 UniConv object");
    	ret = false;
    }

    return ret;
}

Guest::~Guest()
{
    // Cleanup
    UniFreeUconvObject (m_local_ucs);
    UniFreeUconvObject (m_utf8_ucs);
    WinDestroyMsgQueue (m_hmq);
    WinTerminate (m_hab);
}




/**
 \brief Sets the guest clipboard contents
 \param str Input string in UTF-8 format

 Sets the clipboard if it is different from the current one.

 */
 void Guest::setClipboard (const std::string &str)
{
    LOG_FUNCTION ();

    if (str == m_oldClipboard)
    {
      return;
    }

    int rc = 0;
    size_t len = str.size ();
    const char *inbuf = str.c_str();

    if (!WinOpenClipbrd (m_hab))
    {
    	log(0, "[Guest::setClipboard] Failed to open clipboard");
    	m_oldClipboard = "";
    	return;
    }

    log (3, "[Guest::setClipboard] Opened clipboard");
    WinEmptyClipbrd (m_hab);

    logf (2, "[Guest::setClipboard] Input length = %d bytes", len);
    if (len > 0)
    {
	/* Do all the stuff to convert to the local codepage; first to UCS, then local */
	// Allocate UCS memory; at least as many characters as in the input string. We can
	// assume that any input strings will result in equal or less the number UCS characters,
	// as UTF-8 can use more than one byte for a single codepoint.
	UniChar *utmp = (UniChar *)calloc (len + 1, sizeof (UniChar));
	if (!utmp)
	{
	    log (0, "[Guest::setClipboard] Failed to allocate memory for conversion");
	}
	else
	{
	    rc = UniStrToUcs (m_utf8_ucs, utmp, (char *)inbuf, len + 1); // Len + 1 for the terminating byte
	    if (rc != ULS_SUCCESS)
	    {
		logf (0, "[Guest::setClipboard] Conversion to UCS failed: 0x%x", rc);
	    }
	    else
	    {
	    	size_t ulen = UniStrlen (utmp); // This is how many characters we have in our UCS buffer
	    	logf (3, "[Guest::setClipboard] UCS string = %d characters", ulen);
	    	
		// For the clipboard we must use shared memory
		char *buf = NULL;
		int sh_buf_len = 2 * len + 10;
		rc = DosAllocSharedMem((PPVOID)&buf,
				   NULL, sh_buf_len,
				   PAG_COMMIT | PAG_WRITE | OBJ_GIVEABLE);
		if (rc != NO_ERROR)
		{                                          
		    log (0, "[Guest::setClipboard] Failed to allocate shared memory for guest clipboard");
		}
		else
		{
		    rc = UniStrFromUcs (m_local_ucs, buf, utmp, sh_buf_len); // +1 for null byte
		    if (rc != ULS_SUCCESS)
		    {
			logf (1, "[Guest::setClipboard] Conversion to local codepage failed: 0x%x", rc);
			DosFreeMem (buf);  // Prevent memory leaks
		    }
		    else
		    {
		    	logf (3, "[Guest::setClipboard] Output length = %d bytes", strlen (buf));
			// Finally! Set the clipboard
			rc = WinSetClipbrdData (m_hab, (ULONG) buf, CF_TEXT, CFI_POINTER);
			if (rc == 0)
			{
			    logf (1, "Failed to set clipboard data");
			}
			else
			{
			    log (2, "[Guest::setClipboard] Text pasted into clipboard");
			}
		    }
		} // .. DosAllocSharedMem
	    } // ..UniStrToUcs
	    free ((void *)utmp);
	} // ..utmp allocated
    } // ..if len > 0

    m_oldClipboard = str;
    WinCloseClipbrd (m_hab);
}


/**
\brief Gets the guest clipboard contents
\return A string in UTF-8 encoding

The text is internally transformed from the current codepage to UTF-8.
Also CR/LF pairs are converted to single LF's, Unix style.

*/
bool Guest::getClipboard (std::string &str)
{
    LOG_FUNCTION();

    int rc = 0;
    bool ret = false;
    if (!WinOpenClipbrd (m_hab))
    {
    	log (0, "[Guest::getClipboard] Failed to open Clipboard");
    	m_oldClipboard = "";
    	return false;
    }

    log (3, "[Guest::getClipboard] WinOpenClipbrd: success");
    ULONG fmtInfo = 0;

    if (!WinQueryClipbrdFmtInfo (m_hab, CF_TEXT, &fmtInfo))
    {
    	log (2, "[Guest::getClipboard] No text in clipboard");
    	str = "";
    	m_oldClipboard = "";
    	ret = true;
    }
    else
    {
	const char *org = (const char *)WinQueryClipbrdData (m_hab, CF_TEXT);

	if (org)
	{
	    char *copy = NULL;
	    UniChar *utmp = NULL;

	    size_t len = strlen (org);
	    size_t clen = 3 * len + 1;
	    size_t ulen = 2 * len + 1; 
	    copy = (char *) malloc (clen); // Allocate enough for the output UTF-8 string
	    utmp = (UniChar *) calloc (ulen, sizeof (UniChar)); // Intermediate UCS-2 buffer
	    if (!copy || !utmp)
	    {
		log (0, "[Guest::getClipboard] Failed to allocate memory for conversion");
	    }
	    else
	    {
	    	logf (3, "[Guest::getClipboard] Input length = %d bytes" , len);
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
		rc = UniStrToUcs (m_local_ucs, utmp, copy, ulen); // Len + 1 for null byte?
		if (rc != ULS_SUCCESS)
		{
		    logf (1, "[Guest::getClipboard] Failed conversion to UCS: 0x%x", rc);
		}
		else
		{
		    ulen = UniStrlen (utmp); // This is how many characters we really have in our UCS buffer
		    logf (3, "[Guest::getClipboard] UCS string = %d characters", ulen);
		    
		    // Step 3: back to UTF, into the 'copy'  buffer
		    rc = UniStrFromUcs (m_utf8_ucs, copy, utmp, clen);
		    if (rc != ULS_SUCCESS)
		    {
			logf (1, "[Guest::getClipboard] Failed conversion to UTF-8: 0x%x", rc);
		    }
		    else
		    {
			str = copy;
			ret = true;
		    	logf (3, "[Guest::setClipboard] Output length = %d bytes", str.size ());
			log (2, "[Guest::getClipboard] Contents assigned");
		    }
		}
	    }

	    free (utmp);
	    free (copy);
	}
    }

    log (3, "[Guest::getClipboard] Closing clipboard");
    WinCloseClipbrd (m_hab);
    return ret;
}

/**
@brief Reboot guest OS 

Issues a command to properly reboot the OS, but applications do not have
the ability to halt the process (e.g. prompt you with a question like "Discard changes to file X?").

Intended for shutdown when the VM manager asks for it. Should not return, unless something went wrong.
*/

void Guest::rebootOS ()
{
    // From edm2.org
    int rc;
    HFILE dosfile;
    ULONG action = 0;
    const int IOCTL_DOS = 0x00D5;
    const int DOS_REBOOT = 0x00AB;
    
    log (1, "[Guest::rebootOS]");
    rc = DosOpen("\\DEV\\DOS$", &dosfile, &action, 0,
    	FILE_NORMAL, 
    	OPEN_ACTION_OPEN_IF_EXISTS, 
    	OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, 0);
    if (rc != NO_ERROR)
    {
    	logf (0, "[Guest::rebootOS] Failed to open DOS$ device: %d", rc);
    	return;
    }
 
    rc = DosShutdown (0);
    if (rc != NO_ERROR)
    {
    	logf (0, "[Guest::rebootOS] DosShutdown failed: %d", rc);
    }
    else
    {
	rc = DosDevIOCtl (dosfile, IOCTL_DOS, DOS_REBOOT, 
	    NULL, 0, NULL, NULL, 0, NULL);
	// Still here?? Then we did not succeed
	logf (0, "[Guest::rebootOS] IOCTL failed: %d", rc);
    }
    DosClose (dosfile);
}

