#include "apps/blob_native/app/app.h"

#include <Arduino.h>

#include "apps/blob_native/screen_logic.h"
#include "apps/blob_native/shared_state.h"
#include "platform/waveshare_native_board.h"

using namespace blob_native;

static_assert(!BLOB_STABLE_PROFILE_LOCKED || BLOB_RAW_BLOB_MODE,
              "Stable profile lock requires BLOB_RAW_BLOB_MODE=true");

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
    blob_screen_manager_reset();
}

void blob_native_app_loop()
{
    const uint32_t frame_start_us = micros();
    const uint32_t now_ms = millis();
    blob_screen_manager_loop(frame_start_us, now_ms);
}
