/*
 * Creates MAIN.DAT used by retris.
 *
 * Format of main.dat:
 *
 * Array of DATA_ENTRY structures which describe the data in main.dat.
 * Four bytes (a dword) found at the end of main.dat tells how many
 * DATA_ENTRY structures there are at the beginning of the file.
 * After these entries, comes the data of files written to main.dat
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
       long  data_size;            /* Total file size */
       long  data_offset;          /* File data offset in MAIN.DAT */
       char  filename[13];         /* Name of the file */
} DATA_ENTRY;

DATA_ENTRY *data;
long num_entries = 0;

void write_file (char *filename, FILE *datafile, DATA_ENTRY *p);
void get_data_entry (DATA_ENTRY *p, char *filename, FILE *datafile);

long filelength (char *filename);

int main (int argc, char **argv)
{
       int i;
       FILE *f;

       if ((f = fopen ("main.dat", "wb")) == NULL) {
              perror ("Cannot open MAIN.DAT");
              return EXIT_FAILURE;
       }
       fseek (f, 0, SEEK_END);

       num_entries = argc - 1;

       data = (DATA_ENTRY *) calloc (num_entries, sizeof (DATA_ENTRY));
       if (data == NULL) {
              perror ("calloc() failed");
              return EXIT_FAILURE;
       }

       /* Get the header block */
       for (i = 1; i < argc; i++)
              get_data_entry (&data[i - 1], argv[i], f);

       /* Write the header */
       fwrite (data, sizeof (DATA_ENTRY), num_entries, f);

       /* Write the data */
       for (i = 1; i < argc; i++) {
              long tmp_offset = ftell (f);

              /*
               * Quick solution to write data_offset
               */
              fseek (f, ((i - 1) * sizeof (DATA_ENTRY)) + 4, SEEK_SET);
              fwrite (&tmp_offset, sizeof (long), 1, f);
              fseek (f, tmp_offset, SEEK_SET);

              write_file (argv[i], f, &data[i - 1]);
              printf ("%s: %ld bytes at %ld\n", data[i - 1].filename,
                                                data[i - 1].data_size,
                                                data[i - 1].data_offset);
       }

       /* Write the number of entries stamp */
       fwrite (&num_entries, sizeof (long), 1, f); /* Write number of entries */
       fclose (f);
       free (data);
       return (0);
}


void get_data_entry (DATA_ENTRY *p, char *filename, FILE *datafile)
{
       if (datafile == NULL)
              return;

       strcpy (p->filename, filename);      /* Get filename */
       p->data_size = filelength (filename);

       if (p->data_size == -1)
              printf ("Error: %s", p->filename);
}
/*
 * Writes file to MAIN.DAT (f)
 */
void write_file (char *filename, FILE *datafile, DATA_ENTRY *p)
{
       FILE *input;
       char *buf = NULL;
       short i;

       if (datafile == NULL)
              return;
       /*
        * Open source file
        */
       input = fopen (filename, "rb");
       if (input == NULL) {
              perror (filename);
              exit (EXIT_FAILURE);
       }
       buf = (char *) malloc (p->data_size * sizeof (char));
       if (buf == NULL) {
              perror ("write_file: malloc() failed");
              exit (EXIT_FAILURE);
       }
       p->data_offset = ftell (datafile);   /* MAIN.DAT file pointer */
       fread (buf, sizeof (char), p->data_size, input);
       fwrite (buf, sizeof (char), p->data_size, datafile);

       free (buf);
       fclose (input);
}


long filelength (char *filename)
{
       FILE *f = fopen (filename, "rb");
       long filesize = -1;

       if (f == NULL)
              return -1;

       fseek (f, 0, SEEK_END);
       filesize = ftell (f);
       fclose (f);

       return (filesize);
}
