#include "apps/blob_native/blob_native_app.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <utility>

#include "GUI_Paint.h"
#include "LCD_1in28.h"
#include "platform/waveshare_native_board.h"

namespace {

const int SCREEN_W = 240;
const int SCREEN_H = 240;
const float CENTER_X = SCREEN_W / 2.0f;
const float CENTER_Y = SCREEN_H / 2.0f;
const float BLOB_RADIUS = 50.0f;
const int POINTS = 48;
const uint16_t BLOB_COLOR = CYAN;
const uint16_t OUTLINE_COLOR = BLUE;
const uint16_t BG_COLOR = BLACK;
const uint16_t GUIDE_COLOR = GRAY;

float blob_x = CENTER_X;
float blob_y = CENTER_Y;
float vel_x = 0.0f;
float vel_y = 0.0f;
float phase_t = 0.0f;

int16_t px_old[POINTS];
int16_t py_old[POINTS];
bool first_frame = true;

int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void draw_hline(int16_t x0, int16_t x1, int16_t y, uint16_t color) {
  if (y < 0 || y >= SCREEN_H) return;
  if (x0 > x1) {
    int16_t t = x0;
    x0 = x1;
    x1 = t;
  }
  x0 = clamp_i16(x0, 0, SCREEN_W - 1);
  x1 = clamp_i16(x1, 0, SCREEN_W - 1);
  for (int16_t x = x0; x <= x1; x++) {
    Paint_SetPixel(x, y, color);
  }
}

void fill_flat_bottom(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  const float invslope1 = (float)(x1 - x0) / (float)(y1 - y0);
  const float invslope2 = (float)(x2 - x0) / (float)(y2 - y0);

  float curx1 = (float)x0;
  float curx2 = (float)x0;

  for (int16_t y = y0; y <= y1; y++) {
    draw_hline((int16_t)curx1, (int16_t)curx2, y, color);
    curx1 += invslope1;
    curx2 += invslope2;
  }
}

void fill_flat_top(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  const float invslope1 = (float)(x2 - x0) / (float)(y2 - y0);
  const float invslope2 = (float)(x2 - x1) / (float)(y2 - y1);

  float curx1 = (float)x2;
  float curx2 = (float)x2;

  for (int16_t y = y2; y >= y0; y--) {
    draw_hline((int16_t)curx1, (int16_t)curx2, y, color);
    curx1 -= invslope1;
    curx2 -= invslope2;
  }
}

void fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color) {
  if (y0 > y1) {
    std::swap(y0, y1);
    std::swap(x0, x1);
  }
  if (y1 > y2) {
    std::swap(y1, y2);
    std::swap(x1, x2);
  }
  if (y0 > y1) {
    std::swap(y0, y1);
    std::swap(x0, x1);
  }

  if (y1 == y2) {
    fill_flat_bottom(x0, y0, x1, y1, x2, y2, color);
    return;
  }

  if (y0 == y1) {
    fill_flat_top(x0, y0, x1, y1, x2, y2, color);
    return;
  }

  const int16_t x3 = (int16_t)(x0 + ((float)(y1 - y0) / (float)(y2 - y0)) * (x2 - x0));
  const int16_t y3 = y1;

  fill_flat_bottom(x0, y0, x1, y1, x3, y3, color);
  fill_flat_top(x1, y1, x3, y3, x2, y2, color);
}

void compute_blob(float cx, float cy, float vx, float vy, int16_t* px, int16_t* py) {
  const float speed = sqrtf(vx * vx + vy * vy);
  const float move_angle = atan2f(vy, vx);

  for (int i = 0; i < POINTS; i++) {
    const float theta = (2.0f * PI * i) / POINTS;

    float r = BLOB_RADIUS + 4.0f * sinf(3.0f * theta + phase_t * 1.3f)
                        + 2.5f * sinf(5.0f * theta - phase_t * 0.9f)
                        + 1.5f * sinf(7.0f * theta + phase_t * 0.4f);

    const float squish = constrain(speed * 1.2f, 0.0f, 8.0f);
    r += squish * cosf(theta - move_angle);
    r -= squish * 0.5f * cosf(theta - move_angle + PI);

    px[i] = (int16_t)(cx + r * cosf(theta));
    py[i] = (int16_t)(cy + r * sinf(theta));
  }
}

void update_bounds(const int16_t* px, const int16_t* py, int16_t* min_x, int16_t* min_y, int16_t* max_x, int16_t* max_y) {
  for (int i = 0; i < POINTS; i++) {
    if (px[i] < *min_x) *min_x = px[i];
    if (py[i] < *min_y) *min_y = py[i];
    if (px[i] > *max_x) *max_x = px[i];
    if (py[i] > *max_y) *max_y = py[i];
  }
}

void draw_blob_outline(const int16_t* px, const int16_t* py) {
  for (int i = 0; i < POINTS; i++) {
    int next = (i + 1) % POINTS;
    Paint_DrawLine(px[i], py[i], px[next], py[next], OUTLINE_COLOR, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  }
}

void draw_blob_fill(float cx, float cy, const int16_t* px, const int16_t* py) {
  const int16_t ccx = (int16_t)cx;
  const int16_t ccy = (int16_t)cy;
  for (int i = 0; i < POINTS; i++) {
    int next = (i + 1) % POINTS;
    fill_triangle(ccx, ccy, px[i], py[i], px[next], py[next], BLOB_COLOR);
  }
}

}  // namespace

void blob_native_app_setup() {
  Serial.begin(115200);
  if (!waveshare_native_begin()) {
    Serial.println("Native board init failed");
    return;
  }

  waveshare_native_clear(BG_COLOR);
  Paint_DrawCircle((UWORD)CENTER_X, (UWORD)CENTER_Y, 119, GUIDE_COLOR, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  waveshare_native_present_full();

  for (int i = 0; i < POINTS; i++) {
    px_old[i] = (int16_t)CENTER_X;
    py_old[i] = (int16_t)CENTER_Y;
  }
}

void blob_native_app_loop() {
  if (waveshare_native_framebuffer() == NULL) {
    delay(50);
    return;
  }

  float tilt_x = 0.0f;
  float tilt_y = 0.0f;
  waveshare_native_read_tilt(&tilt_x, &tilt_y);

  const float target_x = CENTER_X + tilt_y * (CENTER_X - BLOB_RADIUS - 6.0f);
  const float target_y = CENTER_Y - tilt_x * (CENTER_Y - BLOB_RADIUS - 6.0f);

  vel_x = vel_x * 0.75f + (target_x - blob_x) * 0.15f;
  vel_y = vel_y * 0.75f + (target_y - blob_y) * 0.15f;

  blob_x += vel_x;
  blob_y += vel_y;

  int16_t px_new[POINTS];
  int16_t py_new[POINTS];
  compute_blob(blob_x, blob_y, vel_x, vel_y, px_new, py_new);

  int16_t min_x = SCREEN_W - 1;
  int16_t min_y = SCREEN_H - 1;
  int16_t max_x = 0;
  int16_t max_y = 0;
  update_bounds(px_new, py_new, &min_x, &min_y, &max_x, &max_y);

  if (!first_frame) {
    update_bounds(px_old, py_old, &min_x, &min_y, &max_x, &max_y);
  }

  const int16_t margin = 5;
  min_x = clamp_i16(min_x - margin, 0, SCREEN_W - 1);
  min_y = clamp_i16(min_y - margin, 0, SCREEN_H - 1);
  max_x = clamp_i16(max_x + margin, 0, SCREEN_W - 1);
  max_y = clamp_i16(max_y + margin, 0, SCREEN_H - 1);

  Paint_ClearWindows(min_x, min_y, max_x + 1, max_y + 1, BG_COLOR);

  Paint_DrawCircle((UWORD)CENTER_X, (UWORD)CENTER_Y, 119, GUIDE_COLOR, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
  draw_blob_fill(blob_x, blob_y, px_new, py_new);
  draw_blob_outline(px_new, py_new);

  waveshare_native_present_window(min_x, min_y, max_x, max_y);

  memcpy(px_old, px_new, sizeof(px_new));
  memcpy(py_old, py_new, sizeof(py_new));
  first_frame = false;

  phase_t += 0.08f;
  delay(16);
}
