/*
driver.c
Author: Hanna Veldhuizen
CSC 452
This is the driver code for library.c and graphics.h. It uses the
methods defined in those files to draw squares to the screen.
When running this program, you have the following key press options:
'<key>' -- <result>
'+' -- increases size of square by 10 (max value = 100)
'-' -- decreases size of square by 10 (min value = 10)
'r' -- changes outline of squares to red
'g' -- changes outline of squares to green
'b' -- changes outline of squares to blue
'q' -- quit the program
*/

#include "graphics.h"

void draw(void* buffer, int offset, color_t color) {
	int i;
	for(i = 0; i < 480; i += offset) {
		draw_line(buffer, 0, i, 639, i, color);
	}

	for(i = 0; i < 640; i += offset) {
		draw_line(buffer, i, 0, i, 479, color);
	}
}

int main(int argc, char **argv) {
	init_graphics();

	void *buf = new_offscreen_buffer();
  color_t color = RGB(255, 0, 0);
	char key;
	int offset = 10;

	draw(buf, offset, color);
	blit(buf);

	do {
		key = getkey();

		if (key == 'q')
			break;

		else if (key == '+' && offset < 100) {
			offset += 10;
		} else if (key == '-' && offset > 10) {
			offset -= 10;
		} else if (key == 'r') {
			color = RGB(255, 0, 0);
		} else if (key =='b') {
			color = RGB(0, 0, 255);
		} else if (key == 'g') {
			color = RGB(0, 255, 0);
		}

		clear_screen(buf);
		draw(buf, offset, color);
		blit(buf);

		sleep_ms(200);
	} while(1);

	clear_screen(buf);
  exit_graphics();
  return 0;
}
