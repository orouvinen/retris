#include "keyboard.h"

#ifndef NULL
#define NULL (void *)0
#endif /* NULL */

int
getkey (char *ascii)
{
       char scancode = 0;
       char key;
       asm {
              mov   ah,1
              int   16h
              jz    no_key
       }
       asm {
              xor   ah,ah
              int   16h
              mov   scancode,ah
              mov   key,al
              jmp   got_key
       }
no_key:
       if (ascii != NULL)
              *ascii = 0;
       goto quit;
got_key:
       if (ascii != NULL)
              *ascii = key;
quit:  return scancode;

}
