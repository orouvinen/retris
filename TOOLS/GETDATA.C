/*
 * Gets data from Main.dat
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
       long  data_size;            /* Total file size */
       long  data_offset;          /* File data offset in MAIN.DAT */
       char  filename[13];         /* Name of the file */
} DATA_ENTRY;

DATA_ENTRY *tmp = NULL;

DATA_ENTRY *find_data_entry (char *filename, FILE *datafile);

int main (int argc, char **argv)
{
       FILE *datafile;
       FILE *out;
       char *data;

       DATA_ENTRY *data_info;

       if (argc < 2) {
              fprintf (stderr, "File name needed\n");
              return 1;
       }
       datafile = fopen ("main.dat", "rb");
       out = fopen (argv[1], "wb");

       data_info = find_data_entry (argv[1], datafile);
       if (data_info == NULL) {
              fprintf (stderr, "%s not found in MAIN.DAT\n", argv[1]);
              return 1;
       }
       data = (char *) malloc (data_info->data_size * sizeof (char));
       if (data == NULL) {
              perror ("malloc failed in main()");
              return 1;
       }
       fseek (datafile, data_info->data_offset, SEEK_SET);
       fread (data, sizeof (char), data_info->data_size, datafile); 
       fwrite (data, sizeof (char), data_info->data_size, out);
       fclose (datafile);
       fclose (out);

       free (tmp);
       free (data);

       return (0);
}

DATA_ENTRY *find_data_entry (char *filename, FILE *datafile)
{
       register int i;
       long offset = ftell (datafile);
       long num_entries = 0;

       /* Get number of entries */
       fseek (datafile, -4, SEEK_END);
       fread (&num_entries, sizeof (long), 1, datafile);

       if (tmp == NULL) {
              tmp = (DATA_ENTRY *) malloc (num_entries * sizeof (DATA_ENTRY));
              if (tmp == NULL) {
                     perror ("malloc failed in find_data_entry()");
                     exit (EXIT_FAILURE);
              }
       }
       /* Read in the data info block */
       fseek (datafile, 0, SEEK_SET);
       fread (tmp, sizeof (DATA_ENTRY), num_entries, datafile);

       for (i = 0; i < num_entries; i++) {
              if (strcmp (filename, tmp[i].filename) == 0)
                     return (&tmp[i]);
       }
       fseek (datafile, offset, SEEK_SET);
       return (DATA_ENTRY *) NULL;
}
