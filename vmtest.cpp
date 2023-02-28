#define INCL_DOSNLS
#define INCL_DOSERRORS

#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <uconv.h>

int main(int arg, char *argv[])
{
  char test_string[10];
  
  test_string[0] = 133;
  test_string[1] = 139;
  test_string[2] = 254;
  test_string[3] = 213;
  test_string[4] = 0;
  
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

  UconvObject from_ucs, to_ucs;
  int ret;
  UniChar *cpfromname = (UniChar *)L"IBM-850";
  UniChar *cptoname = (UniChar *)L" IBM-1208";
  ret = UniCreateUconvObject (cpfromname , &from_ucs);
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to create UniConv from object\n");
     return 1;
  }
  ret = UniCreateUconvObject (cptoname , &to_ucs);
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
  
  const char *s = (const char *)&ucsbuf[0];
  for (int i = 0; i < sizeof (ucsbuf); i++)
  {
    printf ("%02x ", *s);
    if ((i & 0xF) == 0xF)
    {
      printf ("\n");
    }
    s++;
  }
  printf ("\n");
  
  // Then go to UTF-8
  char outbuf[40] = {'\0'};
  ret = UniStrFromUcs (to_ucs, outbuf, ucsbuf, 40);
  if (ret != ULS_SUCCESS)
  {
     printf ("Failed to convert to UTF-8\n");
     return 1;
  }

  s = outbuf;
  for (int i = 0; i < sizeof (outbuf); i++)
  {
    printf ("%02x ", *s);
    if ((i & 0xF) == 0xF)
    {
      printf ("\n");
    }
    s++;
  }
  printf ("\n");
  
  
  return 0;
}

