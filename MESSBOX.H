#ifndef MESSBOX_H
#define MESSBOX_H

#define MAX_MESSAGE_LENGTH  27

/* Prototypes */
void print_game_messages (void);
void add_game_message (char *message, ...);

void reset_messages (void);

#endif /* MESSBOX_H */