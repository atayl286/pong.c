/* The two lines below this should be replaced with this block on the actual DE-10 via the Intel FPGA Monitor program.
#include "address_map_arm.h"
volatile uint16_t* VGA_BUFFER = (uint16_t*)FPGA_PIXEL_BUF_BASE; */
	
#include <stdint.h>
volatile uint16_t* VGA_BUFFER = (uint16_t*)0xC8000000;

#include "draw.h"

int main(void) {
	wipe_screen();

    draw_pixel(64, 32, 0xFFFF); // Draw a white pixel at x=64, y=32 
    
    while(1) {}
    return 0;
}