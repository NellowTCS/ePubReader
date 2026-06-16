#include <globals.h>
#include <Fonts/FreeSerifBold12pt8b.h>
#include <Fonts/FreeSerif9pt8b.h>
#include "reader.h"

// TOC overlay
void toc_process_key(char ch) {
    if (ch == 25 || ch == 21) {  // DOWN or RIGHT -> next
        if (g_tocScroll < g_tocCount - 1) {
            g_tocScroll++;
            g_needsRedraw = true;
        }
    } else if (ch == 24 || ch == 19) {  // UP or LEFT -> prev
        if (g_tocScroll > 0) {
            g_tocScroll--;
            g_needsRedraw = true;
        }
    } else if (ch == 32 || ch == 13) {  // Space or Enter -> go to selected chapter
        if (g_tocCount > 0 && g_tocScroll < g_tocCount) {
            uint16_t targetIdx = g_toc[g_tocScroll].index;
            if (targetIdx != g_curChapter) {
                g_curChapter = targetIdx;
                g_curPage = 0;
                reader_load_chapter(g_curChapter);
                saveBookmark();
            }
            g_appMode = MODE_READING;
            g_needsRedraw = true;
        }
    } else if (ch == 27) {  // ESC -> back to reading
        g_appMode = MODE_READING;
        g_needsRedraw = true;
    }
}

void toc_render() {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSerifBold12pt8b);
    display.setCursor(8, 18);
    display.print("Table of Contents");
    display.drawFastHLine(0, 22, display.width(), GxEPD_BLACK);

    if (g_tocCount == 0) {
        display.setFont(&FreeSerif9pt8b);
        display.setCursor(8, 55);
        display.print("No TOC available");
        EINK().refresh();
        return;
    }

    display.setFont(&FreeSerif9pt8b);
    int y = 38;
    int start = g_tocScroll;
    if (start > 10) start = g_tocScroll - 5;
    int end = start + 13;
    if (end > g_tocCount) end = g_tocCount;

    for (int i = start; i < end; i++) {
        int x = 8;
        if (i == (int)g_tocScroll) {
            display.fillRect(0, y - 10, display.width(), 16, GxEPD_BLACK);
            display.setTextColor(GxEPD_WHITE);
        } else {
            display.setTextColor(GxEPD_BLACK);
        }
        display.setCursor(x, y);

        String t = g_toc[i].title;
        if ((int)t.length() > 40) t = t.substring(0, 38) + "..";
        display.print(t);
        y += 16;
    }

    display.setTextColor(GxEPD_BLACK);
    display.setFont(&Font5x7Fixed);
    display.setCursor(4, display.height() - 4);
    display.print("Arrow keys browse  Enter open  ESC back  FN+< exit");

    EINK().refresh();
}

//  Jump to page 
void jump_process_key(char ch) {
    if (ch >= '0' && ch <= '9') {
        if (g_jumpLen < 6) {
            g_jumpBuf[g_jumpLen++] = ch;
            g_jumpBuf[g_jumpLen] = '\0';
            g_needsRedraw = true;
        }
    } else if (ch == 13 || ch == 32) {  // Enter or Space
        if (g_jumpLen > 0) {
            int target = atoi(g_jumpBuf);
            if (target < 1) target = 1;
            if (g_pageCount > 0 && target > (int)g_pageCount) target = g_pageCount;
            g_curPage = target - 1;
            g_appMode = MODE_READING;
            g_needsRedraw = true;
        }
    } else if (ch == 27 || ch == 8) {  // ESC or Backspace
        g_appMode = MODE_READING;
        g_needsRedraw = true;
    }
}

void jump_render() {
    // Overlay on top of current page
    render_page_to_eink(g_curPage);

    int16_t dw = 280, dh = 100;
    int16_t dx = (display.width() - dw) / 2;
    int16_t dy = (display.height() - dh) / 2 - 10;

    display.fillRect(dx, dy, dw, dh, GxEPD_WHITE);
    display.drawRect(dx, dy, dw, dh, GxEPD_BLACK);

    display.setFont(&FreeSerifBold12pt8b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(dx + 12, dy + 24);
    display.print("Jump to Page");

    display.setFont(&FreeSerif9pt8b);
    display.setCursor(dx + 12, dy + 52);
    display.printf("Page (1-%u): ", g_pageCount);
    if (g_jumpLen > 0) {
        display.setCursor(dx + 12, dy + 80);
        display.print(g_jumpBuf);
    }

    display.setFont(&Font5x7Fixed);
    display.setCursor(4, display.height() - 4);
    display.print("Number + Enter to jump  ESC cancel");

    EINK().refresh();
}

//  Help screen 
void help_render() {
    display.fillScreen(GxEPD_WHITE);

    display.setFont(&FreeSerifBold12pt8b);
    display.setCursor(8, 20);
    display.print("Controls");

    display.drawFastHLine(0, 24, display.width(), GxEPD_BLACK);

    struct Row { const char* key; const char* desc; };
    Row rows[] = {
        { "< / >",   "Previous / next page"     },
        { "UP/DOWN", "Previous / next chapter"   },
        { "b",       "Save bookmark"             },
        { "t",       "Table of contents"         },
        { "j",       "Jump to page"              },
        { "h",       "Return to book picker"     },
        { "?",       "Show this screen"          },
        { "FN + <",  "Save & exit to OS"         },
        { "Touch",   "Scroll within chapter"     },
    };

    display.setFont(&Font5x7Fixed);
    int y = 38;
    for (auto& r : rows) {
        display.setCursor(8,  y);   display.print(r.key);
        display.setCursor(80, y);   display.print(r.desc);
        y += 14;
    }

    display.drawFastHLine(0, y + 4, display.width(), GxEPD_BLACK);
    display.setCursor(8, y + 14);
    display.print("Press any key to close");

    EINK().refresh();
}
