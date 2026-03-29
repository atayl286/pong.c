/* The two lines below this should be replaced with this block on the actual DE-10 via the Intel FPGA Monitor program.
#include "address_map_arm.h"
volatile uint16_t* VGA_BUFFER = (uint16_t*)FPGA_PIXEL_BUF_BASE; */
	
#include <stdint.h>
#include <math.h>
#include <string.h>
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xC8000000;

// Defualt colours
uint16_t WHITE = 0xFFFF;

// Ball default values
int BALL_SIZE = 8;
int BALL_INIT_SPEED = 5;
int BALL_START[2] = {320/2, 240/2};


/* A9 Private Timer */
#define A9_TIMER_BASE    0xFFFEC600
#define A9_LOAD          0x00
#define A9_COUNT         0x04
#define A9_CONTROL       0x08
#define A9_STATUS        0x0C
#define TIMER_TICKED() (*TIMER_STAT & 1)
	
volatile uint32_t *TIMER_LOAD    = (uint32_t *)(A9_TIMER_BASE + A9_LOAD);
volatile uint32_t *TIMER_CTRL    = (uint32_t *)(A9_TIMER_BASE + A9_CONTROL);
volatile uint32_t *TIMER_STAT    = (uint32_t *)(A9_TIMER_BASE + A9_STATUS);

// 0 < x < 320
// 0 < y < 240
// color == 16-bit color 
void draw_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= 320 || y < 0 || y >= 240) return; // Return if not in the pixel range of the display (320*240)
    
    /* um hello this is Alex speaking. im gonna explain how the VGA_BUFFER actually works if u two 
	wanna fuck around with it. think of it like laying out the pixels of the screen in one big line
	in memory. you'd THINK intuitively that each row of pixels would be there in sequence, like:
	for line 1, y == 0, all x values from 0 to 319, and then immediately after that move on to y == 1
	and again all x values. however this is NOT exactly how it works and there's a lot of dead space
	after each row so that you only need to multiply by powers of 2 to find the location of a pixel,
	which is much faster for the microprocessor to compute. left bitshifting a number is the same as 
	multiplying it by 2^(the number of times ur bitshifting), so the y << 9 below is the same as saying
	y*(2^9), which is 512. so, each row in memory starts at intervals of 512, i.e. 0, 512, 1024, with
	nothing useful stored between 320 and 511 in each row. hope that makes sense and sorry for the
	paragraph lol, im putting this here as much for me to remember for the demo as for you guys to 
	understand. */
    *(VGA_BUFFER + (y << 9) + x) = color;
}


static void timer_init(void) {
    *TIMER_CTRL = 0;
    *TIMER_LOAD = 3333333U;   // 200MHz * 0.01s
    *TIMER_STAT = 1U;         // clear timeout
    *TIMER_CTRL = 0x3U;       // enable + auto-reload
}


void draw_square(int xpos, int ypos, int height, int width, uint16_t colour){
	int offset_x = xpos - (width/2);
	int offset_y = ypos - (height/2);
	for (int i = 0; i < height; i++){
		for (int j = 0; j < width; j++){
		
			draw_pixel(offset_x + i, offset_y + j, WHITE);
		}
	}
}


void draw_circle(int xpos, int ypos, int rad, uint16_t colour){
	for (int r = -rad; r <= rad; r++){
		int x = sqrt(pow(rad,2) - pow(r,2));
		
		for ( int i = -x; i <= x; i++){
			draw_pixel(xpos + i, ypos + r, colour);
		}
	}
}


void clear_screen(){
	memset((void*)VGA_BUFFER, 0x0000, 512 * 240 * sizeof(uint16_t));
}

// Checks if the ball has created some sort of collision and changes speed acordingly
// Returns speed as a new speed array [x,y]

int check_ball_collisions(x, y, speed_x, speed_y){
	// Check if pos + speed overlaps not just pos
	// Try and make paddle collide on a circle hitbox not a square
	// Wall hitbox willalways be on a perfect x/y peak of the circle so it doesnt matter
	// ball + paddle will change reflection angle based on dist from centre of the paddle
	// keep speed = init_speed^2 = sqrt(x^2 +y^2)
	// wall bounce is either reset(player point) or perfect just invert y speed(top/bottom wall bounce)
	
	return [speed_x, speed_y];	// Filler for now
}



int main(void) {

	// Ball starting non constnat values
	// All arrays for pos/speed are [x,y]
	int ball_speed[2] = {0,5};
	int ball_pos[2] = {BALL_START[0], BALL_START[1]};
	
	
	// Player scores
	// [P1, P2]
	int score[2]={0,0};
	
	// Clear the screen and start the timers
	clear_screen();
	timer_init();
	
    while(1) {
		
		if (TIMER_TICKED()) {
			*TIMER_STAT = 1;
			static int tick_count = 0;
			tick_count++;
			if (tick_count >= 2) {
				tick_count = 0;
				
				// Measure first players pos here

				clear_screen();
				
				ball_speed = check_ball_collisions(ball_pos[0], ball_pos[1], ball_speed[0], ball_speed[1]);

			
				
				// Update ball pos
				ball_pos += ball_speed;
				draw_circle(ball_pos[0], ball_pos[1], BALL_SIZE, WHITE);
			}else{
				// Measure other players pos her
			}
		}
    
	}
    return 0;
}