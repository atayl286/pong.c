// #include "address_map_arm.h"
#include <stdint.h>
#include <math.h>
#include <string.h>
	
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xC8000000 /* (uint16_t*)FPGA_PIXEL_BUF_BASE */; // Hardware VGA buffer.
uint16_t pixel_buffer[240][512] = {0}; // Software pixel buffer (for double-buffering)

// Timer memory locations:
volatile uint32_t* timer_load    = (uint32_t*)(0xFFFEC600 + 0x00);
volatile uint32_t* timer_counter = (uint32_t*)(0xFFFEC600 + 0x04);
volatile uint32_t* timer_control = (uint32_t*)(0xFFFEC600 + 0x08);
volatile uint32_t* timer_status  = (uint32_t*)(0xFFFEC600 + 0x0C);

//Other memory locations:
volatile uint32_t* seven_segment_base_0_3 = (uint32_t*)(0xFF200020);
volatile uint32_t* seven_segment_base_4_5 = (uint32_t*)(0xFF200030);



#define ADC_BASE   0xFF204000u
#define KEY_BASE   0xFF200050

volatile uint32_t *KEY_ptr = (uint32_t *)KEY_BASE;


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
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define P1_X 20
#define P2_X 300

//Game control:
#define SCORE_TO_WIN 10
#define STATE_PLAY 0
#define STATE_PAUSE 1
#define STATE_WIN 2

// Timer macros:
#define TIMER_TICKED() ((*timer_status & 0x1u) != 0u)
#define TIMER_STAT timer_status

//Seven segment codes
char seven_segment_digits[] = {0x3F, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7D, 0x07, 0x7f, 0x6f};
char seven_segment_P = 0x73;
char seven_segment_left1 = 0x30;

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

//Scoring and game control:
int p1_score = 0;
int p2_score = 0;
int gamestate = STATE_PAUSE;

// Prototypes:
void init_frame_timer(void);
void seven_segment_init(void);
void wait_for_next_frame(void);
void clear_pixel_buffer(void);
void update_ball(void);
void draw_ball(int xpos, int ypos);
void draw_paddle(int xpos, int ypos);
void seven_segment_update(void);
void push_frame(void);
void score_point(int player);
void draw_P_WINS(int player);
static inline uint32_t adc_to_y_pos(uint32_t adc_value);

int main(void) {
	init_frame_timer();
	seven_segment_init();
	uint32_t prev_pressed = 0;

	//Main loop
    while(1) {

		// Handle key presses (for pausing and resetting the game)
		uint32_t raw = *KEY_ptr & 0xFU;        // active-low
        uint32_t pressed = (~raw) & 0xFU;      // active-high
        uint32_t edge = pressed & (~prev_pressed);
        prev_pressed = pressed;

		if (edge & 0x1U){
			//KEY0: Pause
			if(gamestate == STATE_PLAY){
				gamestate = STATE_PAUSE;
			}else if(gamestate == STATE_PAUSE){
				gamestate = STATE_PLAY;
			}else if(gamestate == STATE_WIN){

				//reset game
				p1_score = 0;
				p2_score = 0;
				ball_x = X_CENTRE;
				ball_y = Y_CENTRE;
				ball_v_x = 6;
				ball_v_y = 3;

				gamestate = STATE_PLAY;
			}
		}


		
		//measure one players pos at a time and only update screen after both have been measured
		if (TIMER_TICKED()) {
			*TIMER_STAT = 1;
			static int tick_count = 0;
			tick_count++;
			if (tick_count >= 2) {
				tick_count = 0;

				// Update p1 y pos
				p1_y = adc_to_y_pos(*(volatile uint32_t*)(ADC_BASE));
				p1_t = p1_y - PADDLE_HEIGHT/2;
				p1_b = p1_y + PADDLE_HEIGHT/2;

			}else{
				// Update p2 y pos
				p2_y = adc_to_y_pos(*(volatile uint32_t*)(ADC_BASE + 0x04));
				p2_t = p2_y - PADDLE_HEIGHT/2;
				p2_b = p2_y + PADDLE_HEIGHT/2;

				//Logic that always happens
				wait_for_next_frame();

				//State-based logic
				switch(gamestate) {
					case STATE_PLAY:
						clear_pixel_buffer();

						update_ball();
						draw_ball(ball_x, ball_y);
						draw_paddle(p1_x, p1_y);
						draw_paddle(p2_x, p2_y);

						seven_segment_update();

						push_frame();
						break;

					case STATE_PAUSE:
						draw_pause();
						push_frame();
						
						break;

					case STATE_WIN:
						//TEMPORARY, CHANGE TO REAL RESET LOGIC
						break;
				}

			}

		}
	}

    return 0;
}

// Initializes the ARM A9 timer.
void init_frame_timer() {
	*timer_control = 0; // Turns it OFF while changing settings (if it happens to be ON already)
	*timer_load = 3333333U; // Time between frames (will need to be changed on real hardware)
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
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return; // Return if not in the pixel range of the display (320*240)
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
	int y_overlap_p1 = (next_ball_b >= p1_t && next_ball_t <= p1_b);
	int y_overlap_p2 = (next_ball_b >= p2_t && next_ball_t <= p2_b);

	// Top side collides with top of screen
	if (next_ball_t <= 0) {
		ball_v_y *= -1;
	}

	// Bottom side collides with bottom of screen
	if (next_ball_b >= SCREEN_HEIGHT) {
		ball_v_y *= -1;
	}

	// Paddle collisions first (direction-aware) so a valid paddle hit is not treated like a wall score.
	if (ball_v_x < 0 && next_ball_l <= p1_r && next_ball_r >= p1_l && y_overlap_p1) {
		ball_v_x *= -1;
	}

	if (ball_v_x > 0 && next_ball_r >= p2_l && next_ball_l <= p2_r && y_overlap_p2) {
		ball_v_x *= -1;
	}

	// Missed paddle and reached a side wall: score.
	if (next_ball_l <= 0) {
		ball_v_x *= -1;
		score_point(1);
	}

	if (next_ball_r >= SCREEN_WIDTH) {
		ball_v_x *= -1;
		score_point(2);
	}

	ball_x += ball_v_x;
	ball_y += ball_v_y;

	ball_t = ball_y + BALL_DIAMETER/2;
	ball_b = ball_y - BALL_DIAMETER/2;
	ball_l = ball_x - BALL_DIAMETER/2;
	ball_r = ball_x + BALL_DIAMETER/2;
}

// Scores a point for the given player and check for their win (player should be 1 or 2)
void score_point(int player) {
	switch (player) {
		case 1:
			if(p1_score < SCORE_TO_WIN - 1)
				p1_score++;
			else {
				//P1 wins
				gamestate = STATE_WIN;
				draw_P_WINS(2);
			}
			break;
		
		case 2:
			if(p2_score < SCORE_TO_WIN - 1)
				p2_score++;
			else {
				//P2 wins
				gamestate = STATE_WIN;
				draw_P_WINS(1);
			}
			break;
	}
}

// Initializes the seven segment displays with 0 P 1(left) P 2 0
void seven_segment_init() {
	//First four digits
	*seven_segment_base_0_3 = 0;
	*seven_segment_base_0_3 |= seven_segment_left1 << 24;
	*seven_segment_base_0_3 |= seven_segment_P << 16;
	*seven_segment_base_0_3 |= seven_segment_digits[2] << 8;
	*seven_segment_base_0_3 |= seven_segment_digits[0];

	//Last two digits
	*seven_segment_base_4_5 = 0;
	*seven_segment_base_4_5 |= seven_segment_digits[0] << 8;
	*seven_segment_base_4_5 |= seven_segment_P;
}

// Updates the seven segment displays based on the current score
void seven_segment_update() {
	//Wipe the first and last digits
	*seven_segment_base_0_3 &= 0xFFFFFF00;
	*seven_segment_base_4_5 &= 0x000000FF;

	//Update digits
	*seven_segment_base_0_3 |= seven_segment_digits[p1_score];
	*seven_segment_base_4_5 |= seven_segment_digits[p2_score] << 8;
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


static inline uint32_t adc_to_y_pos(uint32_t adc_value)
{
    uint32_t ypos = (adc_value * SCREEN_HEIGHT) / 4095u;

    if (ypos == 0u) return 0u;
    if (ypos > SCREEN_HEIGHT) ypos = SCREEN_HEIGHT;

    return ypos;
}