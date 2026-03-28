# pong.c

pong.c



**on VGA\_BUFFER:**

um hello this is Alex speaking. im gonna explain how the VGA\_BUFFER actually works if u two wanna fuck around with it. think of it like laying out the pixels of the screen in one big line in memory. you'd THINK intuitively that each row of pixels would be there in sequence, like: for line 1, y == 0, all x values from 0 to 319, and then immediately after that move on to y == 1 and again all x values. however this is NOT exactly how it works and there's a lot of dead space after each row so that you only need to multiply by powers of 2 to find the location of a pixel, which is much faster for the microprocessor to compute. left bitshifting a number is the same as multiplying it by 2^(the number of times ur bitshifting), so the y << 9 below is the same as saying y\*(2^9), which is 512. so, each row in memory starts at intervals of 512, i.e. 0, 512, 1024, with nothing useful stored between 320 and 511 in each row. hope that makes sense and sorry for the paragraph lol, im putting this here as much for me to remember for the demo as for you guys to understand.



