// #include "address_map_arm.h"
#include <stdint.h>
#include <math.h>
	
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xC8000000 /* (uint16_t*)FPGA_PIXEL_BUF_BASE */; // Hardware VGA buffer.
uint16_t pixel_buffer[240][512] = {0}; // Software pixel buffer (for double-buffering)

// Timer memory locations:
volatile uint32_t* timer_load    = (uint32_t*)(0xFFFEC600 + 0x00);
volatile uint32_t* timer_counter = (uint32_t*)(0xFFFEC600 + 0x04);
volatile uint32_t* timer_control = (uint32_t*)(0xFFFEC600 + 0x08);
volatile uint32_t* timer_status  = (uint32_t*)(0xFFFEC600 + 0x0C);

// Colours:
#define WHITE 0xFFFF
#define BLACK 0x0000
#define BACKGROUND BLACK
#define FOREGROUND WHITE

// Size of the sprites:
#define BALL_DIAMETER 7
#define PADDLE_WIDTH 5
#define PADDLE_HEIGHT 50

#define X_CENTRE 160
#define Y_CENTRE 120

#define P1_X 20
#define P2_X 300

// Ball:
int ball_x = X_CENTRE;
int ball_y = Y_CENTRE;
int ball_v_x = 6; // Velocity X (Pixels per frame)
int ball_v_y = 3; // Velocity Y (Pixels per frame)
int ball_t = Y_CENTRE - BALL_DIAMETER/2;
int ball_b = Y_CENTRE + BALL_DIAMETER/2;
int ball_l = X_CENTRE - BALL_DIAMETER/2;
int ball_r = X_CENTRE + BALL_DIAMETER/2;

// P1 Paddle:
int p1_x = P1_X;
int p1_y = Y_CENTRE;
int p1_t = Y_CENTRE - PADDLE_HEIGHT/2;
int p1_b = Y_CENTRE + PADDLE_HEIGHT/2;
int p1_l = P1_X - PADDLE_WIDTH/2;
int p1_r = P1_X + PADDLE_WIDTH/2;

// P2 Paddle:
int p2_x = P2_X;
int p2_y = Y_CENTRE;
int p2_t = Y_CENTRE - PADDLE_HEIGHT/2;
int p2_b = Y_CENTRE + PADDLE_HEIGHT/2;
int p2_l = P2_X - PADDLE_WIDTH/2;
int p2_r = P2_X + PADDLE_WIDTH/2;

int main(void) {
	init_frame_timer();

    while(1) {
		wait_for_next_frame();
		clear_pixel_buffer();

		update_ball();
		draw_ball(ball_x, ball_y, FOREGROUND);
		draw_paddle(p1_x, p1_y);
		draw_paddle(p2_x, p2_y);

		push_frame();
	}
    return 0;
}

// Initializes the ARM A9 timer.
void init_frame_timer() {
	*timer_control = 0; // Turns it OFF while changing settings (if it happens to be ON already)
	*timer_load = 33333; // Time between frames (will need to be changed on real hardware)
	*timer_status = 1; // Clear any remaning old status flags
	*timer_control = 3; // Start time with auto-reload
}

// Idles until the frame timer hits 0.
void wait_for_next_frame() {
	while ((*timer_status & 0x1) == 0) {
		// Wait
	}

	*timer_status = 1; // Clear flag
}

// 0 < x < 320
// 0 < y < 240
// colour: 16-bit
void draw_pixel(int x, int y, uint16_t colour) {
    if (x < 0 || x >= 320 || y < 0 || y >= 240) return; // Return if not in the pixel range of the display (320*240)
    pixel_buffer[y][x] = colour;
}

// Copies the entirety of pixel_buffer into the VGA_BUFFER (they have the exact same structure in memory)
void push_frame() {
	memcpy((void*)VGA_BUFFER, pixel_buffer, sizeof(pixel_buffer));
}

// Sets every byte in pixel_buffer to 0x00 (each colour is two bytes, so this ends up making each pixel be coloured with 0x0000)
void clear_pixel_buffer() {
	memset(pixel_buffer, 0, sizeof(pixel_buffer));
}

// Updates the position of the ball
void update_ball() {
	int next_ball_t = ball_y + ball_v_y + BALL_DIAMETER/2;
	int next_ball_b = ball_y + ball_v_y  - BALL_DIAMETER/2;
	int next_ball_l = ball_x + ball_v_x  - BALL_DIAMETER/2;
	int next_ball_r = ball_x + ball_v_x  + BALL_DIAMETER/2;

	// Top side collides with top of screen
	if (next_ball_t <= 0) {
		ball_v_y *= -1;
	}

	// Bottom side collides with bottom of screen
	if (next_ball_b >= 240) {
		ball_v_y *= -1;
	}

	// Left side collides with edge of screen of P1
	if (next_ball_l <= 0 || (next_ball_l <= p1_r && (next_ball_b >= p1_t && next_ball_t <= p1_b))) {
		ball_v_x *= -1;
	}

	// Right side collides with edge of screen or P2
	if (next_ball_r >= 320 || (next_ball_r >= p2_r && (next_ball_b >= p2_t && next_ball_t <= p2_b))) {
		ball_v_x *= -1;
	}

	ball_x += ball_v_x;
	ball_y += ball_v_y;

	ball_t = ball_y + BALL_DIAMETER/2;
	ball_b = ball_y - BALL_DIAMETER/2;
	ball_l = ball_x - BALL_DIAMETER/2;
	ball_r = ball_x + BALL_DIAMETER/2;
}

// Draws a circle of given radius and colour and a given position
void draw_circle(int xpos, int ypos, int radius, uint16_t colour) {
	for (int r = -radius; r <= radius; r++){
        int x = sqrt(pow(radius,2) - pow(r,2));
        
        for ( int i = -x; i <= x; i++){
            draw_pixel(xpos + i, ypos + r, colour);
        }
    }
}

// Draws the ball given a position for the center pixel
void draw_ball(int xpos, int ypos){
	draw_circle(xpos, ypos, (int)(BALL_DIAMETER/2), FOREGROUND);
}

// Draws a rectangle of a given width and height and colour at a given position
void draw_rectangle(int xpos, int ypos, int width, int height, uint16_t colour) {
	for(int y = ypos - (height/2); y <= ypos + (height/2); y++)
	{
		for(int x = xpos - (width/2); x <= xpos + (width/2); x++)
		{
			draw_pixel(x, y, colour);
		}
	}
}

// Draws a paddle at a given position
void draw_paddle(int xpos, int ypos) {
	draw_rectangle(xpos, ypos, PADDLE_WIDTH, PADDLE_HEIGHT, FOREGROUND);
}

// Displays the pause icon
void draw_pause() {
	draw_rectangle(130, 120, 30, 80, FOREGROUND);
	draw_rectangle(190, 120, 30, 80, FOREGROUND);
}

// Displays "P"
void draw_p() {
	draw_rectangle(128, 50, 5, 60, FOREGROUND);
	draw_rectangle(138, 22, 20, 5, FOREGROUND);
	draw_rectangle(138, 48, 20, 5, FOREGROUND);
	draw_rectangle(148, 35, 5, 30, FOREGROUND);
}

// Displays "1"
void draw_1() {
	draw_rectangle(174, 50, 5, 60, FOREGROUND);
	draw_rectangle(174, 78, 20, 5, FOREGROUND);
	draw_rectangle(170, 26, 9, 5, FOREGROUND);
}

// Displays "2"
void draw_2() {
	draw_rectangle(174, 22, 20, 5, FOREGROUND);
	draw_rectangle(166, 65, 5, 30, FOREGROUND);
	draw_rectangle(174, 50, 20, 5, FOREGROUND);
	draw_rectangle(182, 35, 5, 30, FOREGROUND);
	draw_rectangle(174, 78, 20, 5, FOREGROUND);
}

// Displays "WINS"
void draw_wins() {
	//W
	draw_rectangle(90, 140, 5, 60, FOREGROUND);
	draw_rectangle(100, 150, 5, 40, FOREGROUND);
	draw_rectangle(100, 168, 20, 5, FOREGROUND);
	draw_rectangle(110, 140, 5, 60, FOREGROUND);
	
	//I
	draw_rectangle(135, 140, 5, 60, FOREGROUND);
	draw_rectangle(135, 112, 20, 5, FOREGROUND);
	draw_rectangle(135, 168, 20, 5, FOREGROUND);

	//N
	draw_rectangle(160, 140, 5, 60, FOREGROUND);
	draw_rectangle(165, 136, 5, 5, FOREGROUND);
	draw_rectangle(170, 140, 5, 5, FOREGROUND);
	draw_rectangle(175, 144, 5, 5, FOREGROUND);
	draw_rectangle(180, 140, 5, 60, FOREGROUND);

	//S
	draw_rectangle(205, 112, 20, 5, FOREGROUND);
	draw_rectangle(213, 155, 5, 30, FOREGROUND);
	draw_rectangle(205, 140, 20, 5, FOREGROUND);
	draw_rectangle(197, 125, 5, 30, FOREGROUND);
	draw_rectangle(205, 168, 20, 5, FOREGROUND);
}

// Displays the player win messages (player should be 1 or 2)
void draw_P_WINS(int player) {
	draw_p();
	draw_wins();

	if(player == 1)
		draw_1();
	else
		draw_2();
}