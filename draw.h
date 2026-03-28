#ifndef DRAW_H_
#define DRAW_H_

#include <stdint.h>
#include <math.h>

//Colours
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BACKGROUND BLACK
#define FOREGROUND WHITE

//Size of the sprites
#define BALL_DIAMETER 7
#define PADDLE_WIDTH 5
#define PADDLE_HEIGHT 50

void draw_pixel(int x, int y, uint16_t colour);
void wipe_screen();
void draw_ball(int xpos, int ypos);
void clear_ball(int xpos, int ypos);
void draw_circle(int xpos, int ypos, int radius, uint16_t colour);
void draw_paddle(int xpos, int ypos);
void clear_paddle(int xpos, int ypos);
void draw_pause();
void clear_pause();
void draw_P_WINS(int player)

#endif