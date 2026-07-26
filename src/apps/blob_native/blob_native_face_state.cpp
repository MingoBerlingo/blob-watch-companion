#include "apps/blob_native/blob_native_face_state.h"

#include <Arduino.h>

namespace blob_native
{

    static uint16_t random_blink_interval_ms()
    {
        return (uint16_t)random((long)FACE_BLINK_INTERVAL_MIN_MS, (long)FACE_BLINK_INTERVAL_MAX_MS + 1);
    }

    void face_anim_reset(FaceAnimState *state)
    {
        if (state == nullptr)
        {
            return;
        }

        state->eyes_state = EyesAnimState::Idle;
        state->mouth_state = MouthAnimState::Neutral;
        state->eyes_timer_ms = 0;
        state->mouth_timer_ms = 0;
        state->next_blink_ms = random_blink_interval_ms();
        state->double_blink_pending = false;
    }

    void face_anim_update(FaceAnimState *state, bool shake_event, bool click_event, uint16_t dt_ms)
    {
        if (state == nullptr)
        {
            return;
        }

        state->eyes_timer_ms = (uint16_t)(state->eyes_timer_ms + dt_ms);
        state->mouth_timer_ms = (uint16_t)(state->mouth_timer_ms + dt_ms);

        if (click_event)
        {
            state->mouth_state = MouthAnimState::OpenO;
            state->mouth_timer_ms = 0;
        }

        if (state->mouth_state == MouthAnimState::OpenO && state->mouth_timer_ms >= FACE_MOUTH_O_MS)
        {
            state->mouth_state = MouthAnimState::Neutral;
            state->mouth_timer_ms = 0;
        }

        if (shake_event)
        {
            state->eyes_state = EyesAnimState::ShakeX;
            state->eyes_timer_ms = 0;
            state->double_blink_pending = false;
            return;
        }

        switch (state->eyes_state)
        {
        case EyesAnimState::Idle:
            if (state->eyes_timer_ms >= state->next_blink_ms)
            {
                state->eyes_state = EyesAnimState::BlinkClosed;
                state->eyes_timer_ms = 0;
                state->double_blink_pending = (random(100) < FACE_DOUBLE_BLINK_CHANCE_PERCENT);
            }
            break;

        case EyesAnimState::BlinkClosed:
            if (state->eyes_timer_ms >= FACE_BLINK_CLOSED_MS)
            {
                state->eyes_state = EyesAnimState::BlinkOpen;
                state->eyes_timer_ms = 0;
            }
            break;

        case EyesAnimState::BlinkOpen:
            if (state->eyes_timer_ms >= FACE_BLINK_OPEN_MS)
            {
                if (state->double_blink_pending)
                {
                    state->eyes_state = EyesAnimState::DoubleBlinkGap;
                    state->eyes_timer_ms = 0;
                    state->double_blink_pending = false;
                }
                else
                {
                    state->eyes_state = EyesAnimState::Idle;
                    state->eyes_timer_ms = 0;
                    state->next_blink_ms = random_blink_interval_ms();
                }
            }
            break;

        case EyesAnimState::DoubleBlinkGap:
            if (state->eyes_timer_ms >= FACE_DOUBLE_BLINK_GAP_MS)
            {
                state->eyes_state = EyesAnimState::BlinkClosed;
                state->eyes_timer_ms = 0;
            }
            break;

        case EyesAnimState::ShakeX:
            if (state->eyes_timer_ms >= FACE_SHAKE_X_MS)
            {
                state->eyes_state = EyesAnimState::Idle;
                state->eyes_timer_ms = 0;
                state->next_blink_ms = random_blink_interval_ms();
            }
            break;
        }
    }

} // namespace blob_native
