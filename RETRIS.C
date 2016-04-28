/*
 * tetris.c: Retris game / menu / misc routines
 *
 * TODO:
 *
 *   - keyboard handling should be better
 *     (need to fall back to using custom keyboard interrupt handler for
 *      registering key presses)
 *
 *   - credit screen
 *
 *
 *   - the background flash effect that is displayed when four rows are dropped should
 *     not block the game. So, add a add_task() function to the timer module. This function
 *     is served with a pointer to a function that will be called in the timer handler, until
 *     the function returns 0 (or some other known value). We use that function to add a task that just
 *     fades the desired colors down in the timer handler. This way, it happens in the background.
 *
 *
 *   - Fix all functions that load resources so that they don't have to worry about
 *     DATA_ENTRY structures and stuff. Write a function load_resource(char *filename,char*dest)
 *     that will load a the file <filename> to buffer <dest>. Then change any functions
 *     that use fread() or whatever so that they read their data from the buffer.
 *
 *   - Write a wrapper to add_game_message() that splits the message string to multiple strings that
 *     are max 14 chars long.
 *
 *  CHANGES: (dd/mm/yy)
 *
 *  11/03/05      -  Changed the timer system to use clock() instead of hooking up the
 *                   timer interrupt because the game ran too slowly under WinXP.
 *
 *                -  Stripped out 'b' from version number. Current version is 1.50.
 *  =====================================================================================================
 *  16/03/05      -  Moved message box stuff to messbox.c and messbox.h
 *
 *
 */
#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "endgame.h"
#include "retris.h"
#include "gflib.h"
#include "graphics.h"
#include "keyboard.h"
#include "timer.h"
#include "gflib.h"
#include "score.h"
#include "pcx.h"
#include "messbox.h"

const char *program_name = "Retris";
const char *version      = "1.50";
const char *author       = "O. Rouvinen";

char version_string[64];

/***************************************************************************
 *
 * Externs
 *
 ***************************************************************************/

extern unsigned _stklen = 4096;  /* 4 kB stack (isn't this the default?) */

/*
 * graphics.c
 */
extern unsigned char *background;         /* Virtual screen 1 - holds all static content */
extern unsigned char *backscreen;         /* Virtual screen 2 - displays all changing graphics */
extern unsigned char *pcx_buffer;         /* Ptr to background PCX graphics*/
extern unsigned char far *physicalscreen; /* Pointer to physical VGA segment (0xa000) */

extern char *vgafont;                     /* Pointer to the ROM font */
extern char palbuf[256 * 3];              /* Palette buffer */

/***************************************************************************
 *
 *  Global variables
 *
 ***************************************************************************/

/*
 * Block advancing intervals for each level. 
 */
#define MAX_LEVEL 7 /* Number of actual levels - 1 because we start at level 0 */
const float level_delay[] = {1.00*CLOCKS_PER_SEC,0.85*CLOCKS_PER_SEC,0.70*CLOCKS_PER_SEC,
			     0.65*CLOCKS_PER_SEC,0.50*CLOCKS_PER_SEC,0.35*CLOCKS_PER_SEC,
			     0.20*CLOCKS_PER_SEC,0.10*CLOCKS_PER_SEC};

char player_name[MAX_NAME_LENGTH+1];


/*
 * Base color (palette index) for each shape.
 * last element is play area background block color
 */
const char shape_colors[8] = {17,20,23,26,29,32,35,38};

struct game_state tetris;            /* State of the game holder */
char play_area[20][10];              /* The tetris board (20x10 blocks) */

/* To solve SHAPE_WIDTH() and SHAPE_HEIGHT() */
const char shape_dimensions[7][2] = {{2,2},{3,2},{3,2},
                                     {3,2},{3,2},{3,2},{4,1}};

/* Scoring rules */
const char shape_score[7][4] = {{60,60,60,60},{60,70,60,70},{60,70,60,70},
                                {60,70,60,70},{60,70,60,70},{50,50,60,50},
                                {50,80,50,80}};
/*
 * Shape data. Each shape and its rotations are stored in 4x4 matrix.
 */
const char shapes[7][4][16] = {
      {{1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},
       {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0}},
      {{1,1,1,0,0,0,1,0,0,0,0,0,0,0,0,0},{0,1,0,0,0,1,0,0,1,1,0,0,0,0,0,0},
       {1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0},{1,1,0,0,1,0,0,0,1,0,0,0,0,0,0,0}},
      {{1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0},{1,1,0,0,0,1,0,0,0,1,0,0,0,0,0,0},
       {0,0,1,0,1,1,1,0,0,0,0,0,0,0,0,0},{1,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0}},
      {{1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0},{0,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0},
       {1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0},{0,1,0,0,1,1,0,0,1,0,0,0,0,0,0,0}},
      {{0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0},
       {0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0}},
      {{0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0},{1,0,0,0,1,1,0,0,1,0,0,0,0,0,0,0},
       {1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0},{0,1,0,0,1,1,0,0,0,1,0,0,0,0,0,0}},
      {{1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0},
       {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},{1,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0}}
};

FILE *datafile    = NULL;   /* Main.dat handle */
DATA_ENTRY *data  = NULL;   /* Array of entries in MAIN.DAT (the header) */
short num_entries = 0;      /* Number of elements in the array */


#undef DEBUG

int
main()
{
       /* The main menu items that will be passed to menu() */
       char *main_menu_items[] = {"NEW GAME", "HISCORES", "CREDITS", "QUIT"};

       if (!is_386_cpu()) {
              fprintf (stderr, "At least 80386 processor required\n");
              return (1);
       }
       fadedown();

       load_data();                     /* Load stuff from MAIN.DAT */
       srand (time ((time_t *) NULL));  /* Init. random number generator */

       /*
        * Do this after fadedown() so we don't end up with
        * a zero palette.
        */
       if (!graphics_init()) {
              perror ("graphics_init() failed");
              gf_setmode (0x03);
              return (1);
       }

       sprintf (version_string, "Retris (VER. %s)", version);
       /*
        * Main loop
        */
       for (;;) {
              char menu_choice;

              /*
               * Get background graphics from pcx_buffer
               */
              gf_move_block (background, pcx_buffer, SCREEN_SIZE);
              gf_fill_block (backscreen, 0, SCREEN_SIZE);

              /*
               * Draw the title box
               */
              draw_text_field (10, 10, 300, 40, background);
              blit_string (version_string, 12, 12, TEXTCOLOR, background);
              blit_string ("COPYRIGHT (C) O. ROUVINEN", 12, 18, TEXTCOLOR, background);

              menu_choice = menu ("Main Menu", 4, main_menu_items, 10, 55);

              if (menu_choice == 0) {
                     /*
                      * Start a new game
                      */
                     get_player_name();
                     game();
                    
                     /*
                      * Game over, check for hiscore
                      */
                     if (check_score (tetris.score)) {
                            DATA_ENTRY *p = find_data_entry ("hiscore.dat");
                            int sl_index = SCORE_LIST_SIZE - tetris.rank;

                            /*
                             * Get position in the score list
                             * and store the new entry there
                             */
                            roll_score_list (sl_index);
                            put_score_entry (player_name, tetris.score,
                                             sl_index);
                            write_score_list_file (datafile, p);
                     }
                     memset (player_name, 0, MAX_NAME_LENGTH+1);
              } else if (menu_choice == 1) {
                     /*
                      * Display hiscore list
                      */
                     display_hiscores (SCORE_LIST_SIZE - 1);
              } else if (menu_choice == 2) {
                     /*
                      * Display credit screen (not done)
                      */
                     credit_display();
              } else if (menu_choice == 3) {
                     /*
                      * Quit the game
                      */
                     fclose (datafile);
                     fadedown();
                     graphics_end();
                     free_score_list();
                     free (data);
                     break;
              }
       }
       printf ("\n%s\nCopyright (c) %s\n", program_name, author);
       printf ("Version %s (compiled %s %s)\n", version, __TIME__, __DATE__);
       return (0);
}


/*
 * menu(): displays a menu and gets user selection.
 *
 *         Args:
 *                   title     - Menu title string
 *                   item_text - Array of strings that contain the menu choices. Last item must be NULL ptr.
 *                   x, y      - menu position on screen
 */
int
menu (char *title, int num_items, char **item_text, int x, int y)
{
       register int i;
       const char BASE_GREEN = 32;    /* RGB (green) */
       unsigned char g = BASE_GREEN;  /* For palette changing */
       char d = -1;                   /* Palette fade direction modifier */
       char blink_color = MENU_ITEM_1_COLOR; /* Current choice color to fade */
       char choice = 0;               /* Menu choice */

       /*
        * Get background (title box)
        */
       gf_move_block (backscreen, background, SCREEN_SIZE);

       /*
        * Display all the menu items
        */
       for (i = 0; i < num_items; i++) {
              gf_set_palette_color (MENU_ITEM_1_COLOR + i, 0, BASE_GREEN, 0);
              blit_string (item_text[i], x, y, MENU_ITEM_1_COLOR + i,
                           backscreen);

              if (i == num_items - 2)
                     y += 16;  /* Separate "QUIT GAME" and "MAIN MENU"
                                * from rest of the items */
              else   y += 8;
       }
       gf_get_palette_color (MENU_ITEM_1_COLOR, NULL, &g, NULL); /* fix null */

       /*
        * Print menu title to the Game Title box
        */
       blit_string (title, 12, 40, TEXTCOLOR, backscreen);

       /*
        * Display the whole thing
        */
       gf_move_block (physicalscreen, backscreen, SCREEN_SIZE);

       for (;;) {
              int prev_color = blink_color;
              int scancode = getkey (NULL);

              /*
               * Update direction modifier (fade up-down)
               */
              if (g == 24 && d == -1)
                     d = 1;
              else if (g == 63 && d == 1)
                     d = -1;
              g += d;

              gf_waitretrace();
              gf_set_palette_color (blink_color, 0, g, 0);

              if (scancode == KB_DOWN) {
                     /*
                      * Select next item, or if this was the last,
                      * then select the first one
                      */
                     if (choice >= num_items - 1) {
                            choice      = 0;
                            blink_color = MENU_ITEM_1_COLOR;
                     } else {
                            choice++;
                            blink_color++;
                     }
              } else if (scancode == KB_UP) {
                     /*
                      * Select previous item, or if this was the first,
                      * then select the last one
                      */
                     if (choice <= 0) {
                            choice      = num_items - 1;
                            blink_color = MENU_ITEM_1_COLOR + num_items - 1;
                     } else {                                           
                            choice--;
                            blink_color--;
                     }
              }
              /*
               * Reset the color value of the previously selected
               * text to its original value
               */
              if (scancode == KB_UP || scancode == KB_DOWN) {
                     gf_set_palette_color (prev_color,
                                           0, BASE_GREEN, 0);
                     g = BASE_GREEN;
                     d = 1;
              }
              if (scancode == KB_RETURN || scancode == KB_SPACE)
                     break;
       }
       return (choice);
}


/*
 * The actual game routine
 */
void
game (void)
{
       static int need_to_tell_rank = TRUE;
       clock_t     t;
       float       next_update = clock() + level_delay[0];
       
       init_game_graphics();
       reset_game();
       next_shape();
       
       reset_messages();
       add_game_message ("Welcome, %s!", player_name);
       add_game_message ("---------------------------");
       add_game_message ("Current record is");
       add_game_message ("%lu points and held by", get_score (SCORE_LIST_SIZE-1));
       add_game_message ("%s.", get_name (SCORE_LIST_SIZE-1));
                                
       add_game_message ("Have a nice game!");
       add_game_message ("---------------------------");

       while (tetris.running) {
              unsigned char scancode, ch = 0;

              /*
               * Get background graphics
               */
              gf_move_block (backscreen, background, SCREEN_SIZE);

              /*
               * Check the rank if necessary
               */
              if (tetris.rank > 1)
                     check_rank();
              /*
               * Check if it's time to drop the shape
               */
              if ((t = clock ()) >= next_update) {	/* See if it's time to drop the shape */
		       next_update = t + level_delay[(tetris.level < MAX_LEVEL) ? tetris.level : MAX_LEVEL];
                     if (move_down ())
			       tetris.running = shape_stuck();
              }
              scancode = getkey (&ch);

              if (scancode) {
                     switch (scancode) {
                     case 0xe0: /* Extended scancode, need one more getkey() to get the actual value */
                            scancode = getkey (NULL);
                     case KB_P: {
                            /*
                             * Pause. Of course, we cannot make the time stop, but pretending to do so works
			     * quite nicely ;)
                             */
                            float time_left_before_update = next_update - clock ();
		              pause_game ();
		              next_update = clock() + time_left_before_update;
                            break;
                     }
                     case KB_DOWN:
                            if (move_down())
                                   tetris.running = shape_stuck();
                            else
                                   tetris.score += tetris.level + 1;
                            break;
                     case KB_LEFT:
                            move_side (LEFT);
                            break;
                     case KB_RIGHT:
                            move_side (RIGHT);
                            break;
                     case KB_UP:
                     case KB_COMMA:
                            rotate_shape (COUNTERCLOCKWISE);
                            break;
                     case KB_PERIOD:
                            rotate_shape (CLOCKWISE);
                            break;
                     case KB_ESC:
                            tetris.running = FALSE;
                            break;
                     }
              }
              /*
               * Draw shape in current position / rotation
               */
              draw_shape (tetris.shape, tetris.rot,
                          tetris.x, tetris.y, backscreen);

              /*
               * Copy it all to VGA memory
               */
              print_game_info (backscreen);

              if ((tetris.rank < 50 && tetris.rank > 10) && need_to_tell_rank) {
                     add_game_message ("Top 50 reached");
                     need_to_tell_rank = FALSE;
              } else if (tetris.rank <= 10 && tetris.rank > 1 &&
                         !need_to_tell_rank) {
                     add_game_message ("Top 10 reached");
                     need_to_tell_rank = TRUE;
              } else if (tetris.rank == 1 && need_to_tell_rank) {
                     add_game_message ("You have made the new");
                     add_game_message ("record! Congratulations!");
                     need_to_tell_rank = FALSE;
              }
              print_game_messages();
              gf_waitretrace();
              gf_move_block (physicalscreen, backscreen, SCREEN_SIZE);
       }
       
       add_game_message (" ");
       add_game_message ("     *** GAME OVER ***     ");
       add_game_message (" ");

       if (tetris.rank == RANK_NONE) {
              add_game_message ("You didn't make it to the");
              add_game_message ("hiscore list. Better luck");
              add_game_message ("next time..");
       } else {
              if (tetris.rank == 1)
                     add_game_message ("Again, congratulations!");
              add_game_message ("You reached the %d%s place", tetris.rank,
                                   (tetris.rank == 1 ||
                                   (tetris.rank != 11 && !((tetris.rank - 1) % 10))) ?
                                   "st" :
                                   (tetris.rank == 2 ||
                                   (tetris.rank != 12 && !((tetris.rank - 2) % 10))) ?
                                   "nd" :
                                   (tetris.rank == 3 ||
                                   (tetris.rank != 13 && !((tetris.rank - 3) % 10))) ?
                                   "rd" :
                                   "th");
              add_game_message ("in the hiscore list with");
              add_game_message ("a score of %lu.", tetris.score);
       }

       //gf_move_block (backscreen, background, SCREEN_SIZE);
       print_game_messages();
       gf_move_block (physicalscreen, backscreen, SCREEN_SIZE); 

       add_game_message (" ");
       add_game_message ("    *** PRESS <ESC> ***    ");

       gf_move_block (backscreen, background, SCREEN_SIZE);
       print_game_messages();
       gf_move_block (physicalscreen, backscreen, SCREEN_SIZE); 

       while (getkey (NULL) != 1) /* Wait for scancode of ESC */
              ; /* nothing */
}


void
reset_game (void)
{
       register int x, y;

       /*
        * Clear the play area
        */
       for (y = 0; y < 20; y++) {
              for (x = 0; x < 10; x++) {
                     draw_block (x, y, 7, background);
                     play_area[y][x] = 0;
              }
       }
      
       tetris.running    = TRUE;
       tetris.next_shape = rand() % 7;
       tetris.level      = 0;
       tetris.score      = 0;
       tetris.rows       = 0;
       tetris.rank       = RANK_NONE;
}


/*
 * Called when shape is stuck. Checks for full row(s), updates
 * score/level and then calls next_shape().
 *
 * Returns 1 if the shape is stuck and 0 otherwise.
 */
int
shape_stuck (void)
{
       register short y, row;
       int            num_rows   = 0;/* How many rows we got with this shape */
       long           base_score = 0;
       int            retval;

       /*
        * Somewhat brute force, but got tired of the flickering
        */
       mark_shape();        /* Set shape to memory */
       draw_shape (tetris.shape, tetris.rot,
                   tetris.x, tetris.y, background);
       
       gf_move_block (backscreen, background, SCREEN_SIZE);
       print_game_info (backscreen);
       print_game_messages();
       gf_waitretrace();
       gf_move_block (physicalscreen, backscreen, SCREEN_SIZE);

       for (y = tetris.y; y < tetris.y + SHAPE_HEIGHT(); y++) {
             if (full_row (y)) {
		     num_rows++;

                   /*
                    * Check if the level is changing
                    */
                   if (!(++tetris.rows % 10) && tetris.level < MAX_LEVEL) {
                         tetris.level++;
                         add_game_message ("Level %d...", tetris.level + 1);
                   }
                   /*
                    * NOTE: if we score in boundary of two levels, we
                    * score according to the higher of those two levels
                    */
                   base_score = shape_score[tetris.shape][tetris.rot] *
                                (tetris.level + 1);
		     tetris.score += base_score;

                   erase_row (y);
                   if (y) {
                         for (row = y; row > 0; row--)
                               shift_row (row);
		     }
	      }
       }
       if (num_rows) {
              tetris.score += (num_rows * num_rows) + (num_rows * base_score);
       
              if (num_rows == 4)
                     flash_background();
              
       }
       retval = next_shape();

       return (retval);
}


void
rotate_shape (int direction)
{
       short current_rotation = tetris.rot;

       tetris.rot += direction;
       /*
        * Boundary check
        */
       if (tetris.rot < 0)
              tetris.rot = 3;
       else if (tetris.rot > 3)
              tetris.rot = 0;
       
       /*
        * Does the shape fit in in the new rotation?
        */
       if (!shape_fits (tetris.x, tetris.y))
              tetris.rot = current_rotation; /* Fall back to the original */
}


int
move_down (void)
{
       if (!shape_fits (tetris.x, tetris.y + 1))
              return (1);  /* Cannot drop */
       tetris.y++;

       return (0);
}


void
move_side (int direction)
{
       if (!shape_fits (tetris.x + direction, tetris.y))
              return;
       tetris.x += direction;
}


/*
 * Checks for full row at position line (y)
 */
int
full_row (int y)
{
       register short x;
       
       for (x = 0; x < 10; x++) {
             if (!play_area[y][x])
                   return (FALSE);
       }
       return (TRUE);
}


/*
 * Removes row from screen and memory
 */
void
erase_row (int y)
{
       register short x;

       for (x = 0; x < 10; x++) {
              play_area[y][x] = FALSE;
              draw_block (x, y, 7, background);
       }
}


/*
 * Drops a row down by one line
 */
void
shift_row (int y)
{
       register short x;

       y--;
       for (x = 0; x < 10; x++) {
              if (play_area[y][x]) {
                     play_area[y + 1][x] = play_area[y][x];
                     draw_block (x, y + 1, play_area[y + 1][x] - 1,
                                 background);
                    
                     if (!play_area[y - 1][x] || !y) {
                            play_area[y][x] = 0;

                         /* Erase block from (x, y) */
                            draw_block (x, y, 7, background);
                     }
              }
       }
}


/*
 * Tests if shape is stuck
 */
int
shape_fits (int x, int y)
{
       register short i;
       short rx = x, ry = y;

       for (i = 0; i < 16; i++, rx++) {
              if (!(i & 3) && i) {
                     rx = x;
                     ry++;
              }
              if (shapes[tetris.shape][tetris.rot][i] &&
                  play_area[ry][rx]) {
                     return (FALSE);
              }
       }
       return (!(y + SHAPE_HEIGHT() > 20 || x + SHAPE_WIDTH() > 10 || x < 0));
}


/*
 * Marks a shape (and its color) to the play area.
 * Called from shape_stuck()
 */
void
mark_shape (void)
{
       register short i;
       short rx = tetris.x, ry = tetris.y;

       for (i = 0; i < 16; i++, rx++) {
             if (!(i & 3) && i) {
		     rx = tetris.x;
		     ry++;
	      }
             if (shapes[tetris.shape][tetris.rot][i])
                   play_area[ry][rx] = tetris.shape + 1;
       }
}

/*
 * Gets next shape from tetris.next_shape and puts a new one there
 */
int
next_shape (void)
{
       tetris.shape      = tetris.next_shape;
       tetris.next_shape = rand() % 7;
       tetris.rot        = 0;
       tetris.y          = 0;
       tetris.x          = 5 - (SHAPE_WIDTH() >> 1);  /* Center the shape */

       /*
        * Test for game over
        */
       if (!shape_fits (tetris.x, tetris.y))
              return (0);

       return (1);
}

void
check_rank (void) {
       /*
        * Index in score_list[] that we compare our score to
        */
       static int score_comp = 0;
	
       if (tetris.score <= get_score (0) || tetris.score == 0) {
              tetris.rank = RANK_NONE;
              score_comp = 0; /* So it's back to zero in the next game too */
       } else if (tetris.rank > 1) {
              while (tetris.score > get_score (score_comp)) {
                     tetris.rank--; /* Lower rank is better */

                     if (score_comp < SCORE_LIST_SIZE - 1)
                            score_comp++;

                     if (tetris.rank == 1)
                            break;
             }
       }
}

void
pause_game (void)
{
       register int i;
       char pause_palette[768];
       char ch;

       tetris.paused = TRUE;

       /*
        * Get palette
        */
       gf_move_block (pause_palette, palbuf, 768);

       for (i = 0; i < 768; i++)
              pause_palette[i] >>= 1;  /* Dimmed palette */
       gf_setpalette (pause_palette);

       for (;;) {
              getkey (&ch);
              if (ch == 'p' || ch == 'P')
                     break;
       }
       gf_setpalette (palbuf);

       tetris.paused = FALSE;
}


void
get_player_name (void)
{
       register int i;

       short x, y;

       gf_fill_block (backscreen, 0, SCREEN_SIZE);
       gf_fill_block (background, 0, SCREEN_SIZE);
       gf_fill_block (physicalscreen, 0, SCREEN_SIZE);
       /*
        * Set up a gradient palette
        */
       draw_text_field (0, 50, 319, 100, background);

       blit_string ("I KNOW THIS IS ANNOYING, BUT I NEED",
                     2, 52, TEXTCOLOR, background);
       blit_string ("TO KNOW YOUR NAME.",
                     2, 60, TEXTCOLOR, background);
       blit_string ("PLEASE TYPE IT IN SO WE CAN CONTINUE.",
                     2, 68, TEXTCOLOR, background);
       blit_string ("IF YOU DON'T ENTER A NAME, THEN THE DEFAULT",
                     2, 76, TEXTCOLOR, background);
       blit_string ("WILL BE USED.",
                     2, 84, TEXTCOLOR, background);

       blit_string ("THANKS,",
                     2, 116, TEXTCOLOR, background);
       blit_string ("RETRIS GAME EVENT ANNOUNCEMENT SYSTEM V1.0",
                     2, 124, TEXTCOLOR, background);

       blit_string (">", 2, 135, TEXTCOLOR, background);
       if (!get_string (player_name, 10, 135, MAX_NAME_LENGTH,
                        background))
              strcpy (player_name, "JRL"); /* The default */
       gf_waitretrace();
}


/*
 * get_string(): reads a string from keyboard echoing the input with the VGA font.
 *               Returns 0 if no string was given (just RETURN),
 *                       1 otherwise
 *
 * This should be made to separate function from the
 * gfx stuff that goes while the name is typed.
 *
 */
int
get_string (char *dest, int x, int y, int maxlength, char *buf)
{
       int cx = x, cy = y;      /* Cursor position */
       char ch = 0, scancode;   /* Keyboard variables */
       signed int n = 0;        /* Number of chars got */

       char rgb = 0;            /* Cursor r, g, b */
       char d = -1;             /* Direction modifier */

       for (;;) {
              register unsigned short i;

              /*
               * Cursor blinking (cursor RGB value)
               */
              if (rgb == 0 && d == -1)
                     d = 1;
              if (rgb == 63 && d == 1)
                     d = -1;
              rgb += d;

              scancode = getkey (&ch);
              if (ch) {
                     /*
                      * If an alphabet character or a space char, then get it
                      */
                     if ((isalpha (ch) || ch == ' ') && n < maxlength) {
                            cx += 6;             /* Update cursor position */
                            dest[n] = ch;        /* Save the character */
                            n++;                 /* Inc. number of chars got */
                            continue;
                     }
                     switch (scancode) {
                            case KB_RETURN:
                            if (!n)       /* Length == 0 */
                                   return (0); /* No name */
                            else {
                                   dest[n] = '\0'; /* NUL-terminate */
                                   return (1);
                            }
                            /* break; not needed / wanted */

                            case KB_BACKSPACE:
                            if (n) {
                                   n--;     /* Dec. number of chars */
                                   dest[n] = '\0';
                                   cx -= 6; /* Update cursor pos. (measured in pixels) */
                            }
                            break;
                            default: break;
                     }
              }
              /*
               * Copy the changes to the screen
               */
              gf_waitretrace();
              gf_set_palette_color (128, rgb, rgb, rgb);
              gf_move_block (physicalscreen, buf, SCREEN_SIZE);
              blit_string (player_name, 10, 135, 32, physicalscreen); 
              /* Draw cursor */
              gf_line (cx, cy, cx, cy + 6, 128, physicalscreen);
       }
}


int
load_data (void)
{
       DATA_ENTRY *entry;

       /*
        * open main.dat
        */
       if ((datafile = fopen ("main.dat", "r+b")) == NULL)
              fatal_error ("Cannot open MAIN.DAT");
       /*
        * read the number of entries stamp
        */
       fseek (datafile, -4, SEEK_END); /* 4 == sizeof(long) */
       fread (&num_entries, sizeof (long), 1, datafile);

       /*
        * allocate data header space
        */
       data = (DATA_ENTRY *) malloc (num_entries * sizeof (DATA_ENTRY));
       if (data == NULL)
              fatal_error ("load_data(): memory allocation error");

       /*
        * read the header
        */
       fseek (datafile, 0, SEEK_SET);
       fread (data, sizeof (DATA_ENTRY), num_entries, datafile);

       /*
        * Load hiscores
        */
       entry = find_data_entry ("hiscore.dat");
       read_score_list_file (datafile, entry);

       /*
        * Load font
        */
       entry = find_data_entry ("sf8x8.fnt");
       fseek (datafile, entry->data_offset, SEEK_SET);
       fread (vgafont, sizeof (char), entry->data_size, datafile);

       /*
        * Load initial palette buffer
        */
       entry = find_data_entry ("palette.bin");
       fseek (datafile, entry->data_offset, SEEK_SET);
       fread (palbuf, sizeof (char), entry->data_size, datafile);

       return (1);
}

/*
 * Searches for an data header entry
 */
DATA_ENTRY *
find_data_entry (const char *filename)
{
       register int i;

       for (i = 0; i < num_entries; i++) {
              if (strcmp (filename, data[i].filename) == 0)
                     return (&data[i]);
       }
       return ((DATA_ENTRY *) NULL);
}



/*
 * Checks for presence of at least 386 processor.
 *
 * Returns 0 if processor is below 386, and 1 otherwise.
 */
int
is_386_cpu (void)
{
       asm {
              pushf
              pop   ax
              or    ah,0x40     /* try to set NT flag */
              push  ax
              popf
              pushf
              pop   ax
              and   ah,0x40
              jz    not_386
       }
       return (1);
not_386:
       return (0);
}


void
credit_display (void)
{
       int i, x = -2;
       char *title = "RETRIS";

       char *mess1 = "...Credit screen under construction...";

#define CENTER_TEXT(s) (SCREEN_WIDTH>>1)-((FONT_WIDTH*strlen((s)))>>1)

       gf_move_block (background, pcx_buffer, SCREEN_WIDTH * SCREEN_HEIGHT);

       for (i = 0; i < strlen (title); i++, x += 6)
              draw_block_char (title[i], x, 3, background);
       
       blit_string (mess1, CENTER_TEXT(mess1), 100, TEXTCOLOR, background);
       blit_string (mess2, CENTER_TEXT(mess2), 110, TEXTCOLOR, background);
       
       gf_move_block (physicalscreen, background, SCREEN_WIDTH * SCREEN_HEIGHT);

       getch();
}
