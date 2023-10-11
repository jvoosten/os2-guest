#define INCL_DOSERRORS
#define INCL_DOSDEVICES
#define INCL_DOSFILEMGR   /* File manager values */
#define INCL_DOSNLS


#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uconv.h>

#include "backdoor.h" 


int main(int arg, char *argv[])
{
  HFILE mouseHandle = 0;
  ULONG action = 0;
  ULONG size = 0;
  ULONG attribute = FILE_NORMAL;
  ULONG open_flags = FILE_OPEN;
  ULONG open_mode = OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE;
  APIRET rc = NO_ERROR;
  
  // Ioctl variables
  ULONG category = 7; // mouse functions
  ULONG function = 0; // our ioctl function
  int16_t my_param[3] = {0};
  ULONG param_len = 6;
  int16_t my_data[3] = {0};
  ULONG data_len = 6;

  // Open the device to the driver */
  rc = DosOpen("MOUSE$", &mouseHandle, &action, 
  	size, 
	attribute,
	open_flags,
	open_mode,
	(PEAOP2)NULL);
  if (rc != NO_ERROR)
  {
    printf ("Failed to open AMOUSE$: %d.\n", rc);
    return 1;
  }
  printf ("Mouse driver opened.\n");

  /* Try to disable absolute positioning */
  function = 0x7F;      // Set ESX
  param_len = 2;
  my_param[0] = 0;  	// sub-command
  rc = DosDevIOCtl (mouseHandle, category, function,
   	my_param, 6, &param_len,   	// Parameters (3 word)
	NULL, 0, NULL);  			// Data (none)
  if (rc == NO_ERROR)
  {
    printf ("Turned off absolute mouse position.\n");
  }
  else
  {
    printf ("Error with ESX IOCTL call: %d\n", rc);
  }

  DosClose (mouseHandle);
  return 0;
}

