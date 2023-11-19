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

#include <string>
#include <uconv.h>

/**
 * Class to encapsulate interfacing with the local Guest OS
 */
class Guest 
{
public:
    Guest();
    ~Guest();
    
    bool initialize ();
    
    // Gets the guest clipboard contents
    bool getClipboard (std::string  &Str);
    
    // Sets the guest clipboard contents
    void setClipboard (const std::string &str);
    
    // Is mouse integration already enabled
    bool isMouseIntegrationEnabled ();
    
    // Turn mouse integration on or off
    void setMouseIntegration (bool on_off);
    
    // Reboot guest OS
    void rebootOS ();
    
    // Halt (power off) guest OS
    void haltOS ();
    
private:
    unsigned long m_hab;
    unsigned long m_hmq;
  
    std::string m_oldClipboard;
    
    /// For conversion of local strings to UTF8 for the external clipboard
    UconvObject m_local_ucs, m_utf8_ucs;

    HFILE m_mouseHandle;

    bool openMouseDriver ();
    void closeMouseDriver ();
};



