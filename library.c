/*
library.c
Author: Hanna Veldhuizen
CSC 452
This is the implementation of methods defined in graphics.h. This program
can manipulate pixels in the QEMU enviornment. Use driver code to see implementation.
*/

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>
#include <linux/fb.h>
#include "graphics.h"

static int size;
static struct fb_var_screeninfo varScreenInfo;
static struct fb_fix_screeninfo fixScreenInfo;
static color_t* frameBuffer;
static int file;
static struct termios terminalSettings;

/*
init_graphics() -- does all necessary work to initialize the graphics library.
Instead of doing multiple reads and writes, this function maps a file (/dev/fb0)
into our address space so we can manipulate it like an array. All we need to change
a pixel is it's x and y "location" (indices). This function also disables the
keypress echo to easily read user input.
*/
void init_graphics() {
  // open the "zero-th" framebuffer file and save file descriptor for future use
  file = open("/dev/fb0", O_RDWR);

  // virtual resolution of screen
  ioctl(file, FBIOGET_VSCREENINFO, &varScreenInfo);

  // bit depth
  ioctl(file, FBIOGET_FSCREENINFO, &fixScreenInfo);

  // calculate total size based on the return values from ioctl sys call
  size = varScreenInfo.yres_virtual * fixScreenInfo.line_length;

  // memory mapto treat the file as an array
  frameBuffer = (color_t*) (mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, file, 0));

  // get terminal settings using TCGETS
  ioctl(STDIN_FILENO, TCGETS, &terminalSettings);

  // disable the keypress echo by reversing the "ICANON" and "ECHO" bits
  terminalSettings.c_lflag &= ~ICANON;
  terminalSettings.c_lflag &= ~ECHO;

  // set terminal settings using the terminalSettings we just adjusted
  ioctl(STDIN_FILENO, TCSETS, &terminalSettings);
}

/*
exit_graphics() -- this basically reverses all work done in init_graphics().
If the program crashes, run `stty echo` to turn keypress echo back on. Here,
we clear the screen so the last drawn graphics are erased, turn the key press
echo back on, and unmap the memory we used.
*/
void exit_graphics()
{
  clear_screen(frameBuffer);

  // return settings back to how they were before this
  // program ran
  ioctl(STDIN_FILENO, TCGETS, &terminalSettings);

  // turn the keypress echo back on
  terminalSettings.c_lflag |= ECHO;
  terminalSettings.c_lflag |= ICANON;

  // set terminal settings
  ioctl(STDIN_FILENO, TCSETS, &terminalSettings);

  // unmap memory used for frame buffer and close file
  munmap(frameBuffer, size);
  close(file);
}

/*
getKey() -- manually reading input from the user using the select sys call.
This only reads one character at a time.
*/
char getkey()
{
  // input for select about how long to wait before timing out
  struct timeval timeout;

  // file descriptor flags
  fd_set fd;

  // user input we will read
  char input = '\n';

  // select() documentation suggested "initializing the file descriptor set"
  // is good practice when using select
  FD_ZERO(&fd);
  FD_SET(0, &fd);

  // timeout for key stroke input (param for select)
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  // get input from user
  int x = select(1, &fd, NULL, NULL, &timeout);

  // no error or timeout
  if (x == 1) {
    read(STDIN_FILENO, &input, sizeof(input));
  }

  return input;
}

/*
sleep_ms(long ms) -- sleeps for the indicated number of miliseconds.
*/
void sleep_ms(long ms) {
  // sleep man page
  struct timespec time;
  time.tv_sec = ms / 1000;
  time.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&time, NULL);
}

/*
new_offscreen_buffer() -- maps memory to be used as an array of pixels on the
QEMU screen. mmap is used instead of multiple reads and writes because it is
much faster, using only pointer arithmetic.
*/
void* new_offscreen_buffer() {
  return mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

/*
blit(void* original) -- memory copy from off screen buffer to frame buffer
*/
void blit(void* img) {
  color_t* offScreen = (color_t*) img;

  // loop through the enter offscreen buffer and copy it to the frame buffer
  int i;
  for (i = 0; i < size/2; i++) {
    frameBuffer[i] = offScreen[i];
  }
}

/*
draw_pixel(void* img, int x, int y, color_t color) -- draws a pixel at
(x, y) in the indicated color. img is the frame buffer we will be drawing into.
Because we are using a memory map, all we have to do to draw a pixel is calculate
it's index and set that index to the unsigned integer color.
*/
void draw_pixel(void* img, int x, int y, color_t color) {
  // check in bounds
  if (y >= varScreenInfo.yres_virtual || y < 0 || x < 0 || x >= varScreenInfo.xres_virtual) {
    return;
  }

  // cast to color_t
  color_t* offScreen = (color_t*) img;

  // draw the pixel at (x, y) to the color passed in
  // must multiple by xres to scale accordingly
  int index = x + (y * varScreenInfo.xres_virtual);
  offScreen[index] = color;
}

/*
draw_line() -- draws a line from (x0, y0) to (x1, y1) in the indicated color.
The following code for Bresenham's Algorithm was found at:
https://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm
This was the only completed code I could find that took into account
all different directions of a line in a clean and easy to understand fashion.
*/
void draw_line(void* img, int x0, int y0, int x1, int y1, color_t color) {
  // adjusted here to account for x1/y1 actually being smaller than x0/y0
  int dx = (x1>x0 ? (x1-x0) : (x0-x1));
  int dy = (y1>y0 ? (y1-y0) : (y0-y1));

  int sx = x0<x1 ? 1 : -1;
  int sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;

  for(;;) {
    draw_pixel(img, x0, y0, color);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

/*
clear_screen(void* img) -- this "clears" the screen by setting every byte to
zero in both the off screen buffer and frame buffer.
*/
void clear_screen(void* img) {
  color_t* offScreen = (color_t*) img;

  int i;
  for (i = 0; i < size/2; i++) {
    frameBuffer[i] = 0;
    offScreen[i] = 0;
  }
}
