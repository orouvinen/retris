#ifndef SCORE_H
#define SCORE_H

#include "retris.h"

#define SCORE_LIST_SIZE 100
#define MAX_NAME_LENGTH 8
#define SCORE_FILE "hiscore.dat"
#define SCORE_PAGE_SIZE 20

typedef struct hiscore_entry {
       unsigned long score;
       char *player_name;
} SCORE_ENTRY;

int  allocate_score_list (void);
void free_score_list (void);

int  check_score (unsigned long score);
int  read_score_list_file (FILE *datafile, DATA_ENTRY *p);
int  write_score_list_file (FILE *datafile, DATA_ENTRY *p);
void display_hiscores (int start_from);
void roll_score_list (int index);

/*
 * Interface (ugh..)
 */
void put_score_entry (const char *name, unsigned long score, int pos);
unsigned long get_score (int pos);
char *get_name (int index);

#endif /* SCORE_H */
