#include <globals.h>
#include <SD_MMC.h>
#include <Fonts/FreeSerif9pt8b.h>
#include <cpp/HonzoFileReader.hpp>
#include <memory>
#include <string>
#include <vector>
#include "reader.h"

// BookReader wraps HonzoFileReader with chapter-index cache (RAII)
struct HonzoChapter {
    const char* text;
    size_t      len;
    uint8_t     markup;  // 0=Markdown, 1=HTML
};

class BookReader {
public:
    ~BookReader() { close(); }

    BookReader() = default;
    BookReader(const BookReader&) = delete;
    BookReader& operator=(const BookReader&) = delete;

    bool open(const char* path) {
        close();

        auto outer = HonzoFileReader::open(std::string_view(path), 1);
        if (!outer.is_ok()) return false;

        auto inner = std::move(outer).ok();
        if (!inner || !(*inner).is_ok()) return false;

        auto file = std::move(*std::move(*inner).ok());
        if (!file) return false;

        uint32_t total = file->chunk_count();
        if (total == 0) return false;

        for (uint32_t i = 0; i < total; i++) {
            uint32_t tag = file->get_chunk_type(i);
            if (tag == 0x50414843) {          // "CHAP" as little-endian tag
                uint8_t kind = file->get_chunk_content_type_kind(i);
                uint8_t val  = file->get_chunk_content_type_value(i);
                chap_indices_.push_back(i);
                chap_markup_.push_back((kind == 1 && val == 1) ? 1 : 0);
            }
        }

        chapter_count_ = chap_indices_.size();
        if (chapter_count_ == 0) return false;

        reader_ = std::move(file);
        return true;
    }

    void close() {
        reader_.reset();
        chap_indices_.clear();
        chap_markup_.clear();
        chapter_count_ = 0;
    }

    uint32_t chapter_count() const { return chapter_count_; }

    bool load_chapter(uint32_t idx, HonzoChapter& out) {
        if (!reader_ || idx >= chapter_count_) return false;
        uint32_t toc_idx = chap_indices_[idx];
        auto chunk = reader_->get_chunk(toc_idx);
        if (!chunk.has_value()) return false;
        out.text   = reinterpret_cast<const char*>(chunk->data());
        out.len    = chunk->size();
        out.markup = chap_markup_[idx];
        return true;
    }

    std::string get_meta_json() {
        if (!reader_) return {};
        auto result = reader_->get_meta();
        if (!result.is_ok()) return {};
        return std::move(result).ok().value();
    }

private:
    std::unique_ptr<HonzoFileReader> reader_;
    uint32_t chapter_count_ = 0;
    std::vector<uint32_t> chap_indices_;
    std::vector<uint8_t>  chap_markup_;
};

static BookReader s_book;

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
    Serial.printf("[open_book] filename=\"%s\"\n", filename);

    char full[256];
    snprintf(full, sizeof(full), "/sdcard/books/%s", filename);

    g_appMode = MODE_READING;

    s_book.close();
    bool ok = s_book.open(full);
    Serial.printf("[open_book] BookReader::open returned %d\n", ok);
    if (!ok) {
        g_chapterCount = 0;
        g_needsRedraw = true;
        return;
    }

    g_chapterCount = (uint16_t)s_book.chapter_count();
    g_totalWords   = 0;
    g_totalPages   = 0;

    // Parse META JSON for TOC titles
    {
        auto metaStr = s_book.get_meta_json();
        if (!metaStr.empty()) {
            g_tocCount = 0;
            const char* p = metaStr.c_str();
            while (*p && *p != '[') p++;
            if (*p == '[') {
                p++;
                while (*p && g_tocCount < 64) {
                    p = skip_ws(p);
                    if (*p == ']') break;
                    if (*p != '{') { p++; continue; }

                    int ci = -1;
                    char title[64] = {};
                    p++;
                    while (*p && *p != '}') {
                        p = skip_ws(p);
                        if (*p != '"') { p++; continue; }
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
        }
    }

    Serial.printf("[open_book] toc_count=%u\n", g_tocCount);

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
    if (chapter >= g_chapterCount) {
        chapter = g_chapterCount > 0 ? g_chapterCount - 1 : 0;
    }

    g_curChapter = chapter;

    HonzoChapter hc;
    if (!s_book.load_chapter(chapter, hc)) {
        Serial.printf("[load_chapter] FAILED to load chapter %u\n", chapter);
        return;
    }

    render_chapter_markup(hc.text, hc.len, hc.markup);

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
        s_book.close();
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
    if (g_chapterCount == 0) {
        Serial.printf("[reader_render] NO CHAPTERS\n");
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
