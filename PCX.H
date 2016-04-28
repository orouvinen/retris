#ifndef PCX_H

#define PCX_PALETTE_ID   0x0c        /* ID just before the palette */
#define PCX_PALETTE_SIZE 768         /* Size of palette in bytes   */

#define ERROR   0                    /* Simple error code */
#define SUCCESS 1

typedef unsigned char  BYTE;
typedef unsigned short WORD;

/*
 * PCX header
 */
typedef struct {
	BYTE   manufacturer, version, encoding, bits_per_pixel;
	WORD   min_x, min_y, max_x, max_y;
	WORD   horiz_res, vert_res;
	BYTE   ega_palette[48];
	BYTE   reserved;
	BYTE   num_color_planes;
	WORD   bytes_per_line;
	WORD   palette_type;
	BYTE   unused[58];
} PCX_HEADER;

#include "retris.h"

int  get_pcx_palette (FILE *f, BYTE *palette, DATA_ENTRY *p);
int  get_pcx_header  (FILE *f, PCX_HEADER *hdr, DATA_ENTRY *p);
void decode_pcx      (PCX_HEADER *hdr, FILE *f, BYTE *dest, DATA_ENTRY *p);
int  pcx_decode_to_buffer_from_file (FILE *datafile, const char *pcx_file_name,
                                     BYTE *dest, BYTE *palette);

#endif /* PCX_H */
