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

#pragma once

#include <unistd.h>

extern "C" {
/**
@brief Perform backdoor call with a single command
@param cmd Command
@return Value in eax
*/
extern int Backdoor1 (int cmd);

/**
@brief Perform backdoor call with command and (function) parameter
@param cmd Command
@param func Sub-function
@return Value in eax
*/
extern int Backdoor2 (int cmd, int func);

/**
@brief Perform backdoor call with command and parameter, return all registers
@param cmd Command
@param func Sub-function
@param regs Storage for all CPU registers

Performs backdoor call and puts eax, ebx, ecx and edx into @p regs[0...3]

*/
extern void BackdoorAll (int cmd, int func, int regs[4]);

/**
@brief Open RPC channel to host VM
@return  -1 on error or channel number (0-7)
*/
extern int BackdoorRPCOpen ();
/** 
@brief Send an RPC command (string) to the VM host
@param channel Channel number from BackdoorRPCOpen()
@param out String to string to the host
@param reply_id [out] An ID to receive the reply 
@return -1 upon error, otherwise the length of the reply

The reply_id is necessary to receive the correct string from the VM; use this
ID with BackdoorRPCReceive() to get the answer from the VM host.
*/
extern int BackdoorRPCSend (int channel, const char *out, int *reply_id);
/**
@brief Receive RPC reply in buffer 
@param channel Channel number from BackdoorRPCOpen()
@param in String where reply will be copied
@param len Length of buffer for @p in
@param reply_id ID as returned by BackdoorRPCSend()
@return 0 on success, -1 on error, -2 if RPC could not be completed

Receives a reply from the VM host, copies up to @p len bytes to the @p in buffer.
If there is not enough room the reply will be truncated; there is no way to retrieve
the remainder.
*/
extern int BackdoorRPCReceive (int channel, char *in, int len, int reply_id);
/**
@brief Close RPC channel
@param channel Channel number from BackdoorRPCOpen()

Note: you can send multiple commands through an RPC channel, as long as you
read out any reply before sending the next command.
*/
extern void BackdoorRPCClose (int channel);

}


