#include <globals.h>
#include "reader.h"

static constexpr int32_t SCROLL_THRESHOLD = 80;
static constexpr uint32_t SCROLL_COOLDOWN = 300;

static uint32_t s_lastTouchMs = 0;
static int32_t  s_lastScroll  = 0;

void touch_init() {
    g_touchBase     = 0;
    g_scrollAccum   = 0;
    s_lastTouchMs   = 0;
    s_lastScroll    = 0;
}

void touch_scroll() {
    TOUCH().updateScrollFromTouch();
    int32_t current = TOUCH().getDynamicScroll();
    uint32_t now = millis();

    if (now - s_lastTouchMs < SCROLL_COOLDOWN) return;

    int32_t delta = current - s_lastScroll;

    if (delta > SCROLL_THRESHOLD) {
        if (g_curPage + 1 < g_pagesInChapter) {
            g_curPage++;
            g_needsRedraw = true;
            s_lastScroll = current;
            s_lastTouchMs = now;
        }
    } else if (delta < -SCROLL_THRESHOLD) {
        if (g_curPage > 0) {
            g_curPage--;
            g_needsRedraw = true;
            s_lastScroll = current;
            s_lastTouchMs = now;
        }
    }

    // Reset baseline when idle
    if (abs(delta) < 15) {
        s_lastScroll = current;
    }
}
