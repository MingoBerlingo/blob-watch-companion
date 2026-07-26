#include "apps/blob_native/blob_native_app.h"

#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <utility>

#include "GUI_Paint.h"
#include "LCD_1in28.h"
#include "platform/waveshare_native_board.h"

namespace
{

  const int SCREEN_W = 240;
  const int SCREEN_H = 240;
  const float CENTER_X = SCREEN_W / 2.0f;
  const float CENTER_Y = SCREEN_H / 2.0f;
  const float BLOB_RADIUS = 22.0f;
  const int POINTS = 48;
  const uint16_t BLOB_COLOR = CYAN;
  const uint16_t OUTLINE_COLOR = BLUE;
  const uint16_t BG_COLOR = BLACK;
  const uint16_t GUIDE_COLOR = GRAY;
  const uint16_t EYE_COLOR = WHITE;
  const uint16_t MOUTH_COLOR = WHITE;
  const int GLOW_LAYER_COUNT = 3;
  const float GLOW_LAYER_SCALE[GLOW_LAYER_COUNT] = {1.3f, 1.2f, 1.1f};
  const uint16_t GLOW_LAYER_COLOR[GLOW_LAYER_COUNT] = {0x0008, 0x082b, 0x106f};
  const int16_t EYE_RADIUS = 1;
  const float EYE_FORWARD = BLOB_RADIUS * 0.28f;
  const float EYE_SIDE = BLOB_RADIUS * 0.15f;
  const float MOUTH_FORWARD = -BLOB_RADIUS * 0.08f;
  const float MOUTH_HALF_LEN = BLOB_RADIUS * 0.16f;
  const float MOUTH_SMILE_DEPTH = BLOB_RADIUS * 0.05f;
  const float EYE_DIR_MIN_SPEED = 3.0f;
  const float EYE_DIR_MAX_TURN = 0.10f;
  const float EYE_DIR_TURN_DEADBAND = 0.08f;
  const float FACE_IDLE_SPEED_MAX = 3.0f;
  const float FACE_IDLE_EYE_BOB = BLOB_RADIUS * 0.03f;
  const float FACE_IDLE_MOUTH_BOB = BLOB_RADIUS * 0.04f;
  const float FACE_IDLE_BLINK_RATE = 0.85f;
  const float FACE_IDLE_BLINK_THRESHOLD = 0.965f;
  const float MAX_BLOB_EXTENT = (BLOB_RADIUS + 16.0f) * GLOW_LAYER_SCALE[0];
  const int DIRTY_MARGIN = 8;
  const int BACKUP_SIDE = (int)(MAX_BLOB_EXTENT * 2.0f) + DIRTY_MARGIN * 2 + 4;
  const int BACKUP_PIXELS = BACKUP_SIDE * BACKUP_SIDE;

  struct Rect
  {
    int16_t x0;
    int16_t y0;
    int16_t x1;
    int16_t y1;
  };

  float blob_x = CENTER_X;
  float blob_y = CENTER_Y;
  float vel_x = 0.0f;
  float vel_y = 0.0f;
  float eye_dir_x = 0.0f;
  float eye_dir_y = -1.0f;
  float face_phase_prev = 0.0f;
  float phase_t = 0.0f;

  uint16_t *saved_region = NULL;
  Rect saved_rect = {0, 0, -1, -1};
  bool has_saved_region = false;
  bool use_saved_region = false;

  int16_t px_old[POINTS];
  int16_t py_old[POINTS];
  bool first_frame = true;

  void update_bounds(const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);
  void update_glow_bounds(float cx, float cy, const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y);

  void update_point_bounds(int16_t x, int16_t y, int16_t radius, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
  {
    if (x - radius < *min_x)
      *min_x = x - radius;
    if (y - radius < *min_y)
      *min_y = y - radius;
    if (x + radius > *max_x)
      *max_x = x + radius;
    if (y + radius > *max_y)
      *max_y = y + radius;
  }

  void normalize_vec(float *x, float *y)
  {
    const float len = sqrtf((*x) * (*x) + (*y) * (*y));
    if (len < 0.0001f)
    {
      *x = 0.0f;
      *y = -1.0f;
      return;
    }

    *x /= len;
    *y /= len;
  }

  void update_eye_direction(float vx, float vy)
  {
    const float speed = sqrtf(vx * vx + vy * vy);
    if (speed < EYE_DIR_MIN_SPEED)
    {
      return;
    }

    float target_x = vx / speed;
    float target_y = vy / speed;

    float current_angle = atan2f(eye_dir_y, eye_dir_x);
    const float target_angle = atan2f(target_y, target_x);
    float delta = target_angle - current_angle;

    while (delta > PI)
      delta -= 2.0f * PI;
    while (delta < -PI)
      delta += 2.0f * PI;

    if (fabsf(delta) < EYE_DIR_TURN_DEADBAND)
    {
      return;
    }

    if (delta > EYE_DIR_MAX_TURN)
      delta = EYE_DIR_MAX_TURN;
    else if (delta < -EYE_DIR_MAX_TURN)
      delta = -EYE_DIR_MAX_TURN;

    current_angle += delta;
    eye_dir_x = cosf(current_angle);
    eye_dir_y = sinf(current_angle);
  }

  float compute_idle_amount(float motion_speed)
  {
    return 1.0f - constrain(motion_speed / FACE_IDLE_SPEED_MAX, 0.0f, 1.0f);
  }

  float compute_blink_amount(float motion_speed, float face_phase)
  {
    const float idle_amount = compute_idle_amount(motion_speed);
    const float blink_wave = 0.5f + 0.5f * sinf(face_phase * FACE_IDLE_BLINK_RATE);
    if (blink_wave <= FACE_IDLE_BLINK_THRESHOLD)
    {
      return 0.0f;
    }

    const float blink = (blink_wave - FACE_IDLE_BLINK_THRESHOLD) / (1.0f - FACE_IDLE_BLINK_THRESHOLD);
    return constrain(blink * idle_amount, 0.0f, 1.0f);
  }

  void compute_eye_positions(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                             int16_t *left_x, int16_t *left_y, int16_t *right_x, int16_t *right_y)
  {
    normalize_vec(&dir_x, &dir_y);

    const float side_x = -dir_y;
    const float side_y = dir_x;
    const float idle_amount = compute_idle_amount(motion_speed);
    const float idle_forward = cosf(face_phase * 1.3f) * FACE_IDLE_EYE_BOB * idle_amount;
    const float eye_forward = EYE_FORWARD + idle_forward;
    const float eye_side = EYE_SIDE;

    *left_x = (int16_t)(cx + dir_x * eye_forward - side_x * eye_side);
    *left_y = (int16_t)(cy + dir_y * eye_forward - side_y * eye_side);
    *right_x = (int16_t)(cx + dir_x * eye_forward + side_x * eye_side);
    *right_y = (int16_t)(cy + dir_y * eye_forward + side_y * eye_side);
  }

  void draw_blob_eyes(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase, uint16_t color)
  {
    int16_t left_x, left_y, right_x, right_y;
    compute_eye_positions(cx, cy, dir_x, dir_y, motion_speed, face_phase, &left_x, &left_y, &right_x, &right_y);

    const float blink_amount = compute_blink_amount(motion_speed, face_phase);
    if (blink_amount > 0.4f)
    {
      float side_x = -dir_y;
      float side_y = dir_x;
      normalize_vec(&side_x, &side_y);
      const int16_t half_lid = EYE_RADIUS + 1;

      const int16_t lx0 = (int16_t)(left_x - side_x * half_lid);
      const int16_t ly0 = (int16_t)(left_y - side_y * half_lid);
      const int16_t lx1 = (int16_t)(left_x + side_x * half_lid);
      const int16_t ly1 = (int16_t)(left_y + side_y * half_lid);

      const int16_t rx0 = (int16_t)(right_x - side_x * half_lid);
      const int16_t ry0 = (int16_t)(right_y - side_y * half_lid);
      const int16_t rx1 = (int16_t)(right_x + side_x * half_lid);
      const int16_t ry1 = (int16_t)(right_y + side_y * half_lid);

      Paint_DrawLine(lx0, ly0, lx1, ly1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
      Paint_DrawLine(rx0, ry0, rx1, ry1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
      return;
    }

    Paint_DrawCircle((UWORD)left_x, (UWORD)left_y, EYE_RADIUS, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
    Paint_DrawCircle((UWORD)right_x, (UWORD)right_y, EYE_RADIUS, color, DOT_PIXEL_1X1, DRAW_FILL_FULL);
  }

  void update_eyes_bounds(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                          int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
  {
    int16_t left_x, left_y, right_x, right_y;
    compute_eye_positions(cx, cy, dir_x, dir_y, motion_speed, face_phase, &left_x, &left_y, &right_x, &right_y);
    update_point_bounds(left_x, left_y, EYE_RADIUS + 1, min_x, min_y, max_x, max_y);
    update_point_bounds(right_x, right_y, EYE_RADIUS + 1, min_x, min_y, max_x, max_y);
  }

  void compute_mouth_points(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                            int16_t *x0, int16_t *y0, int16_t *xm, int16_t *ym, int16_t *x1, int16_t *y1)
  {
    normalize_vec(&dir_x, &dir_y);
    const float side_x = -dir_y;
    const float side_y = dir_x;
    const float idle_amount = compute_idle_amount(motion_speed);
    const float idle_bob = sinf(face_phase * 1.1f) * FACE_IDLE_MOUTH_BOB * idle_amount;

    const float mouth_cx = cx + dir_x * (MOUTH_FORWARD + idle_bob);
    const float mouth_cy = cy + dir_y * (MOUTH_FORWARD + idle_bob);

    *x0 = (int16_t)(mouth_cx - side_x * MOUTH_HALF_LEN);
    *y0 = (int16_t)(mouth_cy - side_y * MOUTH_HALF_LEN);
    *xm = (int16_t)(mouth_cx - dir_x * MOUTH_SMILE_DEPTH);
    *ym = (int16_t)(mouth_cy - dir_y * MOUTH_SMILE_DEPTH);
    *x1 = (int16_t)(mouth_cx + side_x * MOUTH_HALF_LEN);
    *y1 = (int16_t)(mouth_cy + side_y * MOUTH_HALF_LEN);
  }

  void draw_blob_mouth(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase, uint16_t color)
  {
    int16_t x0, y0, xm, ym, x1, y1;
    compute_mouth_points(cx, cy, dir_x, dir_y, motion_speed, face_phase, &x0, &y0, &xm, &ym, &x1, &y1);
    Paint_DrawLine(x0, y0, xm, ym, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(xm, ym, x1, y1, color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
  }

  void update_mouth_bounds(float cx, float cy, float dir_x, float dir_y, float motion_speed, float face_phase,
                           int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
  {
    int16_t x0, y0, xm, ym, x1, y1;
    compute_mouth_points(cx, cy, dir_x, dir_y, motion_speed, face_phase, &x0, &y0, &xm, &ym, &x1, &y1);
    update_point_bounds(x0, y0, 1, min_x, min_y, max_x, max_y);
    update_point_bounds(xm, ym, 1, min_x, min_y, max_x, max_y);
    update_point_bounds(x1, y1, 1, min_x, min_y, max_x, max_y);
  }

  int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi)
  {
    if (v < lo)
      return lo;
    if (v > hi)
      return hi;
    return v;
  }

  int rect_width(const Rect &rect)
  {
    return rect.x1 - rect.x0 + 1;
  }

  int rect_height(const Rect &rect)
  {
    return rect.y1 - rect.y0 + 1;
  }

  Rect rect_union(const Rect &a, const Rect &b)
  {
    Rect merged;
    merged.x0 = (a.x0 < b.x0) ? a.x0 : b.x0;
    merged.y0 = (a.y0 < b.y0) ? a.y0 : b.y0;
    merged.x1 = (a.x1 > b.x1) ? a.x1 : b.x1;
    merged.y1 = (a.y1 > b.y1) ? a.y1 : b.y1;
    return merged;
  }

  Rect make_blob_rect(float cx, float cy, float eye_x, float eye_y, float motion_speed, float face_phase,
                      const int16_t *px, const int16_t *py)
  {
    Rect rect = {SCREEN_W - 1, SCREEN_H - 1, 0, 0};
    update_bounds(px, py, &rect.x0, &rect.y0, &rect.x1, &rect.y1);
    update_glow_bounds(cx, cy, px, py, &rect.x0, &rect.y0, &rect.x1, &rect.y1);
    update_eyes_bounds(cx, cy, eye_x, eye_y, motion_speed, face_phase, &rect.x0, &rect.y0, &rect.x1, &rect.y1);
    update_mouth_bounds(cx, cy, eye_x, eye_y, motion_speed, face_phase, &rect.x0, &rect.y0, &rect.x1, &rect.y1);

    rect.x0 = clamp_i16(rect.x0 - DIRTY_MARGIN, 0, SCREEN_W - 1);
    rect.y0 = clamp_i16(rect.y0 - DIRTY_MARGIN, 0, SCREEN_H - 1);
    rect.x1 = clamp_i16(rect.x1 + DIRTY_MARGIN, 0, SCREEN_W - 1);
    rect.y1 = clamp_i16(rect.y1 + DIRTY_MARGIN, 0, SCREEN_H - 1);
    return rect;
  }

  void save_region_pixels(const Rect &rect)
  {
    uint16_t *framebuffer = waveshare_native_framebuffer();
    const int width = rect_width(rect);
    const int height = rect_height(rect);

    for (int row = 0; row < height; row++)
    {
      memcpy(&saved_region[row * width], &framebuffer[(rect.y0 + row) * SCREEN_W + rect.x0], width * sizeof(uint16_t));
    }
  }

  void restore_region_pixels(const Rect &rect)
  {
    uint16_t *framebuffer = waveshare_native_framebuffer();
    const int width = rect_width(rect);
    const int height = rect_height(rect);

    for (int row = 0; row < height; row++)
    {
      memcpy(&framebuffer[(rect.y0 + row) * SCREEN_W + rect.x0], &saved_region[row * width], width * sizeof(uint16_t));
    }
  }

  void draw_hline(int16_t x0, int16_t x1, int16_t y, uint16_t color)
  {
    if (y < 0 || y >= SCREEN_H)
      return;
    if (x0 > x1)
    {
      int16_t t = x0;
      x0 = x1;
      x1 = t;
    }
    x0 = clamp_i16(x0, 0, SCREEN_W - 1);
    x1 = clamp_i16(x1, 0, SCREEN_W - 1);
    for (int16_t x = x0; x <= x1; x++)
    {
      Paint_SetPixel(x, y, color);
    }
  }

  void fill_flat_bottom(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
  {
    const float invslope1 = (float)(x1 - x0) / (float)(y1 - y0);
    const float invslope2 = (float)(x2 - x0) / (float)(y2 - y0);

    float curx1 = (float)x0;
    float curx2 = (float)x0;

    for (int16_t y = y0; y <= y1; y++)
    {
      draw_hline((int16_t)curx1, (int16_t)curx2, y, color);
      curx1 += invslope1;
      curx2 += invslope2;
    }
  }

  void fill_flat_top(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
  {
    const float invslope1 = (float)(x2 - x0) / (float)(y2 - y0);
    const float invslope2 = (float)(x2 - x1) / (float)(y2 - y1);

    float curx1 = (float)x2;
    float curx2 = (float)x2;

    for (int16_t y = y2; y >= y0; y--)
    {
      draw_hline((int16_t)curx1, (int16_t)curx2, y, color);
      curx1 -= invslope1;
      curx2 -= invslope2;
    }
  }

  void fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
  {
    if (y0 > y1)
    {
      std::swap(y0, y1);
      std::swap(x0, x1);
    }
    if (y1 > y2)
    {
      std::swap(y1, y2);
      std::swap(x1, x2);
    }
    if (y0 > y1)
    {
      std::swap(y0, y1);
      std::swap(x0, x1);
    }

    if (y1 == y2)
    {
      fill_flat_bottom(x0, y0, x1, y1, x2, y2, color);
      return;
    }

    if (y0 == y1)
    {
      fill_flat_top(x0, y0, x1, y1, x2, y2, color);
      return;
    }

    const int16_t x3 = (int16_t)(x0 + ((float)(y1 - y0) / (float)(y2 - y0)) * (x2 - x0));
    const int16_t y3 = y1;

    fill_flat_bottom(x0, y0, x1, y1, x3, y3, color);
    fill_flat_top(x1, y1, x3, y3, x2, y2, color);
  }

  void compute_blob(float cx, float cy, float vx, float vy, int16_t *px, int16_t *py)
  {
    const float speed = sqrtf(vx * vx + vy * vy);
    const float move_angle = atan2f(vy, vx);

    for (int i = 0; i < POINTS; i++)
    {
      const float theta = (2.0f * PI * i) / POINTS;

      float r = BLOB_RADIUS + 4.0f * sinf(3.0f * theta + phase_t * 1.3f) + 2.5f * sinf(5.0f * theta - phase_t * 0.9f) + 1.5f * sinf(7.0f * theta + phase_t * 0.4f);

      const float squish = constrain(speed * 1.2f, 0.0f, 8.0f);
      r += squish * cosf(theta - move_angle);
      r -= squish * 0.5f * cosf(theta - move_angle + PI);

      px[i] = (int16_t)(cx + r * cosf(theta));
      py[i] = (int16_t)(cy + r * sinf(theta));
    }
  }

  void update_bounds(const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
  {
    for (int i = 0; i < POINTS; i++)
    {
      if (px[i] < *min_x)
        *min_x = px[i];
      if (py[i] < *min_y)
        *min_y = py[i];
      if (px[i] > *max_x)
        *max_x = px[i];
      if (py[i] > *max_y)
        *max_y = py[i];
    }
  }

  void draw_blob_outline(const int16_t *px, const int16_t *py, uint16_t color)
  {
    for (int i = 0; i < POINTS; i++)
    {
      int next = (i + 1) % POINTS;
      Paint_DrawLine(px[i], py[i], px[next], py[next], color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }
  }

  void scale_blob_points(float cx, float cy, float scale, const int16_t *px_in, const int16_t *py_in, int16_t *px_out, int16_t *py_out)
  {
    for (int i = 0; i < POINTS; i++)
    {
      px_out[i] = (int16_t)(cx + (px_in[i] - cx) * scale);
      py_out[i] = (int16_t)(cy + (py_in[i] - cy) * scale);
    }
  }

  void draw_blob_glow(float cx, float cy, const int16_t *px, const int16_t *py, uint16_t base_color)
  {
    int16_t glow_px[POINTS];
    int16_t glow_py[POINTS];

    for (int layer = 0; layer < GLOW_LAYER_COUNT; layer++)
    {
      scale_blob_points(cx, cy, GLOW_LAYER_SCALE[layer], px, py, glow_px, glow_py);

      for (int i = 0; i < POINTS; i++)
      {
        int next = (i + 1) % POINTS;
        const uint16_t layer_color = (base_color == BG_COLOR) ? BG_COLOR : GLOW_LAYER_COLOR[layer];
        Paint_DrawLine(glow_px[i], glow_py[i], glow_px[next], glow_py[next], layer_color, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
      }
    }
  }

  void update_glow_bounds(float cx, float cy, const int16_t *px, const int16_t *py, int16_t *min_x, int16_t *min_y, int16_t *max_x, int16_t *max_y)
  {
    int16_t glow_px[POINTS];
    int16_t glow_py[POINTS];

    scale_blob_points(cx, cy, GLOW_LAYER_SCALE[0], px, py, glow_px, glow_py);
    update_bounds(glow_px, glow_py, min_x, min_y, max_x, max_y);
  }

  void draw_blob_fill(float cx, float cy, const int16_t *px, const int16_t *py)
  {
    const int16_t ccx = (int16_t)cx;
    const int16_t ccy = (int16_t)cy;
    for (int i = 0; i < POINTS; i++)
    {
      int next = (i + 1) % POINTS;
      fill_triangle(ccx, ccy, px[i], py[i], px[next], py[next], BLOB_COLOR);
    }
  }

} // namespace

void blob_native_app_setup()
{
  Serial.begin(115200);
  if (!waveshare_native_begin())
  {
    Serial.println("Native board init failed");
    return;
  }

  waveshare_native_clear(BG_COLOR);
  waveshare_native_present_full();

  saved_region = (uint16_t *)malloc(BACKUP_PIXELS * sizeof(uint16_t));
  if (saved_region != NULL)
  {
    use_saved_region = true;
  }
  else
  {
    Serial.println("Blob backup buffer alloc failed; using geometry erase fallback");
  }

  for (int i = 0; i < POINTS; i++)
  {
    px_old[i] = (int16_t)CENTER_X;
    py_old[i] = (int16_t)CENTER_Y;
  }
}

void blob_native_app_loop()
{
  if (waveshare_native_framebuffer() == NULL)
  {
    delay(50);
    return;
  }

  float tilt_x = 0.0f;
  float tilt_y = 0.0f;
  waveshare_native_read_tilt(&tilt_x, &tilt_y);

  const float prev_blob_x = blob_x;
  const float prev_blob_y = blob_y;
  const float prev_eye_dir_x = eye_dir_x;
  const float prev_eye_dir_y = eye_dir_y;
  const float prev_face_phase = face_phase_prev;
  const float prev_speed = sqrtf(vel_x * vel_x + vel_y * vel_y);

  const float target_x = CENTER_X + tilt_y * (CENTER_X - BLOB_RADIUS - 6.0f);
  const float target_y = CENTER_Y - tilt_x * (CENTER_Y - BLOB_RADIUS - 6.0f);

  vel_x = vel_x * 0.75f + (target_x - blob_x) * 0.15f;
  vel_y = vel_y * 0.75f + (target_y - blob_y) * 0.15f;

  // Keep face orientation fixed (no rotation with movement).

  blob_x += vel_x;
  blob_y += vel_y;

  const float speed_now = sqrtf(vel_x * vel_x + vel_y * vel_y);

  int16_t px_new[POINTS];
  int16_t py_new[POINTS];
  compute_blob(blob_x, blob_y, vel_x, vel_y, px_new, py_new);

  const Rect current_rect = make_blob_rect(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, px_new, py_new);

  int16_t min_x = SCREEN_W - 1;
  int16_t min_y = SCREEN_H - 1;
  int16_t max_x = 0;
  int16_t max_y = 0;

  if (use_saved_region)
  {
    if (has_saved_region)
    {
      restore_region_pixels(saved_rect);
    }

    save_region_pixels(current_rect);

    Rect present_rect = current_rect;
    if (has_saved_region)
    {
      present_rect = rect_union(saved_rect, current_rect);
    }

    draw_blob_glow(blob_x, blob_y, px_new, py_new, OUTLINE_COLOR);
    draw_blob_outline(px_new, py_new, OUTLINE_COLOR);
    draw_blob_eyes(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, EYE_COLOR);
    draw_blob_mouth(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, MOUTH_COLOR);

    waveshare_native_present_window(present_rect.x0, present_rect.y0, present_rect.x1, present_rect.y1);

    saved_rect = current_rect;
    has_saved_region = true;
  }
  else
  {
    update_bounds(px_new, py_new, &min_x, &min_y, &max_x, &max_y);
    update_glow_bounds(blob_x, blob_y, px_new, py_new, &min_x, &min_y, &max_x, &max_y);
    update_eyes_bounds(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, &min_x, &min_y, &max_x, &max_y);
    update_mouth_bounds(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, &min_x, &min_y, &max_x, &max_y);

    if (!first_frame)
    {
      update_bounds(px_old, py_old, &min_x, &min_y, &max_x, &max_y);
      update_glow_bounds(prev_blob_x, prev_blob_y, px_old, py_old, &min_x, &min_y, &max_x, &max_y);
      update_eyes_bounds(prev_blob_x, prev_blob_y, prev_eye_dir_x, prev_eye_dir_y, prev_speed, prev_face_phase, &min_x, &min_y, &max_x, &max_y);
      update_mouth_bounds(prev_blob_x, prev_blob_y, prev_eye_dir_x, prev_eye_dir_y, prev_speed, prev_face_phase, &min_x, &min_y, &max_x, &max_y);
    }

    min_x = clamp_i16(min_x - DIRTY_MARGIN, 0, SCREEN_W - 1);
    min_y = clamp_i16(min_y - DIRTY_MARGIN, 0, SCREEN_H - 1);
    max_x = clamp_i16(max_x + DIRTY_MARGIN, 0, SCREEN_W - 1);
    max_y = clamp_i16(max_y + DIRTY_MARGIN, 0, SCREEN_H - 1);

    if (!first_frame)
    {
      draw_blob_glow(prev_blob_x, prev_blob_y, px_old, py_old, BG_COLOR);
      draw_blob_outline(px_old, py_old, BG_COLOR);
      draw_blob_eyes(prev_blob_x, prev_blob_y, prev_eye_dir_x, prev_eye_dir_y, prev_speed, prev_face_phase, BG_COLOR);
      draw_blob_mouth(prev_blob_x, prev_blob_y, prev_eye_dir_x, prev_eye_dir_y, prev_speed, prev_face_phase, BG_COLOR);
    }

    draw_blob_glow(blob_x, blob_y, px_new, py_new, OUTLINE_COLOR);
    draw_blob_outline(px_new, py_new, OUTLINE_COLOR);
    draw_blob_eyes(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, EYE_COLOR);
    draw_blob_mouth(blob_x, blob_y, eye_dir_x, eye_dir_y, speed_now, phase_t, MOUTH_COLOR);

    waveshare_native_present_window(min_x, min_y, max_x, max_y);
  }

  memcpy(px_old, px_new, sizeof(px_new));
  memcpy(py_old, py_new, sizeof(py_new));
  first_frame = false;
  face_phase_prev = phase_t;

  phase_t += 0.08f;
  delay(16);
}
