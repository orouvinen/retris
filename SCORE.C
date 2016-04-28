/*
 * score.c: hiscore list handling
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "score.h"
#include "endgame.h"
#include "retris.h"
#include "pcx.h"
#include "gflib.h"
#include "graphics.h"
#include "keyboard.h"

/* Externs */
extern unsigned char far *pcx_buffer, *background, *backscreen, *physicalscreen;

extern struct game_state tetris;


/* Pointer to the hiscore list */
SCORE_ENTRY *scores = NULL;

/*
 * allocate_score_list(): allocates memory for the hiscore list.
 *                        Returns 1 if successful, 0 otherwise
 */
int
allocate_score_list (void)
{
       register int i;

       if (scores != NULL)
              return 1;     /* Already allocated */

       scores = (SCORE_ENTRY *) malloc (SCORE_LIST_SIZE *
                                        sizeof (SCORE_ENTRY));
       if (scores != NULL) {
              for (i = 0; i < SCORE_LIST_SIZE; i++) {
                     scores[i].player_name =
                                   (char *) malloc ((MAX_NAME_LENGTH *
                                                     sizeof (char)) + 1);
                     if (scores[i].player_name == NULL)
                            return 0;  /* Failure */

                     memset (scores[i].player_name, 0, MAX_NAME_LENGTH + 1);
              }
              return 1; /* Success */
       }
       return 0; /* Failure */
}


void
free_score_list (void)
{
       register int i;

       for (i = 0; i < SCORE_LIST_SIZE; i++) {
              if (scores[i].player_name != NULL)
                     free (scores[i].player_name);
       }
       free (scores);
}


int
read_score_list_file (FILE *datafile, DATA_ENTRY *p)
{
       register int i;

       if (scores == NULL) {
              if (!allocate_score_list())
                     fatal_error ("read_score_list_file(): mem. alloc. error");
       }
       /*
        * Read in the scores (slow);
        */
       fseek (datafile, p->data_offset, SEEK_SET);

       for (i = 0; i < SCORE_LIST_SIZE; i++) {
              char *p;
              
              fread (&scores[i].score, sizeof (unsigned long), 1, datafile);
              fread (scores[i].player_name, sizeof (char), MAX_NAME_LENGTH+1,
                     datafile);
             
              /* Trim out the trailing spaces in the name string */
              p = strchr (scores[i].player_name, ' ');
              if (p != NULL)
                     *p = '\0';
                     
       }
       return 1;  /* Success */
}


int
write_score_list_file (FILE *datafile, DATA_ENTRY *p)
{
       register int i;

       fseek (datafile, p->data_offset, SEEK_SET);

       for (i = 0; i < SCORE_LIST_SIZE; i++) {
              fwrite (&scores[i].score, 4, 1, datafile);
              fprintf (datafile, "%-8s", scores[i].player_name);
              fprintf (datafile, "%c", 0); /* NUL-terminate */
       }
       return 1; /* Success */
}


/*
 * Checks if player got a hiscore
 */
int
check_score (unsigned long score)
{
       if (scores == NULL)
              fatal_error ("check_score(): score table not allocated\n");
       
       /*
        * Return TRUE if player got a hiscore, FALSE otherwise.
        * Assumes that the score list is sorted, because it always is :)
        */
       return (score > scores[0].score && score != 0);
}


/*
 * Makes room for a new hiscore record.
 * I was too lazy to write a quick sort routine, so this came up.
 * And don't you think that it's a just another brute force solution to
 * first sort the list and then put the new record to the lowest place
 * in it, and then sort the list again to put it in order again...
 */
void
roll_score_list (int index)
{
       register int i;

       for (i = 0; i < index; i++) {
              scores[i].score = scores[i + 1].score;
              strcpy (scores[i].player_name, scores[i + 1].player_name);
       }
}


/*
 * !!! This probably should be in graphics.c
 */
void
display_hiscores (int start_from)
{
       int i;
       int x = 90, y;
       int v = 0;           /* For color gradient */
       int rank;

       gf_fill_block (backscreen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
       gf_move_block (background, pcx_buffer, SCREEN_WIDTH * SCREEN_HEIGHT);

       blit_string ("TOP 100 SCORES", 110,  0,
                    HISCORE_COLOR_START - 1, background);
       blit_string ("USE ARROW KEYS AND PGUP / PGDN TO SCROLL", 40, 190,
                    HISCORE_COLOR_START - 1, background);
       blit_string ("NAME",             x, 20,
                    HISCORE_COLOR_START - 1, background);
       blit_string ("SCORE",      x + 100, 20,
                    HISCORE_COLOR_START - 1, background);

       for (;;) {
              char ch, scancode;

              gf_move_block (backscreen, background, 320 * 200);

              v = 0;   /* Color number for green gradient */
              y = 30;
              rank = SCORE_LIST_SIZE - start_from;

              for (i = start_from; i > start_from - SCORE_PAGE_SIZE; i--) {
                     char tmp[10];
                     /*
                      * Print rank
                      */
                     sprintf (tmp, "%2.d", rank);
                     blit_string (tmp, x - 32, y, HISCORE_COLOR_START + v,
                                  backscreen);

                     /*
                      * Print player name
                      */
                     blit_string (scores[i].player_name, x, y,
                                  HISCORE_COLOR_START + v, backscreen);
                     /*
                      * Print score
                      */
                     ltoa (scores[i].score, tmp, DECIMAL);
                     blit_string (tmp, x + 100, y, HISCORE_COLOR_START + v,
                                  backscreen);

                     y += 7;   /* Next row   */
                     v++;      /* Next color */
                     rank++;   /* Next rank  */
              }
              /*
               * Handle pressed keys
               */
              scancode = getkey (&ch);
              if (scancode == KB_ESC)
                     break;
              
              switch (scancode) {
              case KB_DOWN:
                     if (start_from > SCORE_PAGE_SIZE - 1)
                                   start_from--;
              break;

              case KB_UP:
                     if (scancode == KB_UP && start_from < SCORE_LIST_SIZE - 1)
                            start_from++;
              break;

              case KB_PGDN:
                     if (scancode == KB_PGDN) {
                            if (start_from >= 39) /* Can scroll whole page */
                                   start_from -= SCORE_PAGE_SIZE;
                            else /* Cannot, go to the end of the list */
                                   start_from = SCORE_PAGE_SIZE - 1;
                     }
              break;

              case KB_PGUP:
                     if (scancode == KB_PGUP) {
                            if (start_from < SCORE_LIST_SIZE - SCORE_PAGE_SIZE)
                                   start_from += SCORE_PAGE_SIZE;
                            else
                                   start_from = SCORE_LIST_SIZE - 1;
                     }
              break;

              default:
              break;
              }
              gf_waitretrace();
              gf_move_block (physicalscreen, backscreen, 320 * 200);
       }
}

/*
 * Interface functions
 */
void put_score_entry (const char *name, unsigned long score, int pos)
{
       strncpy (scores[pos].player_name, name, MAX_NAME_LENGTH);
       scores[pos].score = score;
}


unsigned long get_score (int pos)
{
       return (scores[pos].score);
}


char *get_name (int pos)
{
       return (scores[pos].player_name);
}

