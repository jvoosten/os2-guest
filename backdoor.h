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
#ifndef INCLUDED_BACKDOOR_H
#define INCLUDED_BACKDOOR_H

#include <unistd.h>

extern "C" {
extern int BackdoorIn (int);
extern int BackdoorOut (int, int);

/* Open RPC channel to host VM; return -1 on error or channel number (0-7)*/
extern int BackdoorRPCOpen ();
/** 
\brief Send an RPC command (string) to the VM host;
\param channel Channel number from RPCOpen()
\param out String to string to the host
\param reply_id [out] An ID to receive the reply 
\return -1 upon error, otherwise the length of the reply

The reply_id is necessary to receive the correct string from the VM; pass in
the address of a variable.
*/
extern int BackdoorRPCSend (int channel, const char *out, int *reply_id);
/* Receive RPC reply in buffer */
extern int BackdoorRPCReceive (int channel, char *in, int len, int reply_id);
/* Close RPC channel */
extern void BackdoorRPCClose (int channel);

}


#endif
