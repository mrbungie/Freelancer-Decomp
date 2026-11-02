// Mouse input as the game consumes it (replaces the DirectInput8 mouse device).
// Implemented by the front end (src/platform/frontend/sdl/sdl_mouse.cpp); obtained through
// IWindowHost::CreateMouse(). Keyboard input arrives as window messages / text events, not here.
#pragma once

namespace fl
{
    struct MouseEvent
    {
        enum Control : unsigned { X, Y, Wheel, Button0, Button7 = Button0 + 7 };
        unsigned control;       // Control; Button0 + n for button n
        int value;              // axes: movement since the previous event (wheel in 1/120 notch units);
                                // buttons: 1 pressed, 0 released
        unsigned timestampMs;   // monotonic, same clock as IClock::MonotonicMilliseconds
    };

    struct MouseState
    {
        int x, y, wheel;        // movement accumulated since the previous ReadState
        unsigned char down[8];  // non-zero while button n is held
    };

    class IMouse
    {
    public:
        virtual ~IMouse() {}
        // Starts delivering input. False when it cannot (e.g. the window is not in front).
        virtual bool Acquire() = 0;
        virtual void Unacquire() = 0;
        // Capture the pointer for the window (relative movement, hidden/confined) or release it.
        virtual bool SetExclusive(void* window, bool exclusive) = 0;
        // 0 = no event queue (use ReadState only). Otherwise queue up to `capacity` events.
        virtual bool SetEventBuffer(unsigned capacity) = 0;
        // Pops up to *count queued events into `out`; *count receives how many were written.
        virtual bool ReadEvents(MouseEvent* out, unsigned* count) = 0;
        virtual bool ReadState(MouseState* out) = 0;
        // Refreshes from the OS; called once per frame before reading.
        virtual void Poll() = 0;
        virtual void Destroy() = 0;
    };
}
