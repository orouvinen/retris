#ifndef GRAPHICS_H
#define GRAPHICS_H

#define SCREEN_HEIGHT       200
#define SCREEN_WIDTH        320
#define SCREEN_SIZE         SCREEN_WIDTH*SCREEN_HEIGHT
#define FONT_WIDTH          6
#define FONT_HEIGHT         6


#define BLOCK_HEIGHT        9
#define BLOCK_WIDTH         9

#define PLAY_AREA_MIN_X     20
#define PLAY_AREA_MAX_X     PLAY_AREA_MIN_X + (BLOCK_WIDTH * 10)
#define PLAY_AREA_MIN_Y     10
#define PLAY_AREA_MAX_Y     PLAY_AREA_MIN_Y + (BLOCK_HEIGHT * 20)

#define INFO_BOX_X          150
#define INFO_BOX_Y          10

#define TEXTCOLOR           239

/*
 * Some colors
 */
#define DYNAMIC_TEXT_COLOR 41
#define STATIC_TEXT_COLOR  40

#define TEXT_FIELD_NORMAL  42
#define TEXT_FIELD_DARK    43
#define TEXT_FIELD_LIGHT   44

/*
 * General use colors
 */
#define GEN_COLOR_1        45
#define GEN_COLOR_2        46
#define GEN_COLOR_3        47
#define GEN_COLOR_4        48

#define MENU_ITEM_1_COLOR  51
#define MENU_ITEM_2_COLOR  52
#define MENU_ITEM_3_COLOR  53
#define MENU_ITEM_4_COLOR  54
#define MENU_ITEM_5_COLOR  55
#define MENU_ITEM_6_COLOR  56
#define MENU_ITEM_7_COLOR  57
#define MENU_ITEM_8_COLOR  58

#define HISCORE_COLOR_START 92  /* Where gradient starts at hiscore list */

#define drawpixel(x,y,c,p) (p)[line_offset[(y)]+(x)]=(c)

int  graphics_init (void);
void init_game_graphics (void);
void graphics_end (void);
void draw_block (int x, int y, int blocknum, unsigned char far *buf);
void draw_shape (int shape, int rot, int x, int y, unsigned char far *buf);
void draw_text_field (int x, int y, int width, int height, unsigned char far *buf);
void draw_box (int x, int y, int width, int height, int color, unsigned char far *buf);
void message_box_text (char *message, char *buf);
void new_message_box (char *message, char *buf);

void set_palbuf_rgb (unsigned char i, char r, char g, char b, char *palette);
void palette_init (void);
/*
 * 8x8 font blitter
 */
void blit_char (unsigned char c, int x, int y, unsigned char color,
                                               unsigned char far *ptr);
void blit_string (char *s, int x, int y, unsigned char color,
                                         unsigned char far *ptr);
                                         
void draw_block_char (char c, int x, int y, unsigned char far *ptr);
void fadedown (void);
void flash_background(void);
void print_game_info (unsigned char far *buf);
#endif /* GRAPHICS_H */
