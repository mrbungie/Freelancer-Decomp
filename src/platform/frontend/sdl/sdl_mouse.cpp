// fl::IMouse over SDL (via fl_sdl.h, so it works for either SDL generation).
#include "fl_sdl.h"
#include "fl/input.h"
#include <deque>
#include <stdio.h>
#include <stdlib.h>

namespace
{
    class SdlMouse : public fl::IMouse
    {
        bool acquired_ = false;
        unsigned capacity_ = 0;
        std::deque<fl::MouseEvent> queue_;
        unsigned prevButtons_ = 0;
        long accX_ = 0, accY_ = 0, accWheel_ = 0;   // for ReadState

        void Push(unsigned control, int value, unsigned t)
        {
            if (capacity_ == 0) return;
            if (queue_.size() >= capacity_) queue_.pop_front();   // overflow drops the oldest
            queue_.push_back({control, value, t});
        }
        void Sample()
        {
            if (!acquired_) return;
            int dx, dy, wheel; unsigned buttons;
            if (!fl_sdl_mouse_state(&dx, &dy, &wheel, &buttons)) return;
            unsigned t = fl_sdl_ticks();
            if (dx) { Push(fl::MouseEvent::X, dx, t); accX_ += dx; }
            if (dy) { Push(fl::MouseEvent::Y, dy, t); accY_ += dy; }
            if (wheel) { Push(fl::MouseEvent::Wheel, wheel * 120, t); accWheel_ += wheel * 120; }
            if (getenv("FL_TRACE_INPUT") && (dx || dy || buttons != prevButtons_)) fprintf(stderr, "[fl] mouse dx=%d dy=%d buttons=%u\n", dx, dy, buttons);
            unsigned changed = buttons ^ prevButtons_;
            for (unsigned b = 0; b < 8; ++b)
                if (changed & (1u << b)) Push(fl::MouseEvent::Button0 + b, (buttons >> b) & 1u, t);
            prevButtons_ = buttons;
        }
    public:
        bool Acquire() override
        {
            if (!acquired_)
            {   // discard movement that happened while we were not listening
                int a, b, c; unsigned d;
                fl_sdl_mouse_state(&a, &b, &c, &d);
                prevButtons_ = d;
                accX_ = accY_ = accWheel_ = 0;
                queue_.clear();
            }
            acquired_ = true;
            return true;
        }
        void Unacquire() override { acquired_ = false; queue_.clear(); }
        bool SetExclusive(void* window, bool exclusive) override { fl_sdl_set_relative_mouse(window, exclusive); return true; }
        bool SetEventBuffer(unsigned capacity) override { capacity_ = capacity; if (!capacity) queue_.clear(); return true; }
        bool ReadEvents(fl::MouseEvent* out, unsigned* count) override
        {
            if (!acquired_) { *count = 0; return false; }
            Sample();
            unsigned n = 0;
            while (n < *count && !queue_.empty()) { out[n++] = queue_.front(); queue_.pop_front(); }
            *count = n;
            return true;
        }
        bool ReadState(fl::MouseState* s) override
        {
            if (!acquired_) return false;
            Sample();
            s->x = (int)accX_; s->y = (int)accY_; s->wheel = (int)accWheel_;
            accX_ = accY_ = accWheel_ = 0;
            for (unsigned b = 0; b < 8; ++b) s->down[b] = (prevButtons_ >> b) & 1u;
            return true;
        }
        void Poll() override { Sample(); }
        void Destroy() override { delete this; }
    };
}

fl::IMouse* FlSdlCreateMouse() { return new SdlMouse(); }
