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

#include <os2.h>
#include <string>

/**
@brief Class to communicate with the host OS
*/
class Host
{
public:	
    enum ResultCode
    {
    	Ok = 0,
    	CommError = -1,
    	Failed = -2
    };
    
    Host ();
    ~Host ();

    // Open host communication
    bool initialize ();
    
    // Get VMWare backdoor version
    int getBackdoorVersion () const;
    
    // Tell VMWare we have integration
    void announceToolsInstallation ();

    // Test mouse pointer position
    bool isPointerInGuest () const;

    // Get clipboard from host
    bool getClipboard (std::string &str);

    // Set clipboard in host
    void setClipboard (const std::string &str);
    
    // Get TCLO command (if any)
    bool getHostCommand (std::string &str);
    // Set reply to the Host of last TCLO command
    void replyHost (const std::string &str);
    // Restart command channel 
    bool restartCommandChannel ();
  
    // Set capability in VMHost
    ResultCode setCapability (const std::string &str);
    // Set capability in VMHost with value
    bool setCapability (const std::string &str, unsigned int value);
    
    // Do a full send/reply roundtrip over the RPC channel
    int rpcSendReply (const std::string &send, std::string &reply);

private:
    std::string m_oldClipboard;	 
    
    int m_rpcChannel;
    int m_tcloChannel;
    std::string m_tcloReply;     // Reply we must send to the host over the TCLO channel
};

