// Input.hpp — small helpers for held-button behaviour.
//
// libtesla calls handleInput() once per frame with keysDown (edge) and keysHeld
// (level). A Repeater turns "held" into a stream of events: one immediately, then
// after `delay` ms one every `interval` ms, speeding up after `turbo` ms.
#pragma once
#include <switch.h>

namespace input {
    inline u64 nowMs() {
        return armTicksToNs(armGetSystemTick()) / 1000000ULL;
    }

    class Repeater {
    public:
        Repeater(u32 delayMs = 280, u32 intervalMs = 45, u32 turboAfterMs = 1500, u32 turboIntervalMs = 16)
            : m_delay(delayMs), m_interval(intervalMs), m_turboAfter(turboAfterMs), m_turboInterval(turboIntervalMs) {}

        // Feed the current state of the button(s) this repeater watches.
        // Returns how many events to fire this frame (0, 1, or more if turbo).
        u32 update(bool held) {
            u64 t = nowMs();
            if (!held) { m_held = false; return 0; }
            if (!m_held) {
                m_held = true;
                m_start = t;
                m_last = t;
                return 1;
            }
            u64 sinceStart = t - m_start;
            if (sinceStart < m_delay) return 0;
            u32 interval = (sinceStart >= m_turboAfter) ? m_turboInterval : m_interval;
            if (t - m_last >= interval) {
                m_last = t;
                return (sinceStart >= m_turboAfter * 2) ? 3 : 1;
            }
            return 0;
        }

        void reset() { m_held = false; }

    private:
        u32 m_delay, m_interval, m_turboAfter, m_turboInterval;
        bool m_held = false;
        u64 m_start = 0, m_last = 0;
    };

    // Button groups we use (D-pad + left stick, never the right stick).
    constexpr u64 NAV_UP    = HidNpadButton_Up    | HidNpadButton_StickLUp;
    constexpr u64 NAV_DOWN  = HidNpadButton_Down  | HidNpadButton_StickLDown;
    constexpr u64 NAV_LEFT  = HidNpadButton_Left  | HidNpadButton_StickLLeft;
    constexpr u64 NAV_RIGHT = HidNpadButton_Right | HidNpadButton_StickLRight;
}
