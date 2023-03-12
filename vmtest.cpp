#define INCL_DOSNLS
#define INCL_DOSERRORS

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <uconv.h>

#include "log.h" 

int main(int arg, char *argv[])
{
  char test_string[10];
  
  test_string[0] = 133;
  test_string[1] = 139;
  test_string[2] = 254;
  test_string[3] = 213; // euro
  test_string[4] = 200;
  test_string[5] = 0;
  
  printf ("Input = %s\n", test_string);
  
  ULONG code_pages[20];
  ULONG cp_len, cp_count;
  
  // Let's do some codepage conversion magic
  cp_len = 20 * sizeof (ULONG);
  DosQueryCp (cp_len, code_pages, &cp_count);
  cp_count /= sizeof (ULONG);
  printf ("There are %d codepages: " , cp_count);
  for (int i = 0; i < cp_count; i++)
  {
     printf ("%d ", code_pages[i]);
  }
  printf ("\n");

  UconvObject from_ucs, utf8_cs;
  int ret;
  UniChar *cpfromname = (UniChar *)L"IBM-850";
  UniChar *cptoname = (UniChar *)L" IBM-1208";
  ret = UniCreateUconvObject (cpfromname , &from_ucs);
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to create UniConv from object\n");
     return 1;
  }
  ret = UniCreateUconvObject (cptoname , &utf8_cs);
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to create UniConv to object\n");
     return 1;
  }

  // Do a conversion to UCS-2
  UniChar ucsbuf[20]= {0};
  ret = UniStrToUcs (from_ucs, ucsbuf, test_string, 20);
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to convert to UCS\n");
     return 1;
  }
  loghex ((const char *)ucsbuf, sizeof (ucsbuf));
  
  // Then go to UTF-8
  char outbuf[40] = {'\0'};
  ret = UniStrFromUcs (utf8_cs, outbuf, ucsbuf, 40);
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to convert to UTF-8\n");
     return 1;
  }
  loghex (outbuf, sizeof (outbuf));
  

  // And back to UCS, just for fun
  ret = UniStrToUcs (utf8_cs, ucsbuf, outbuf, strlen (outbuf));
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to convert to UCS again: 0x%x\n", ret);
     return 1;
  }
  loghex ((const char *)ucsbuf, sizeof (ucsbuf));

  
  return 0;
}

