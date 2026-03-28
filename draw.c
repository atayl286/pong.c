/* The two lines below this should be replaced with this block on the actual DE-10 via the Intel FPGA Monitor program.
#include "address_map_arm.h"
volatile uint16_t* VGA_BUFFER = (uint16_t*)FPGA_PIXEL_BUF_BASE; */
	
#include <stdint.h>
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xC8000000;

//This needs to be in quotes rather than <>
#include "draw.h"


// 0 < x < 320
// 0 < y < 240
// color == 16-bit color 
void draw_pixel(int x, int y, uint16_t colour)
{
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

    /* um hello this is Ollie speaking. I've stolen your function for my own nefarious purposes.*/
    *(VGA_BUFFER + (y << 9) + x) = colour;
}

//Manually sets the screen to the background colour. Should only be called at the start of the program or on reset or whatever
void wipe_screen()
{
	for (int i = 0; i < 320; i++) {
		for (int j = 0; j < 240; j++) {
			draw_pixel(i, j, BACKGROUND);
		}
	}
}

//Draws a circle of given radius and colour and a given position
void draw_circle(int xpos, int ypos, int radius, uint16_t colour)
{
	for (int r = -radius; r <= radius; r++){
        int x = sqrt(pow(radius,2) - pow(r,2));
        
        for ( int i = -x; i <= x; i++){
            draw_pixel(xpos + i, ypos + r, colour);
        }
    }
}

//Draws the ball given a position for the center pixel
void draw_ball(int xpos, int ypos){
	draw_circle(xpos, ypos, (int)(BALL_DIAMETER/2), FOREGROUND);
}

//Clears the space the ball would occupy, good to call before drawing the new ball position
void clear_ball(int xpos, int ypos)
{
	draw_circle(xpos, ypos, (int)(BALL_DIAMETER/2), BACKGROUND);
}

//Draws a rectangle of a given width and height and colour at a given position
void draw_rectangle(int xpos, int ypos, int width, int height, uint16_t colour)
{
	for(int y = ypos - (height/2); y <= ypos + (height/2); y++)
	{
		for(int x = xpos - (width/2); x <= xpos + (width/2); x++)
		{
			draw_pixel(x, y, colour);
		}
	}
}

//Draws a paddle at a given position
void draw_paddle(int xpos, int ypos)
{
	draw_rectangle(xpos, ypos, PADDLE_WIDTH, PADDLE_HEIGHT, FOREGROUND);
}

//Clears the space a paddle would occupy, good to call before drawing the new ball position
void clear_paddle(int xpos, int ypos)
{
	draw_rectangle(xpos, ypos, PADDLE_WIDTH, PADDLE_HEIGHT, BACKGROUND);
}

//Displays the pause icon
void draw_pause()
{
	draw_rectangle(130, 120, 30, 80, FOREGROUND);
	draw_rectangle(190, 120, 30, 80, FOREGROUND);
}

//Clears the pause icon
void clear_pause()
{
	draw_rectangle(130, 120, 30, 80, BACKGROUND);
	draw_rectangle(190, 120, 30, 80, BACKGROUND);
}

//Displays "P"
void draw_p()
{
	draw_rectangle(128, 50, 5, 60, FOREGROUND);
	draw_rectangle(138, 22, 20, 5, FOREGROUND);
	draw_rectangle(138, 48, 20, 5, FOREGROUND);
	draw_rectangle(148, 35, 5, 30, FOREGROUND);
}

//Displays "1"
void draw_1()
{
	draw_rectangle(174, 50, 5, 60, FOREGROUND);
	draw_rectangle(174, 78, 20, 5, FOREGROUND);
	draw_rectangle(170, 26, 9, 5, FOREGROUND);
}

//Displays "2"
void draw_2()
{
	draw_rectangle(174, 22, 20, 5, FOREGROUND);
	draw_rectangle(166, 65, 5, 30, FOREGROUND);
	draw_rectangle(174, 50, 20, 5, FOREGROUND);
	draw_rectangle(182, 35, 5, 30, FOREGROUND);
	draw_rectangle(174, 78, 20, 5, FOREGROUND);
}

//Displays "WINS"
void draw_wins()
{
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

//Displays the player win messages (player should be 1 or 2)
void draw_P_WINS(int player)
{
	draw_p();
	draw_wins();

	if(player == 1)
		draw_1();
	else
		draw_2();
}