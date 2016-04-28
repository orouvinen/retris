#ifndef TETRIS_H
#define TETRIS_H

#define FALSE 0
#define TRUE  1

#define DECIMAL 10   /* For string to integer conversions */

struct game_state {
       char  shape;           /* Current shape    */
       char  next_shape;      /* Next shape       */
       char  rot;             /* Current rotation */
       short x, y;            /* X & Y position of shape (block X/Y) */
       short rows;            /* Rows dropped     */
       short rank;            /* Current rank     */
       unsigned long score;   /* Current score    */
       short level;           /* Current level    */
       char  paused;          /* Pause flag       */
       char  running;         /* Is a game currently running */
};

/*
 * Main.dat header entry structure
 */
typedef struct {
       long  data_size;            /* Total file size */
       long  data_offset;          /* File data offset in MAIN.DAT */
       char  filename[13];         /* Name of the entry file */
} DATA_ENTRY;


#define RANK_NONE (SCORE_LIST_SIZE + 1)

/* Macros to solve the size of the current shape */
#define SHAPE_WIDTH() shape_dimensions[tetris.shape][tetris.rot & 1]
#define SHAPE_HEIGHT() shape_dimensions[tetris.shape][1 - (tetris.rot & 1)]

/*
 * Rotating directions for rotate_shape()
 */
#define RIGHT 1
#define LEFT -1
#define CLOCKWISE         RIGHT
#define COUNTERCLOCKWISE  LEFT


/*
 * Function prototypes
 */
void rotate_shape (int direction);
int  move_down (void);
void move_side (int direction);
int  shape_fits (int x, int y);
void mark_shape (void);
int  next_shape (void);
int  shape_stuck (void);
int  full_row (int y);
void shift_row (int y);
void erase_row (int y);
void end_game (void);
void check_rank (void);
void game (void);
void reset_game (void);
void pause_game (void);
int has_gaps (void);

int  menu (char *title, int num_items, char **item_text, int x, int y);

void get_player_name (void);
int  get_string (char *dest, int x, int y, int maxlength, char *buf);

int  load_data (void);

DATA_ENTRY *find_data_entry (const char *filename);
int  is_386_cpu (void);

void credit_display (void);

#endif /* TETRIS_H */
