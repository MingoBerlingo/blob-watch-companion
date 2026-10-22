#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
uint32_t millis(void);
void delay(uint32_t milliseconds);
#ifdef __cplusplus
}
template <typename T> constexpr T constrain(T value, T minimum, T maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}
#endif
#ifndef PI
#define PI 3.14159265358979323846
#endif
