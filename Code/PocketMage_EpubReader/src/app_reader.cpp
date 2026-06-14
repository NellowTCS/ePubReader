#include <globals.h>
#include <SD_MMC.h>
#include <Fonts/FreeSerif9pt8b.h>
#include "reader.h"

#include "EpubExtractor.h"
using namespace capi;

static EpubExtractor* s_epub = nullptr;
static char*          s_chapterJson = nullptr;

// TOC JSON parser helpers
static const char* skip_ws(const char* p) {
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    return p;
}

static const char* scan_string_val(const char* p, char* out, int maxLen) {
    if (*p != '"') return p;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxLen - 1) {
        if (*p == '\\' && *(p+1)) { p++; }
        out[i++] = *p;
        p++;
    }
    out[i] = '\0';
    if (*p == '"') p++;
    return p;
}

static const char* scan_int_val(const char* p, int* out) {
    *out = 0;
    bool neg = false;
    p = skip_ws(p);
    if (*p == '-') { neg = true; p++; }
    while (*p >= '0' && *p <= '9') {
        *out = *out * 10 + (*p - '0');
        p++;
    }
    if (neg) *out = -*out;
    return p;
}

// Open book
void open_book(const char* filename) {
    if (s_epub) { EpubExtractor_destroy(s_epub); s_epub = nullptr; }

    char full[96];
    snprintf(full, sizeof(full), "/books/%s", filename);
    g_appMode = MODE_READING;

    s_epub = EpubExtractor_create(full, strlen(full));
    if (!s_epub) {
        g_chapterCount = 0;
        g_needsRedraw = true;
        return;
    }

    g_chapterCount = (uint16_t)EpubExtractor_get_chapter_count(s_epub);
    g_totalWords   = (uint32_t)EpubExtractor_get_total_word_count(s_epub);
    g_totalPages   = 0;

    // Extract TOC
    char tocBuf[8192];
    DiplomatWriteable tocW = diplomat_simple_write(tocBuf, sizeof(tocBuf));
    auto tocResult = EpubExtractor_get_toc_json(s_epub, &tocW);
    (void)tocResult;

    // Parse TOC JSON - manual lightweight parser
    g_tocCount = 0;
    const char* p = tocBuf;
    while (*p && *p != '[') p++;
    if (*p == '[') {
        p++;
        while (*p && g_tocCount < 256) {
            p = skip_ws(p);
            if (*p == ']') break;
            if (*p != '{') { p++; continue; }

            int ci = -1;
            char title[128] = {};
            p++;
            while (*p && *p != '}') {
                p = skip_ws(p);
                if (*p != '"') { p++; continue; }

                // Read key
                char key[32] = {};
                p = scan_string_val(p, key, sizeof(key));
                p = skip_ws(p);
                if (*p == ':') p++;
                p = skip_ws(p);

                if (strcmp(key, "chapter_index") == 0) {
                    p = scan_int_val(p, &ci);
                } else if (strcmp(key, "title") == 0) {
                    p = scan_string_val(p, title, sizeof(title));
                } else {
                    // Skip unknown value
                    if (*p == '"') {
                        char dummy[8];
                        p = scan_string_val(p, dummy, sizeof(dummy));
                    } else {
                        while (*p && *p != ',' && *p != '}') p++;
                    }
                }
                p = skip_ws(p);
                if (*p == ',') p++;
            }
            if (*p == '}') p++;

            if (ci >= 0) {
                g_toc[g_tocCount].index = (uint16_t)ci;
                strncpy(g_toc[g_tocCount].title, title, sizeof(g_toc[0].title) - 1);
                g_tocCount++;
            }
            p = skip_ws(p);
            if (*p == ',') p++;
        }
    }

    // Restore bookmark
    Bookmark bm;
    if (loadBookmark(&bm)) {
        g_curChapter = bm.chapter;
        g_curPage    = bm.page;
    } else {
        g_curChapter = 0;
        g_curPage    = 0;
    }

    reader_load_chapter(g_curChapter);
    g_needsRedraw = true;
}

    // Load a chapter
void reader_load_chapter(uint16_t chapter) {
    if (chapter >= g_chapterCount) chapter = g_chapterCount > 0 ? g_chapterCount - 1 : 0;
    if (!s_epub) return;

    g_curChapter = chapter;

    // Allocate chapter JSON buffer on first use (heap)
    if (!s_chapterJson) {
        s_chapterJson = (char*)malloc(65536);
        if (!s_chapterJson) return;
    }

    DiplomatWriteable cw = diplomat_simple_write(s_chapterJson, 65535);
    auto result = EpubExtractor_get_chapter_json(s_epub, chapter, &cw);
    if (!result.is_ok) return;

    s_chapterJson[cw.len] = '\0';
    render_chapter(s_chapterJson);

    if (g_curPage >= g_pagesInChapter) g_curPage = 0;
}

// Process keys
void reader_process_key(char ch) {
    if (ch == 21) {  // RIGHT -> next page
        if (g_curPage + 1 < g_pagesInChapter) {
            g_curPage++;
            g_needsRedraw = true;
        } else if (g_curChapter + 1 < g_chapterCount) {
            g_curChapter++;
            g_curPage = 0;
            reader_load_chapter(g_curChapter);
            saveBookmark();
            g_needsRedraw = true;
        }
    } else if (ch == 19) {  // LEFT -> prev page
        if (g_curPage > 0) {
            g_curPage--;
            g_needsRedraw = true;
        } else if (g_curChapter > 0) {
            g_curChapter--;
            reader_load_chapter(g_curChapter);
            g_curPage = g_pagesInChapter > 0 ? g_pagesInChapter - 1 : 0;
            saveBookmark();
            g_needsRedraw = true;
        }
    } else if (ch == 24) {  // UP -> prev chapter
        if (g_curChapter > 0) {
            g_curChapter--;
            g_curPage = 0;
            reader_load_chapter(g_curChapter);
            saveBookmark();
            g_needsRedraw = true;
        }
    } else if (ch == 25) {  // DOWN -> next chapter
        if (g_curChapter + 1 < g_chapterCount) {
            g_curChapter++;
            g_curPage = 0;
            reader_load_chapter(g_curChapter);
            saveBookmark();
            g_needsRedraw = true;
        }
    } else if (ch == 'b') {
        saveBookmark();
    } else if (ch == 't') {
        g_tocScroll = 0;
        g_appMode = MODE_TOC;
        g_needsRedraw = true;
    } else if (ch == 'h') {
        saveBookmark();
        SD_MMC.remove(CUR_PATH);
        if (s_epub) { EpubExtractor_destroy(s_epub); s_epub = nullptr; }
        library_init();
    } else if (ch == 'j') {
        g_jumpLen = 0;
        g_jumpBuf[0] = '\0';
        g_appMode = MODE_JUMP;
    } else if (ch == '?') {
        g_appMode = MODE_HELP;
        g_needsRedraw = true;
    }
}

// Reader render
void reader_render() {

    if (!s_epub || g_chapterCount == 0) {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeSerif9pt8b);
        display.setCursor(10, 60);
        display.print("Cannot open book");
        EINK().refresh();
        return;
    }

    render_page_to_eink(g_curPage);

    // Header overlay
    display.setFont(&Font5x7Fixed);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 11);

    const char* title = "";
    for (uint16_t i = 0; i < g_tocCount; i++) {
        if (g_toc[i].index == g_curChapter) {
            title = g_toc[i].title;
            break;
        }
    }
    String hdr = title;
    if ((int)hdr.length() > 42) hdr = hdr.substring(0, 40) + "~";
    display.print(hdr);

    // Clock
    if (SYSTEM_CLOCK) {
        DateTime now = CLOCK().nowDT();
        char timeBuf[8];
        snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", now.hour(), now.minute());
        int16_t tx1, ty1; uint16_t tw, th;
        display.getTextBounds(timeBuf, 0, 0, &tx1, &ty1, &tw, &th);
        display.setCursor(display.width() - (int)tw - 4, 11);
        display.print(timeBuf);
    }

    // Progress bar
    int py = display.height() - 3;
    int progW = display.width() - 16;
    display.drawRect(8, py - 6, progW, 5, GxEPD_BLACK);
    if (g_chapterCount > 0) {
        uint32_t globalPageNum = g_curPage;
        // Estimate total: each chapter roughly equal size
        uint32_t estTotal = g_pagesInChapter * g_chapterCount;
        if (estTotal > 0) {
            int filled = (int)((progW * (int64_t)globalPageNum) / estTotal);
            if (filled > progW) filled = progW;
            if (filled > 0) display.fillRect(8 + 1, py - 5, filled, 3, GxEPD_BLACK);
        }
    }

    // Bottom info
    display.setFont(&Font5x7Fixed);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, display.height() - 14);
    display.printf("Ch %d/%d", g_curChapter + 1, g_chapterCount);
    if (g_pagesInChapter > 0) {
        display.setCursor(display.width() - 68, display.height() - 14);
        display.printf("p %d/%d", g_curPage + 1, g_pagesInChapter);
    }

    EINK().refresh();
}
