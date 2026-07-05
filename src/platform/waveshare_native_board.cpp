#include "platform/waveshare_native_board.h"

#include <Arduino.h>

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in28.h"
#include "QMI8658.h"

static UWORD* s_framebuffer = NULL;

static int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

bool waveshare_native_begin() {
  if (DEV_Module_Init() != 0) {
    return false;
  }

  LCD_1IN28_Init(HORIZONTAL);
  DEV_SET_PWM(100);

  const UDOUBLE image_size = LCD_1IN28_HEIGHT * LCD_1IN28_WIDTH * 2;
  s_framebuffer = (UWORD*)malloc(image_size);
  if (s_framebuffer == NULL) {
    return false;
  }

  Paint_NewImage((UBYTE*)s_framebuffer, LCD_1IN28.WIDTH, LCD_1IN28.HEIGHT, 0, BLACK);
  Paint_SetScale(65);
  Paint_SetRotate(ROTATE_0);
  Paint_Clear(BLACK);
  LCD_1IN28_Display(s_framebuffer);

  QMI8658_init();
  return true;
}

uint16_t* waveshare_native_framebuffer() {
  return s_framebuffer;
}

void waveshare_native_clear(uint16_t color) {
  Paint_Clear(color);
}

void waveshare_native_present_full() {
  LCD_1IN28_Display(s_framebuffer);
}

void waveshare_native_present_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  x0 = clamp_i16(x0, 0, LCD_1IN28_WIDTH - 1);
  y0 = clamp_i16(y0, 0, LCD_1IN28_HEIGHT - 1);
  x1 = clamp_i16(x1, 0, LCD_1IN28_WIDTH - 1);
  y1 = clamp_i16(y1, 0, LCD_1IN28_HEIGHT - 1);

  if (x0 > x1 || y0 > y1) {
    return;
  }

  LCD_1IN28_DisplayWindows(x0, y0, x1 + 1, y1 + 1, s_framebuffer);
}

bool waveshare_native_read_tilt(float* tilt_x, float* tilt_y) {
  if (tilt_x == NULL || tilt_y == NULL) {
    return false;
  }

  float acc[3] = {0};
  float gyro[3] = {0};
  unsigned int tim_count = 0;
  QMI8658_read_xyz(acc, gyro, &tim_count);

  *tilt_x = constrain(acc[0] / 1000.0f, -1.0f, 1.0f);
  *tilt_y = constrain(acc[1] / 1000.0f, -1.0f, 1.0f);
  return true;
}
