#include <SPI.h>
#include <Wire.h>

#include "apps/blob_native/blob_native_app.h"

void setup() {
  blob_native_app_setup();
}

void loop() {
  blob_native_app_loop();
}
