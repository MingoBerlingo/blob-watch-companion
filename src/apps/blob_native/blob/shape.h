#ifndef BLOB_NATIVE_BLOB_H
#define BLOB_NATIVE_BLOB_H

#include <stdint.h>

namespace blob_native
{

    void compute_blob(float cx, float cy, float vx, float vy, float phase_t, int16_t *px, int16_t *py);
    void scale_blob_points(float cx, float cy, float scale, const int16_t *px_in, const int16_t *py_in, int16_t *px_out, int16_t *py_out);

} // namespace blob_native

#endif
