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
    Host ();
    ~Host ();

    // Get VMWare backdoor version
    int getBackdoorVersion () const;
    
    // Tell VMWare we have integration
    void announceToolsInstallation();

    // Is mouse integration already enabled
    bool isMouseIntegrationEnabled ();
    
    // Turn mouse integration on or off
    void setMouseIntegration (bool on_off);
    
    // Test mouse pointer position
    bool isPointerInGuest () const;

    // Get clipboard from host
    std::string getClipboard ();

    // Set clipboard in host
    void setClipboard (const std::string &str);
  
private:
    HFILE m_mouseHandle;

    std::string m_oldClipboard;	 
    
    bool openMouseDriver ();
    void closeMouseDriver ();
};


