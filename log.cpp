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

#include "log.h"

// Use stdint.h vs. cstdint so types are in global namespace, not std::
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <cstdlib>
#include <cstring>

#define INCL_PM
#define INCL_DOS
#define INCL_ERRORS
#include <os2.h>

// Logs at a higher level than threshold will not be logged.
static int LOG_THRESHOLD = 1;
static FILE* logh = stderr;

void set_loglevel(int lvl) 
{
  LOG_THRESHOLD = lvl;
}

void set_logfile(const char* fn) 
{
  logh = fopen(fn, "wt");
  if (!logh) {
    logh = stderr;
  }
}

void log(const char* s) 
{
  fprintf(logh, "%s\n", s);
  fflush(logh);
}

/**
 * Log at log level lvl. 
 * higher levels mean more logging.  Use 1-4.
 */
void log(int lvl, const char* s) 
{
  if (lvl <= LOG_THRESHOLD) 
  {
    log(s);
  }
}

void logf(int lvl, const char* msg, ...) 
{
  char buf[1024];
  va_list argptr;
  va_start(argptr, msg);
  vsnprintf(buf, 1024, msg, argptr);
  va_end(argptr);
  log(lvl, buf);
}

/**
\brief Print a buffer as hexadecimal values

*/
void loghex(const char *buf, int len)
{
  const char *fmt = "%-48s  %s\n";
  char buf1[50];
  char buf2[20];
  
  char *h = buf1; // hex-pos
  char *t = buf2; // text-pos
  const char *s = buf;
  int i = 0;
  
  for (i = 0; i < len; i++)
  {
    char c = *s++;
    h += sprintf (h, "%02x ", c);
    if (c < 32 || c > 127)
    {
      c = '.';
    }
    *t = c;
    t++;
    
    if ((i & 0xF) == 0xF)
    {
      *t = '\0';
      fprintf(logh, fmt, buf1, buf2);
      
      // reset pointers
      h = buf1;
      t = buf2;
    }
  }
  
  // Print final bit
  *t = '\0';
  if (h != buf1)
  {
    fprintf(logh, fmt, buf1, buf2);
  }
}









