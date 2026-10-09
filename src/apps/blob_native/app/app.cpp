#include "apps/blob_native/app/app.h"

#include <Arduino.h>

#include "apps/blob_native/lv_port/lv_port.h"
#include "apps/blob_native/pages/page_manager.h"
#include "apps/blob_native/shared_state.h"
#include "platform/waveshare_native_board.h"

using namespace blob_native;

void blob_native_app_setup()
{
    Serial.begin(115200);
    if (!waveshare_native_begin())
    {
        Serial.println("Native board init failed");
        return;
    }

    waveshare_native_clear_display(BG_COLOR);

    lv_port_init();
    page_manager_init();
}

void blob_native_app_loop()
{
    page_manager_loop();
}
