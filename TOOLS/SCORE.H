#ifndef SCORE_H
#define SCORE_H

#define SCORE_LIST_SIZE 100
#define MAX_NAME_LENGTH 10
#define SCORE_FILE "hiscore.dat"

struct hiscore_entry {
       char *player_name;
       unsigned long score;
};

int  allocate_score_list (void);
void free_score_list (void);

int check_hiscore (unsigned long score);
int read_score_list_file (void);
int write_score_list_file (void);

/*
 * Routines for sorting
 */
void  swap_entry (struct hiscore_entry *p1, struct hiscore_entry *p2);
short partition (struct hiscore_entry *scores, short left, short right);
void  do_qsort (struct hiscore_entry *scores, const short left,
                                                 const short right);
void sort_score_list (struct hiscore_entry *scores);

#endif /* SCORE_H */
