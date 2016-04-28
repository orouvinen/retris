#include <stdarg.h>
#include <stdio.h>

#include "graphics.h"
#include "messbox.h"

extern unsigned char far *backscreen;

/*
 * Holds messages to be printed on the message area
 */
char messages[14][MAX_MESSAGE_LENGTH+1];
char first_message = 0, new_message = 0;

void
print_game_messages (void)
{
       register int i;
       signed int mess = first_message;
       unsigned char color = 233;

       for (i = 0; i < 14; i++) {
              /* MAX 14 lines of 27 chars text in the message box */
              blit_string (messages[mess],
                           INFO_BOX_X + 2, (INFO_BOX_Y + 77) + (i*7),
                           color, backscreen);

              color--;
              if (++mess >= 14)
                     mess = 0;
       }
}

/*
 * Adds a new message to the messages array.
 */ 
void
add_game_message (char *message, ...)
{
       va_list arg_ptr;
       
       va_start (arg_ptr, message);
       vsprintf (messages[new_message], message, arg_ptr);
       va_end (arg_ptr);
       
       if (++new_message >= 14)
              new_message = 0;

       if (++first_message >= 14)
              first_message = 0;
}

void reset_messages (void)
{
       int i;
       
       for (i = 0; i < 14; i++)
              messages[i][0] = '\0';
       first_message = new_message = 0;
}