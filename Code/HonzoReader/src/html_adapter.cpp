#include "reader.h"
#include <html_parser.h>
#include <cstring>

struct HtmlCtx {
    StyledRun* runs;
    int        max_runs;
    int        run_count;

    char*      pool;
    int        pool_cap;
    int        pool_used;

    uint8_t  cur_heading;
    uint8_t  cur_style;
    uint8_t  cur_flags;
    uint16_t cur_indent;

    // Text accumulation state
    int      text_start;  // pool offset of current text (-1 if none)
    bool     in_para;     // true inside <p> or block element
};

struct TagStack {
    char     tag[16];
    uint8_t  saved_heading;
    uint8_t  saved_style;
    uint8_t  saved_flags;
    uint16_t saved_indent;
    bool     saved_in_para;
};

static const int MAX_TAG_DEPTH = 32;
static TagStack stack[MAX_TAG_DEPTH];
static int sp = 0;

static int commit_text(HtmlCtx* ctx) {
    if (ctx->text_start < 0) return 0;
    if (ctx->run_count >= ctx->max_runs) return -1;

    int len = ctx->pool_used - ctx->text_start;
    while (len > 0 && ctx->pool[ctx->text_start + len - 1] == '\0') len--;
    if (len == 0) { ctx->text_start = -1; return 0; }

    StyledRun* r = &ctx->runs[ctx->run_count];
    r->text    = ctx->pool + ctx->text_start;
    r->len     = len;
    r->font    = ctx->cur_style;
    r->heading = ctx->cur_heading;
    r->flags   = ctx->cur_flags;
    r->indent  = ctx->cur_indent;
    ctx->run_count++;
    ctx->text_start = -1;
    return 0;
}

static int append_text(HtmlCtx* ctx, const char* text, int len) {
    if (len <= 0) return 0;
    if (ctx->pool_used + len + 1 > ctx->pool_cap) {
        commit_text(ctx);
        if (ctx->pool_used + len + 1 > ctx->pool_cap) return -1;
    }
    if (ctx->text_start < 0)
        ctx->text_start = ctx->pool_used;
    memcpy(ctx->pool + ctx->pool_used, text, len);
    ctx->pool_used += len;
    ctx->pool[ctx->pool_used] = '\0';
    ctx->pool_used++;
    return 0;
}

static int emit_newline(HtmlCtx* ctx) {
    if (ctx->run_count >= ctx->max_runs) return -1;
    if (ctx->pool_used + 2 > ctx->pool_cap) return -1;
    ctx->pool[ctx->pool_used] = '\n';
    ctx->pool_used++;
    ctx->pool[ctx->pool_used] = '\0';
    ctx->pool_used++;
    StyledRun* r = &ctx->runs[ctx->run_count];
    r->text    = ctx->pool + ctx->pool_used - 2;
    r->len     = 1;
    r->font    = ctx->cur_style;
    r->heading = ctx->cur_heading;
    r->flags   = ctx->cur_flags;
    r->indent  = ctx->cur_indent;
    ctx->run_count++;
    return 0;
}

static void tag_callback(struct html_tag* tag, void* userdata) {
    HtmlCtx* ctx = (HtmlCtx*)userdata;
    const char* name = tag->name;

    if (tag->is_end_tag) {
        if (sp == 0) return;

        // Extract text content between start and end tags
        if (tag->contents_begin && tag->contents_end) {
            int text_len = (int)(tag->contents_end - tag->contents_begin);
            const char* tstart = tag->contents_begin;
            // Trim leading whitespace
            while (text_len > 0 && (*tstart == ' ' || *tstart == '\n' || *tstart == '\r' || *tstart == '\t')) {
                tstart++; text_len--;
            }
            // Trim trailing whitespace
            while (text_len > 0 && (tstart[text_len-1] == ' ' || tstart[text_len-1] == '\n' || tstart[text_len-1] == '\r' || tstart[text_len-1] == '\t'))
                text_len--;
            if (text_len > 0) {
                if (strcmp(name, "script") != 0 && strcmp(name, "style") != 0)
                    append_text(ctx, tstart, text_len);
            }
        }

        sp--;
        TagStack* s = &stack[sp];
        ctx->cur_heading = s->saved_heading;
        ctx->cur_style   = s->saved_style;
        ctx->cur_flags   = s->saved_flags;
        ctx->cur_indent  = s->saved_indent;
        ctx->in_para     = s->saved_in_para;

        // On block exit, commit any pending text
        if (strcmp(name, "p") == 0 || strcmp(name, "h1") == 0 ||
            strcmp(name, "h2") == 0 || strcmp(name, "h3") == 0 ||
            strcmp(name, "h4") == 0 || strcmp(name, "h5") == 0 ||
            strcmp(name, "h6") == 0 || strcmp(name, "li") == 0 ||
            strcmp(name, "blockquote") == 0 || strcmp(name, "div") == 0 ||
            strcmp(name, "pre") == 0)
        {
            commit_text(ctx);
            emit_newline(ctx);
            ctx->in_para = false;
        }
        return;
    }

    // Opening tag
    if (sp >= MAX_TAG_DEPTH) return;
    TagStack* s = &stack[sp++];
    strncpy(s->tag, name, sizeof(s->tag) - 1);
    s->tag[sizeof(s->tag) - 1] = '\0';
    s->saved_heading = ctx->cur_heading;
    s->saved_style   = ctx->cur_style;
    s->saved_flags   = ctx->cur_flags;
    s->saved_indent  = ctx->cur_indent;
    s->saved_in_para = ctx->in_para;

    // Block-level elements flush text
    if (strcmp(name, "p") == 0 || strcmp(name, "h1") == 0 ||
        strcmp(name, "h2") == 0 || strcmp(name, "h3") == 0 ||
        strcmp(name, "h4") == 0 || strcmp(name, "h5") == 0 ||
        strcmp(name, "h6") == 0 || strcmp(name, "li") == 0 ||
        strcmp(name, "blockquote") == 0 || strcmp(name, "pre") == 0 ||
        strcmp(name, "div") == 0)
    {
        commit_text(ctx);
        ctx->in_para = true;
    }

    // Apply tag formatting
    if      (strcmp(name, "h1") == 0) ctx->cur_heading = 1;
    else if (strcmp(name, "h2") == 0) ctx->cur_heading = 2;
    else if (strcmp(name, "h3") == 0) ctx->cur_heading = 3;
    else if (strcmp(name, "h4") == 0) ctx->cur_heading = 4;
    else if (strcmp(name, "h5") == 0) ctx->cur_heading = 5;
    else if (strcmp(name, "h6") == 0) ctx->cur_heading = 6;
    else if (strcmp(name, "b") == 0 || strcmp(name, "strong") == 0)
        ctx->cur_style = (ctx->cur_style == 2) ? 3 : 1;
    else if (strcmp(name, "i") == 0 || strcmp(name, "em") == 0)
        ctx->cur_style = (ctx->cur_style == 1) ? 3 : 2;
    else if (strcmp(name, "code") == 0)
        ctx->cur_flags |= 1;
    else if (strcmp(name, "pre") == 0)
        ctx->cur_flags |= 1;
    else if (strcmp(name, "blockquote") == 0)
        ctx->cur_flags |= 2;
    else if (strcmp(name, "li") == 0)
        ctx->cur_flags |= 4;
    else if (strcmp(name, "ul") == 0 || strcmp(name, "ol") == 0)
        ctx->cur_indent += 10;
    else if (strcmp(name, "br") == 0) {
        commit_text(ctx);
        emit_newline(ctx);
    }
}

int html_to_runs(const char* text, size_t len,
                  StyledRun* runs, int max_runs,
                  char* pool, int pool_cap)
{
    HtmlCtx ctx;
    ctx.runs      = runs;
    ctx.max_runs  = max_runs;
    ctx.run_count = 0;
    ctx.pool      = pool;
    ctx.pool_cap  = pool_cap;
    ctx.pool_used = 0;
    ctx.cur_heading = 0;
    ctx.cur_style   = 0;
    ctx.cur_flags   = 0;
    ctx.cur_indent  = 0;
    ctx.text_start  = -1;
    ctx.in_para     = false;
    sp = 0;

    html_scan_tags(text, (int)len, tag_callback, &ctx, HTML_SCAN_TRIM_VALUES, NULL, NULL);

    commit_text(&ctx);
    return ctx.run_count;
}
