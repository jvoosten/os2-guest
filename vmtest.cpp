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




int send_something_rpc()
{
  int rpc, len, reply_id;
  char buf[0x8000];


  rpc = BackdoorRPCOpen ();
  if (rpc < 0)
  {
    printf ("Failed to open RPC channel.\n");
    return 1;
  }

  printf ("Opened RPC channel %d.\n", rpc);
  len = BackdoorRPCSend (rpc, "tools.set.version 2147483647", &reply_id);
  if (len < 0)
  {
    printf ("Error from RPC: %d\n", len);
  }
  else
  {
    if (len < 0x80000)
    {
      if (BackdoorRPCReceive (rpc, buf, len, reply_id) < 0)
      {
	printf ("Receiving of reply failed.\n");
      }
      else
      {
	buf[len] = '\0';
	printf ("Reply from RPC: %s\n", buf);
      }
    }
    else
    {
      printf ("Reply too big (%d)\n", len);
    }
  }
  
  BackdoorRPCClose (rpc);
  printf ("Closed RPC channel.\n");
  return 0;
}


/* Use our new IOCTL calls to probe ESX */
int probe_ioctl()
{
  HFILE mouseHandle = 0;
  ULONG action = 0;
  ULONG size = 0;
  ULONG attribute = FILE_NORMAL;
  ULONG open_flags = FILE_OPEN;
  ULONG open_mode = OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE;
  APIRET rc = NO_ERROR;
  
  // Open the device to the driver */
  rc = DosOpen("MOUSE$", &mouseHandle, &action, 
  	size, 
	attribute,
	open_flags,
	open_mode,
	(PEAOP2)NULL);
  if (rc != NO_ERROR)
  {
    printf ("Failed to open AMOUSE$: %d\n", rc);
    return 1;
  }
  printf ("Mouse driver opened.\n");
  
  // Ioctl variables
  ULONG category = 7; // mouse functions
  ULONG function = 0; // our ioctl function
  ULONG param_len = 0;
  int16_t my_data = 0;
  ULONG data_len = 2;
  
  // Get number of buttons
  function = 0x60; 
  rc = DosDevIOCtl (mouseHandle, category, function,
   	NULL, 0, NULL,            // Parameters (none)
	&my_data, sizeof(my_data), &data_len);  // Data (2 bytes)
  if (rc == NO_ERROR)
  {
    printf ("Number of buttons: %d\n" , my_data);
  }
  else
  {
    printf ("Error with IOCTL call for number of buttons: %d\n" , rc);
  }
  
  function = 0x7E;  // GET_ESX
  data_len = 2;
  rc = DosDevIOCtl (mouseHandle, category, function,
   	NULL, 0, NULL,            // Parameters (none)
	&my_data, sizeof(my_data), &data_len);  // Data (6 bytes)
  if (rc == NO_ERROR)
  {
    printf ("ESX call: 0x%x\n" , my_data);
  }
  else
  {
    printf ("Error with ESX IOCTL call: %d\n", rc);
  }

  // We're done
  DosClose (mouseHandle);
  return 0;
}


/* Try to write to the mouse driver; enable absolute positioning, read a few positions and then disable again */

int set_ioctl()
{
  HFILE mouseHandle = 0;
  ULONG action = 0;
  ULONG size = 0;
  ULONG attribute = FILE_NORMAL;
  ULONG open_flags = FILE_OPEN;
  ULONG open_mode = OPEN_SHARE_DENYNONE | OPEN_FLAGS_FAIL_ON_ERROR | OPEN_ACCESS_READWRITE;
  APIRET rc = NO_ERROR;
  
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
  
  // Ioctl variables
  ULONG category = 7; // mouse functions
  ULONG function = 0; // our ioctl function
  int16_t my_param[3] = {0};
  ULONG param_len = 6;
  int16_t my_data[3] = {0};
  ULONG data_len = 6;
  
  // Get mouse type
  function = 0x6B;
  data_len = 6;
  rc = DosDevIOCtl (mouseHandle, category, function,
   	NULL, 0, NULL,            // Parameters (none)
	my_data, 2, &data_len);  // Data (2 bytes)
  if (rc == NO_ERROR)
  {
    printf ("Mouse type: %d, secondary type: %d.\n" , my_data[0], my_data[2]);
  }
  else
  {
    printf ("Error with IOCTL call for number of buttons: %d\n" , rc);
  }
  
  function = 0x7E;  // GET_ESX
  data_len = 6;
  rc = DosDevIOCtl (mouseHandle, category, function,
   	NULL, 0, NULL,            // Parameters (none)
	my_data, 6, &data_len);  // Data (3 words)
  if (rc == NO_ERROR)
  {
    printf ("ESX status: 0x%x.\n" , (uint16_t)my_data[0]);
  }
  else
  {
    printf ("Error with ESX IOCTL call: %d\n", rc);
  }
  
  
  /* Try to enable absolute positioning */
  function = 0x7F;      // Set ESX
  param_len = 2;
  my_param[0] = 1;  	// sub-command
  rc = DosDevIOCtl (mouseHandle, category, function,
   	my_param, 2, &param_len,   	// Parameters (1 word)
	NULL, 0, NULL);  		// Data (none)
  if (rc == NO_ERROR)
  {
    printf ("Enabled absolute mouse position.\n");
  }
  else
  {
    printf ("Error with ESX IOCTL call: %d\n", rc);
  }

  int status = 0;
  int regs[4] = {0};
  
#if 1
  for (int i = 0; i < 20; i++)
  {
    printf ("Sleeping...\n");
    sleep (1);
  }
#endif    

#if 0
  for (int i = 0; i < 10; i++)
  {
    // Get again with mouse positioning 
    function = 0x7E;  // GET_ESX
    data_len = 6;
    rc = DosDevIOCtl (mouseHandle, category, function,
  	NULL, 0, NULL,            // Parameters (none)
  	my_data, 6, &data_len);   // Data (3 words)
    if (rc == NO_ERROR)
    {
      printf ("Mouse position: status = %x | x = %d, y = %d.\n" ,  (uint16_t)my_data[0], (uint16_t)my_data[1], (uint16_t)my_data[2]);
    }
    sleep (1);
  }
#endif    
  
#if 0
  // Keep looping until we click the middle button
  while  (!(regs[0] & 0x8))
  {
    status = BackdoorOut (40, 0);
    printf (" status = 0x%x", status);
    if (((status & 0xFFFF0000) != 0xFFFF0000) && ((status & 0xFFFF) >= 4))
    {
      BackdoorAll (39, 4, regs);
      printf (" | regs = 0x%08x 0x%08x 0x%08x 0x%08x", regs[0], regs[1], regs[2], regs[3]);
    }
    printf("\n");
    sleep (1);
  }
#endif
  
  // Turn absolute position off
  /* Try to enable absolute positioning */
  function = 0x7F;      // Set ESX
  param_len = 2;
  my_param[0] = 0;  	// sub-command
  rc = DosDevIOCtl (mouseHandle, category, function,
   	my_param, 2, &param_len,   	// Parameters (1 word)
	NULL, 0, NULL);  		// Data (none)
  if (rc == NO_ERROR)
  {
    printf ("Turned off absolute mouse position.\n");
  }
  else
  {
    printf ("Error with ESX IOCTL call: %d\n", rc);
  }


  
  // We're done
  DosClose (mouseHandle);
  return 0;
}	


int main(int arg, char *argv[])
{
  printf ("CPU speed = %d.\n" , BackdoorIn (1));	
  printf ("VMWare backdoor version = %d.\n" , BackdoorIn (10));	

  set_ioctl();
  
  return 0;
}

