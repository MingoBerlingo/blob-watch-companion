#include "platform/waveshare_native_board.h"

#include <Arduino.h>

#include "DEV_Config.h"
#include "LCD_1in28.h"
#include "QMI8658.h"

#ifndef WAVESHARE_NATIVE_ENABLE_TOUCH
#define WAVESHARE_NATIVE_ENABLE_TOUCH 1
#endif

#if WAVESHARE_NATIVE_ENABLE_TOUCH
#include "CST816S.h"
#endif

#if WAVESHARE_NATIVE_ENABLE_TOUCH
static bool s_touch_ready = false;
static uint8_t s_touch_init_attempts = 0;
static uint32_t s_touch_last_init_ms = 0;
#endif

#if WAVESHARE_NATIVE_ENABLE_TOUCH
static WaveshareNativeTouchGesture map_touch_gesture(uint8_t gesture)
{
  if (gesture == CST816S_Gesture_Up)
    return WaveshareNativeTouchGesture::Up;
  if (gesture == CST816S_Gesture_Down)
    return WaveshareNativeTouchGesture::Down;
  if (gesture == CST816S_Gesture_Left)
    return WaveshareNativeTouchGesture::Left;
  if (gesture == CST816S_Gesture_Right)
    return WaveshareNativeTouchGesture::Right;
  if (gesture == CST816S_Gesture_Click)
    return WaveshareNativeTouchGesture::Click;
  if (gesture == CST816S_Gesture_Double_Click)
    return WaveshareNativeTouchGesture::DoubleClick;
  if (gesture == CST816S_Gesture_Long_Press)
    return WaveshareNativeTouchGesture::LongPress;
  return WaveshareNativeTouchGesture::None;
}
#endif

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

  QMI8658_init();
#if WAVESHARE_NATIVE_ENABLE_TOUCH
  s_touch_ready = false;
  s_touch_init_attempts = 0;
  s_touch_last_init_ms = 0;
#endif
  return true;
}

void waveshare_native_clear_display(uint16_t color)
{
  LCD_1IN28_ClearSafe((UWORD)color);
}

void waveshare_native_flush_area(int16_t x0, int16_t y0, int16_t x1, int16_t y1, const uint16_t *pixels)
{
  if (pixels == NULL)
  {
    return;
  }

  x0 = clamp_i16(x0, 0, LCD_1IN28_WIDTH - 1);
  y0 = clamp_i16(y0, 0, LCD_1IN28_HEIGHT - 1);
  x1 = clamp_i16(x1, 0, LCD_1IN28_WIDTH - 1);
  y1 = clamp_i16(y1, 0, LCD_1IN28_HEIGHT - 1);

  if (x0 > x1 || y0 > y1)
  {
    return;
  }

  LCD_1IN28_DisplayArea((UWORD)x0, (UWORD)y0, (UWORD)(x1 + 1), (UWORD)(y1 + 1), (UWORD *)pixels);
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

bool waveshare_native_poll_touch(WaveshareNativeTouchSample *sample)
{
  if (sample == NULL)
  {
    return false;
  }

  sample->touching = false;
  sample->x = 0;
  sample->y = 0;
  sample->gesture = WaveshareNativeTouchGesture::None;

#if !WAVESHARE_NATIVE_ENABLE_TOUCH
  return true;
#else
  if (!s_touch_ready)
  {
    const uint32_t now = millis();
    if (s_touch_init_attempts < 3 && (s_touch_last_init_ms == 0 || (now - s_touch_last_init_ms) > 500))
    {
      s_touch_last_init_ms = now;
      s_touch_init_attempts++;
      s_touch_ready = CST816S_init(CST816S_ALL_Mode) ? true : false;
    }

    if (!s_touch_ready)
    {
      return false;
    }
  }

  sample->gesture = map_touch_gesture(CST816S_Get_Gesture());
  sample->touching = (CST816S_Get_FingerNum() > 0);
  const CST816S point = CST816S_Get_Point();
  sample->x = point.x_point;
  sample->y = point.y_point;

  return true;
#endif
}
