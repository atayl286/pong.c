/* The two lines below this should be replaced with this block on the actual DE-10 via the Intel FPGA Monitor program.
#include "address_map_arm.h"
volatile uint16_t* VGA_BUFFER = (uint16_t*)FPGA_PIXEL_BUF_BASE; */
	
#include <stdint.h>
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xC8000000;

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

int main(void) {
	// Draw all pixels black:
	for (int i = 0; i < 320; i++) {
		for (int j = 0; j < 240; j++) {
			draw_pixel(i, j, 0x0000);
		}
	}

    draw_pixel(64, 32, 0xFFFF); // Draw a white pixel at x=64, y=32 
    
    while(1) {}
    return 0;
}