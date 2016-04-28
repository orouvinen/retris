#ifndef GFLIB_H
#define GFLIB_H

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 200

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#define gf_setpalette(p) gf_setpalette_colors((p), 0, 256);

/*
 * Prototypes
 */
void gf_setmode (BYTE mode);
void gf_waitretrace (void);
void gf_get_palette_color (BYTE color, char far *r, char far *g, char far *b);
void gf_set_palette_color (BYTE color, char r, char g, char b);
void gf_setpalette_colors (char far *palbuf, short start, short num);
void gf_pixel (short x, short y, BYTE color, char far *buf);
void gf_line (short x, short y, short x2, short y2, BYTE color, char far *buf);
void gf_hline (short y, short x, short x2, BYTE color, char far *buf);
void gf_move_block (char far *dest, char far *src, unsigned short nbytes);
void gf_fill_block (char far *block, char value, unsigned short nbytes);

#endif /* GFLIB_H */
