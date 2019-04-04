/*
graphics.h
Author: Hanna Veldhuizen
CSC 452
*/

#ifndef GRAPHICS_INCLUDED
#define GRAPHICS_INCLUDED

typedef unsigned short color_t;

static color_t RGB(int r, int g, int b) {
  // using bit shifting, shifts to the left by 11 for red and
  // 5 to the left for green while blue does not shift at all
  return r << 11 | g << 5 | b;
}

void init_graphics();

void exit_graphics();

char getkey();

void sleep_ms(long ms);

void clear_screen(void* img);

void draw_pixel(void* img, int x, int y, color_t color);

void draw_line(void* img, int x1, int y1, int x2, int y2, color_t c);

void* new_offscreen_buffer();

void blit(void* src);

#endif
