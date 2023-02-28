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
#include "host.h"
#include "backdoor.h"
#include "log.h"

// Use stdint.h vs. cstdint so types are in global namespace, not std::
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define INCL_DOSPROCESS
#define INCL_ERRORS
#include <os2.h>


/* From https://sites.google.com/site/chitchatvmback/backdoor
   =========================================================================================
   02h	APM function	                Y	Y	Y	Y *1	Y *1	Y	Y *1
   04h	Get mouse cursor position	Y	Y	Y	Y	Y	Y	Y
   05h	Set mouse cursor position	Y	Y	Y	Y	Y	Y	Y
   06h	Get text length from clipboard	Y	Y	Y	Y	Y	Y	Y
   07h	Get text from clipboard	        Y	Y	Y	Y	Y	Y	Y
   08h	Set text length to clipboard	Y	Y	Y	Y	Y	Y	Y
   09h	Set text to clipboard	        Y	Y	Y	Y	Y	Y	Y
   =========================================================================================
   ECX gets command
   EBX gets optional params
*/

#define BACKDOOR_CMD_GET_MOUSE_POS 0x04
#define BACKDOOR_CMD_SET_MOUSE_POS 0x05
#define BACKDOOR_CMD_GET_CLIPBOARD_LEN 0x06
#define BACKDOOR_CMD_GET_CLIPBOARD_TEXT 0x07
#define BACKDOOR_CMD_SET_CLIPBOARD_LEN 0x08
#define BACKDOOR_CMD_SET_CLIPBOARD_TEXT 0x09


void set_host_clipboard(const char *buf, uint32_t len) {
  LOG_FUNCTION();
  if (!buf) {
    return;
  }
  //fprintf(stdout, "set_host_clipboard: len: %d; buf: '%s'", len, buf); fflush(stdout);
  BackdoorOut(BACKDOOR_CMD_SET_CLIPBOARD_LEN, len);
  const uint32_t* p = (const uint32_t*) buf;
  for (int i = 0; i < len; i += sizeof(uint32_t)) {
    //fprintf(stdout, "set_host_clipboard_text_piece: %d/%d [%c]\r\n", i, len, (char*) *p); fflush(stdout);
    BackdoorOut(BACKDOOR_CMD_SET_CLIPBOARD_TEXT, *p++);
  }
  free( (void *)buf);
}

static const int16_t XPOS_IN_HOST = -100;

bool pointer_in_host(const host_point& pos) {
  return pos.x == XPOS_IN_HOST;
}

Host::Host() {}
Host::~Host() {}

host_point Host::pointer() {
  host_point pos;
  
  uint32_t i = BackdoorIn(BACKDOOR_CMD_GET_MOUSE_POS);
  pos.x = i >> 16;
  pos.y = i & 0xffff;
  return pos;
}

bool Host::pointer(const host_point& pos) {
  uint32_t d = (pos.x << 16) | pos.y;
  BackdoorOut(BACKDOOR_CMD_SET_MOUSE_POS, d);
  return true;
}

bool Host::clipboard(const char *b) {
  LOG_FUNCTION();
  if (!b) {
    return false;
  }
  int len = strlen(b);
  set_host_clipboard(b, len);
  return true;
}

/**
\brief Get clipboard from host
\return an UTF-8 encoded string or NULL if there was no data

Note: the returned string must be freed later.
*/
const char *Host::clipboard() 
{
  LOG_FUNCTION();
  int32_t len = BackdoorIn (BACKDOOR_CMD_GET_CLIPBOARD_LEN);
  if (len <= 0) {
    log(1, "failed to get clipboard length from host\r\n");
    return NULL;
  }
  char *buf = (char *)malloc (len * sizeof (int32_t) + 10);
  if (!buf)
  {
    log(1, "Memory allocation failed\r\n");
    return NULL;
  }

   // The buffer is read 4 bytes at a time.
   uint32_t *current = (uint32_t *)buf;
   len = (len + sizeof (uint32_t) - 1) / sizeof (uint32_t);
   while (len > 0)
   {
      *current = BackdoorIn(BACKDOOR_CMD_GET_CLIPBOARD_TEXT);
      current++;
      len--;
   }

   return buf;
}

