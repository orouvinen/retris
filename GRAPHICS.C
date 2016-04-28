#include <dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "endgame.h"
#include "fontdata.h"
#include "gflib.h"
#include "graphics.h"
#include "retris.h"
#include "score.h"  /* SCORE_PAGE_SIZE in palette_init() */
#include "pcx.h"

#pragma inline

/*
 * Externs:
 */

/* tetris.c */
extern FILE *datafile;
extern const char shape_colors[7];
extern const char shapes[7][4][16];
extern struct game_state tetris;
extern char play_area[20][10];

extern char player_name[MAX_NAME_LENGTH+1];

/*
 * Global variables
 */

/*
 * Used to speed drawing pixels.
 * Every element has a value SCREEN_WIDTH * n, where n < SCREEN_HEIGHT
 */
unsigned short line_offset[SCREEN_HEIGHT];

char palbuf[256 * 3];                  /* Palette */
unsigned char *background;             /* vbuf 1 (static elements)  */
unsigned char *backscreen;             /* vbuf 2 (dynamic elements) */
unsigned char *pcx_buffer;             /* pointer to uncompressed PCX data */
unsigned char far *physicalscreen;     /* Pointer to VGA memory */

/*
 * Block bitmap
 */
char blockmap[BLOCK_HEIGHT][BLOCK_WIDTH] = {{-1,-1,-1,-1,-1,-1,-1,-1,-1},
                                            {-1, 0, 0, 0, 0, 0, 0, 0, 1},
                                            {-1, 0, 0 ,0, 0, 0, 0, 0, 1},
                                            {-1, 0, 0, 0, 0, 0, 0, 0, 1},
                                            {-1, 0, 0, 0, 0, 0, 0, 0, 1},
                                            {-1, 0, 0, 0, 0, 0, 0, 0, 1},
                                            {-1, 0, 0, 0, 0, 0, 0, 0, 1},
                                            {-1, 0, 0, 0, 0, 0, 0, 0, 1},
                                            {-1, 1, 1, 1, 1, 1, 1, 1, 1}};

/*
 * Pointer to font data
 */
unsigned char *vgafont = fontdata_8x8;

/*
 * Graphics init.
 * Returns 0 in case of memory allocation error.
 */
int
graphics_init (void)
{
       unsigned short i;
       FILE *f;
       
       /* Pre-calc line offsets (i*320) where i is something from 0 to 199. */
       for (i = 0; i < SCREEN_HEIGHT; i++)
              line_offset[i] = i * SCREEN_WIDTH; 

       physicalscreen = (unsigned char far *) 0xa0000000;  /* VGA segment */
       /*
        * Allocate memory for two 320x200 sized virtual screens
        * and a background picture buffer
        */
       background = (unsigned char *) malloc (65535);  /* 65535 is max */
       backscreen = (unsigned char *) malloc (65535);
       pcx_buffer = (unsigned char *) malloc (SCREEN_SIZE);
       if (background == NULL || backscreen == NULL || pcx_buffer == NULL)
              return (0);

       gf_fill_block (backscreen, 0, 64000);

       /*
        * Load background graphics
        */

       if (pcx_decode_to_buffer_from_file (datafile, "backgrnd.pcx",
                                           pcx_buffer, NULL) == ERROR)
              fatal_error ("Failed to load backgrnd.pcx");
       gf_setmode (0x13);
       palette_init();

       return (1);
}

/*
 * All palette values that are not to be modified later on in the program
 */
void
palette_init (void)
{
       register short i;
       BYTE color = 255;
       /*
        * palbuf[] previously loaded from MAIN.DAT
        */
       for (i = 0; i < 16; i++, color--) {
              float value =(float) ((palbuf[i*3]   * 0.2126) +
                                    (palbuf[i*3+1] * 0.7152) +
                                    (palbuf[i*3+2] * 0.0722));
              set_palbuf_rgb (color, (long)value >> 2,
                                     (long)value >> 1,
                                     (long)value >> 2, palbuf);
       }
       /* Set text color (239) RGB values */
       set_palbuf_rgb (TEXTCOLOR, 16, 48, 0, palbuf);
       /* Text field border colors */
       set_palbuf_rgb (TEXT_FIELD_LIGHT, 0, 52, 0, palbuf);
       set_palbuf_rgb (TEXT_FIELD_DARK,  0, 32, 0, palbuf);

       for (i = 0, color = 220; i < 14; color++, i++)
              set_palbuf_rgb (color, 0, 63 - (3 * i), 0, palbuf);

       gf_setpalette (palbuf); /* Write the whole palette */
}

/*
 * Called before starting a new game.
 * Do any game screen specific graphic initialization here
 */
void
init_game_graphics (void)
{
       register int i;

       /*
        * Set up virtual screens
        */
       pcx_decode_to_buffer_from_file (datafile, "backgrnd.pcx",
                                       background, NULL);
       gf_fill_block (backscreen, 0, 320 * 200);

       /*
        * Draw game information text box
        */
       draw_text_field (INFO_BOX_X, INFO_BOX_Y, 168, 60, background);
       blit_string ("Score:", INFO_BOX_X + 2, INFO_BOX_Y + 2, TEXTCOLOR, background);
       blit_string ("Level:", INFO_BOX_X + 2, INFO_BOX_Y + 10, TEXTCOLOR, background);
       blit_string ("Rank :", INFO_BOX_X + 2, INFO_BOX_Y + 18, TEXTCOLOR, background);
       blit_string ("Next :", INFO_BOX_X + 2, INFO_BOX_Y + 26, TEXTCOLOR, background);

       /*
        * Draw the message area
        */
       draw_text_field (INFO_BOX_X, INFO_BOX_Y + 75, 168, 100, background);
}



void
draw_box (int x, int y, int width, int height, int color, unsigned char far *buf)
{
       int sy;
       for (sy = y; sy < y + height; sy++)
              gf_hline (sy, x, x + width - 1, color, buf);
}

void
draw_block (int x, int y, int blocknum, unsigned char *buf)
{
       register int rx, ry;

       /*
        * Calculate coordinates of the block.
        * The parameters x and y are coordinates in the play area
        * (0 <= x < 10) and
        * (0 <= y < 20)
        */
       short sx = PLAY_AREA_MIN_X + (x * BLOCK_WIDTH);
       short sy = PLAY_AREA_MIN_Y + (y * BLOCK_HEIGHT);

       for (ry = sy; ry < sy + BLOCK_HEIGHT; ry++) {
              for (rx = sx; rx < sx + BLOCK_WIDTH; rx++) {
                     drawpixel (rx, ry,
                                shape_colors[blocknum] +
                                blockmap[ry - sy][rx - sx],
                                buf);
              }
       }
}

/*
 * Draws a shape (int shape) in rotation (int rot) to (int x),(int y)
 */
void
draw_shape (int shape, int rot, int x, int y, unsigned char *buf)
{
       register int     i;
       register int     rx = x, ry = y;

       for (i = 0; i < 16; i++, rx++) {
              if (!(i & 3) && i) {   /* i&3 as i%4 */
                     rx = x;
                     ry++;
              }
              if (shapes[shape][rot][i])
                     draw_block (rx, ry, shape, buf);
       }
}


void
graphics_end (void)
{
       gf_setmode (0x03);
       if (backscreen != NULL) free (backscreen);
       if (background != NULL) free (background);
       if (pcx_buffer != NULL) free (pcx_buffer);
}


void
draw_text_field (int x, int y, int width, int height,
                 unsigned char far *buf)
{
       int sx, sy;

       gf_line (x + 1, y + height, x + width, y + height, TEXT_FIELD_LIGHT, buf);
       gf_line (x + width, y + height, x + width, y + 1, TEXT_FIELD_LIGHT, buf);

       gf_line (x, y, x, y + height - 1, TEXT_FIELD_DARK, buf);
       gf_line (x, y, x + width - 1, y, TEXT_FIELD_DARK, buf);

       for (sy = y + 1; sy < y + height; sy++) {
              for (sx = x + 1; sx < x + width; sx++) {
                     buf[line_offset[sy] + sx] =
                         pcx_buffer[line_offset[sy] + sx] + 240;
              }
       }
}


/*
 * Blitter functions
 */
void
blit_char (unsigned char c, int x, int y, unsigned char color,
           unsigned char far *ptr)
{
       int i;
       unsigned short offset = line_offset[y] + x;
       unsigned short char_offset = (c << 3);

       for (i = 0; i < 8; i++) {
              register short bitmask;
                                                
              for (bitmask = 0x80; bitmask; bitmask >>= 1) {
                     if (vgafont[char_offset + i] & bitmask)
                            ptr[offset] = color;
                     offset++;
              }
              offset += (SCREEN_WIDTH - 8);  /* 8 == font width */
       }
}

void
blit_string (char *s, int x, int y, unsigned char color,
             unsigned char far *ptr)
{
       while (*s) {
              if (x >= SCREEN_WIDTH)
                     break;
              blit_char (*s++, x, y, color, ptr);
              x += 6;   /* visible font with */
       }
}

/*
 * draw_block_char(): draws a character that is composed of tetris blocks :)
 */
void
draw_block_char (char c, int x, int y, unsigned char far *ptr)
{
       int i;
       unsigned short char_offset = (c << 3);
       int sx;

       for (i = 0; i < 8; i++) {
              register short bitmask;

              sx = x;
              for (bitmask = 0x80; bitmask; bitmask >>= 1) {
                     if (vgafont[char_offset + i] & bitmask)
                            draw_block (sx, y, 1, ptr);
                     sx++;
              }
              y++;
       }
       
}


void
set_palbuf_rgb (unsigned char i, char r, char g, char b, char *palette)
{
       short index = (i << 1) + i; /* i MUL 3 */

       palette[index] = r;
       palette[index + 1] = g;
       palette[index + 2] = b;
}

void
fadedown (void)
{
       register int i, j;
       for (i = 0; i < 64; i++) {
              unsigned char r, g, b;

              for (j = 0; j < 256; j++) {
                     gf_get_palette_color (j, &r, &g, &b);

                     if (r) r--;
                     if (g) g--;
                     if (b) b--;

                     set_palbuf_rgb (j, r, g, b, palbuf);
              }
              gf_waitretrace();
              gf_setpalette (palbuf);
       }
}

/*
 * Flashes the background blocks,
 * called when four rows are dropped
 */
void
flash_background (void)
{
       register int i, j, k;
       char r, g, b, R[3], G[3], B[3];

       R[0] = G[0] = B[0] = 23;
       R[1] = G[1] = B[1] = 43;
       R[2] = G[2] = B[2] = 63;

       for (j = 0; j < 63; j++) {
              gf_waitretrace();
              k = 0;

             /* outport (0x3d4, (scr_offset << 8) | 8); */

              for (i = shape_colors[7] - 1; i < shape_colors[7] + 2; i++) {
                     int index = (i << 1) + i; /* i MUL 3 */
                     r = palbuf[index];
                     g = palbuf[index + 1];
                     b = palbuf[index + 2];

                     R[k] = (R[k] < r) ? R[k] + 1 : (R[k] > r) ? R[k] - 1 : r;
                     G[k] = (G[k] < g) ? G[k] + 1 : (G[k] > g) ? G[k] - 1 : g;
                     B[k] = (B[k] < b) ? B[k] + 1 : (B[k] > b) ? B[k] - 1 : b;

                     gf_set_palette_color (i, R[k], G[k], B[k]);
                     k++;
              }
       }
       /* outport (0x3d4, 8); */
}

void
print_game_info (unsigned char far *buf)
{
       char score_string[10], level_string[3];
       char rank_string[6];

       /*
        * Convert the needed integers to printable strings
        */
       ltoa (tetris.score, score_string, DECIMAL);
       itoa (tetris.level, level_string, DECIMAL);

       /*
        * Get a correct suffix (st/nd/rd/th)
        * for the ranking string.
        */
       sprintf (rank_string, "%d%s", tetris.rank,
                (tetris.rank == 1 || (tetris.rank != 11 &&
                                      !((tetris.rank - 1) % 10))) ? "st" :
                (tetris.rank == 2 || (tetris.rank != 12 &&
                                      !((tetris.rank - 2) % 10))) ? "nd" :
                (tetris.rank == 3 || (tetris.rank != 13 &&
                                      !((tetris.rank - 3) % 10))) ? "rd" :
                "th");

       /*
        * Print current score/level/ranking
        */
       blit_string (score_string, INFO_BOX_X + 45, INFO_BOX_Y + 2,
                    DYNAMIC_TEXT_COLOR, buf);
       blit_string (level_string, INFO_BOX_X + 45, INFO_BOX_Y + 10,
                    DYNAMIC_TEXT_COLOR, buf);
       
       blit_string ((tetris.rank == RANK_NONE) ? "Not ranked" : rank_string,
                    INFO_BOX_X + 45,
                    INFO_BOX_Y + 18,
                    DYNAMIC_TEXT_COLOR,
                    buf);
       /*
        * Display the shape that's coming after this shape
        */
       draw_shape (tetris.next_shape, 0, 15, 4, buf);
}
