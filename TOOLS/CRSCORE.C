/*
 * Creates a hiscore list
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../score.h"

int main()
{
       FILE *f;
       struct hiscore_entry scores[SCORE_LIST_SIZE];
       register int i;

       for (i = 0; i < SCORE_LIST_SIZE; i++) {
              scores[i].score = 0;
              scores[i].player_name = malloc (MAX_NAME_LENGTH + 1);
              sprintf (scores[i].player_name, "%-8s", "Nobody");
              fprintf (f, "%c", 0);  /* NUL char */
       }
       
       f = fopen ("hiscore.dat", "wb");
       if (!f) {
              perror ("");
              return 1;
       }
       for (i = 0; i < SCORE_LIST_SIZE; i++) {
              fwrite (&(scores[i].score), sizeof (long), 1, f);
              fprintf (f, "%s", scores[i].player_name);
              fprintf (f, "%c", (char) 0);
       }
       fclose (f);
       return (0);
}
