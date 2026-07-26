#include "platform/waveshare_native_board.h"

#include <Arduino.h>
#include <string.h>

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "LCD_1in28.h"
#include "QMI8658.h"

#ifndef WAVESHARE_NATIVE_ENABLE_DOUBLE_BUFFER
#define WAVESHARE_NATIVE_ENABLE_DOUBLE_BUFFER 1
#endif

#ifndef WAVESHARE_NATIVE_MIN_FRAME_MS
#define WAVESHARE_NATIVE_MIN_FRAME_MS 0
#endif

static UWORD *s_framebuffer_a = NULL;
static UWORD *s_framebuffer_b = NULL;
static UWORD *s_drawbuffer = NULL;
static UWORD *s_frontbuffer = NULL;
static bool s_double_buffer_active = false;
static uint16_t s_min_frame_ms = WAVESHARE_NATIVE_MIN_FRAME_MS;
static uint32_t s_last_present_ms = 0;

static void pace_present_if_needed()
{
  if (s_min_frame_ms == 0)
  {
    return;
  }

  const uint32_t now = millis();
  const uint32_t elapsed = now - s_last_present_ms;
  if (elapsed < s_min_frame_ms)
  {
    delay(s_min_frame_ms - elapsed);
  }
}

static void post_present_bookkeeping()
{
  s_last_present_ms = millis();

  if (!s_double_buffer_active)
  {
    return;
  }

  s_frontbuffer = s_drawbuffer;
  s_drawbuffer = (s_drawbuffer == s_framebuffer_a) ? s_framebuffer_b : s_framebuffer_a;

  const size_t frame_bytes = (size_t)LCD_1IN28_WIDTH * (size_t)LCD_1IN28_HEIGHT * sizeof(UWORD);
  memcpy(s_drawbuffer, s_frontbuffer, frame_bytes);
  Paint_SelectImage((UBYTE *)s_drawbuffer);
}

static int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi)
{
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

bool waveshare_native_begin()
{
  if (DEV_Module_Init() != 0)
  {
    return false;
  }

  LCD_1IN28_Init(HORIZONTAL);
  DEV_SET_PWM(100);

  const size_t image_size = (size_t)LCD_1IN28_HEIGHT * (size_t)LCD_1IN28_WIDTH * sizeof(UWORD);
  s_framebuffer_a = (UWORD *)malloc(image_size);
  if (s_framebuffer_a == NULL)
  {
    return false;
  }

  s_drawbuffer = s_framebuffer_a;
  s_frontbuffer = s_framebuffer_a;

#if WAVESHARE_NATIVE_ENABLE_DOUBLE_BUFFER
  s_framebuffer_b = (UWORD *)malloc(image_size);
  if (s_framebuffer_b != NULL)
  {
    s_double_buffer_active = true;
  }
#endif

  Paint_NewImage((UBYTE *)s_drawbuffer, LCD_1IN28.WIDTH, LCD_1IN28.HEIGHT, 0, BLACK);
  Paint_SetScale(65);
  Paint_SetRotate(ROTATE_0);
  Paint_Clear(BLACK);
  LCD_1IN28_Display(s_drawbuffer);

  if (s_double_buffer_active)
  {
    s_frontbuffer = s_drawbuffer;
    s_drawbuffer = s_framebuffer_b;
    memcpy(s_drawbuffer, s_frontbuffer, image_size);
    Paint_SelectImage((UBYTE *)s_drawbuffer);
  }

  s_last_present_ms = millis();

  QMI8658_init();
  return true;
}

uint16_t *waveshare_native_framebuffer()
{
  return s_drawbuffer;
}

void waveshare_native_clear(uint16_t color)
{
  Paint_Clear(color);
}

void waveshare_native_present_full()
{
  pace_present_if_needed();
  LCD_1IN28_Display(s_drawbuffer);
  post_present_bookkeeping();
}

void waveshare_native_present_window(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
  x0 = clamp_i16(x0, 0, LCD_1IN28_WIDTH - 1);
  y0 = clamp_i16(y0, 0, LCD_1IN28_HEIGHT - 1);
  x1 = clamp_i16(x1, 0, LCD_1IN28_WIDTH - 1);
  y1 = clamp_i16(y1, 0, LCD_1IN28_HEIGHT - 1);

  if (x0 > x1 || y0 > y1)
  {
    return;
  }

  pace_present_if_needed();
  LCD_1IN28_DisplayWindows(x0, y0, x1 + 1, y1 + 1, s_drawbuffer);
  post_present_bookkeeping();
}

void waveshare_native_present_set_min_frame_ms(uint16_t min_frame_ms)
{
  s_min_frame_ms = min_frame_ms;
}

bool waveshare_native_is_double_buffer_active()
{
  return s_double_buffer_active;
}

bool waveshare_native_read_tilt(float *tilt_x, float *tilt_y)
{
  if (tilt_x == NULL || tilt_y == NULL)
  {
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
