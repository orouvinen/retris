#include <stdio.h>
#include <stdlib.h>

#include "endgame.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"

extern FILE *datafile;

void
fatal_error (const char *error_message)
{
       graphics_end();
       restore_timer();

       if (datafile != NULL) {
              fflush (datafile); /* No, I don't know what "neurotic" means ;) */
              fclose (datafile);
       }
       fprintf (stderr, "fatal_error(): %s", error_message);
       perror("");

       exit (1);
}

