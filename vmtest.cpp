#define INCL_DOSNLS
#define INCL_DOSERRORS

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uconv.h>

#include "backdoor.h" 

int main(int arg, char *argv[])
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

