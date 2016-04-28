/*
 * PCX.C: Routines for decoding version 5 RLE encoded PCX pictures.
 *        (Version 5: 320x200x256)
 *
 * Copyright (c) 2001 O. Rouvinen
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcx.h"
#include "retris.h"

/*
 * Decodes an RLE encoded PCX picture from a data file (i.e. MAIN.DAT),
 * to a buffer (dest) and stores palette of the PCX to (palette)
 */
int
pcx_decode_to_buffer_from_file (FILE *datafile, const char *pcx_file_name,
                                BYTE *dest, BYTE *palette) {
       PCX_HEADER header;
       DATA_ENTRY *entry = find_data_entry (pcx_file_name);
       register int i;

       if (entry == NULL)
              return ERROR;

       fseek (datafile, entry->data_offset, SEEK_SET);

       if (get_pcx_header (datafile, &header, entry) == ERROR)
              return ERROR;

       if (palette != NULL)
              get_pcx_palette (datafile, palette, entry);

       decode_pcx (&header, datafile, dest, entry);
       return SUCCESS;
}

/*
 * Function: decode_pcx(): decodes RLE encoded PCX data to memory.
 *
 *   Arguments:
 *   hdr  - pointer to valid PCX_HEADER block filled byt get_pcx_header()
 *   f    - file to read the data from (the PCX file)
 *   dest - pointer to memory where to decode the data
 *
 *   Return value: none
 */
void
decode_pcx (PCX_HEADER *hdr, FILE *f, BYTE *dest, DATA_ENTRY *p)
{
       int x, y, runcount;
       BYTE data;
       
       fseek (f, p->data_offset + 128, SEEK_SET);    /* Skip header */
       
       for (y = 0; y < hdr->max_y - hdr->min_y + 1; y++) {
              WORD offset = (y << 8) + (y << 6);    /* 320 * y */
              x = 0;
              while (x < hdr->bytes_per_line) {
                     data = fgetc (f);
                     if ((data & 0xc0) == 0xc0) {
                            register int i;
       
                            runcount = data & 0x3f;
                            data = fgetc (f);
       
                            for (i = 0; i < runcount; i++, x++)
                                   dest[offset++] = data;
                     } else {
                            dest[offset++] = data;
                            x++;
                     }
              }
       }
}

/*
 * Function: get_pcx_header(): reads a PCX header into a PCX_HEADER struct.
 *
 *   Arguments:
 *     f      - the PCX file
 *     header - pointer to a PCX_HEADER structure
 *
 *     Return value:  ERROR in case of error,
 *                    SUCCESS if no errors occurred
 */
int
get_pcx_header (FILE *f, PCX_HEADER *header, DATA_ENTRY *p)
{
       fseek (f, p->data_offset, SEEK_SET);
       
       if (fread (header, sizeof (BYTE), 128, f) != 128) {
              return ERROR;
       }
       if (header->version != 5) {
              return ERROR;
       }
       return SUCCESS;
}

/*
 * Function: get_pcx_palette(): reads a PCX palette
 *
 *   Arguments:
 *     f       - the PCX file
 *     palette - pointer to at least 768 byte memory block where the palette
 *               data will be stored
 *
 *     Return value: ERROR   in case of error,
 *                   SUCCESS if no error occurred.
 */
int
get_pcx_palette (FILE *f, BYTE *palette, DATA_ENTRY *p)
{
       fseek (f, (p->data_offset + p->data_size) -
                 (PCX_PALETTE_SIZE + 1), SEEK_SET);
       if (fgetc (f) != PCX_PALETTE_ID)
              return ERROR;
       
       if (fread (palette, sizeof (BYTE), 768, f) != 768)
              return ERROR;

       return SUCCESS;
}
