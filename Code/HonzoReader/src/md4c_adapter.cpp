#include "reader.h"
#include <md4c.h>
#include <cstring>

struct MdCtx {
    StyledRun* runs;
    int        max_runs;
    int        run_count;

    char*      pool;
    int        pool_cap;
    int        pool_used;

    // Current attributes for the run being built
    uint8_t  cur_heading;
    uint8_t  cur_style;
    uint8_t  cur_flags;
    uint16_t cur_indent;

    // Inline style stack
    uint8_t style_stack[16];
    int     style_sp;

    // Track start of current text content in pool for debounced run creation
    int      run_start;  // pool offset where current run's text begins (-1 if none)
};

static void push_style(MdCtx* ctx, uint8_t s) {
    if (ctx->style_sp < 16) {
        ctx->style_stack[ctx->style_sp++] = ctx->cur_style;
        if (s == 1)
            ctx->cur_style = (ctx->cur_style == 2) ? 3 : 1;
        else if (s == 2)
            ctx->cur_style = (ctx->cur_style == 1) ? 3 : 2;
    }
}

static void pop_style(MdCtx* ctx) {
    if (ctx->style_sp > 0)
        ctx->cur_style = ctx->style_stack[--ctx->style_sp];
}

// Commit the current text range (run_start..pool_used) as a StyledRun
static int commit_run(MdCtx* ctx) {
    if (ctx->run_start < 0) return 0;  // no text accumulated
    if (ctx->run_count >= ctx->max_runs) return -1;

    int len = ctx->pool_used - ctx->run_start;
    // Trim trailing NULs
    while (len > 0 && ctx->pool[ctx->run_start + len - 1] == '\0') len--;

    if (len == 0) {
        ctx->run_start = -1;
        return 0;
    }

    StyledRun* r = &ctx->runs[ctx->run_count];
    r->text    = ctx->pool + ctx->run_start;
    r->len     = len;
    r->font    = ctx->cur_style;
    r->heading = ctx->cur_heading;
    r->flags   = ctx->cur_flags;
    r->indent  = ctx->cur_indent;
    ctx->run_count++;
    ctx->run_start = -1;
    return 0;
}

// Append text to pool and start a run if not already started
static int append_text(MdCtx* ctx, const MD_CHAR* text, MD_SIZE size) {
    if (size == 0) return 0;
    if (ctx->pool_used + (int)size + 1 > ctx->pool_cap) {
        commit_run(ctx);
        if (ctx->pool_used + (int)size + 1 > ctx->pool_cap)
            return -1;
    }
    if (ctx->run_start < 0)
        ctx->run_start = ctx->pool_used;
    memcpy(ctx->pool + ctx->pool_used, text, size);
    ctx->pool_used += size;
    ctx->pool[ctx->pool_used] = '\0';
    ctx->pool_used++;
    return 0;
}

// Emit a newline run (for block breaks)
static int emit_newline(MdCtx* ctx) {
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

static int enter_block_cb(MD_BLOCKTYPE type, void* detail, void* userdata) {
    MdCtx* ctx = (MdCtx*)userdata;
    switch (type) {
        case MD_BLOCK_H: {
            MD_BLOCK_H_DETAIL* hd = (MD_BLOCK_H_DETAIL*)detail;
            ctx->cur_heading = (uint8_t)hd->level;
            break;
        }
        case MD_BLOCK_CODE:
            ctx->cur_flags |= 1;
            break;
        case MD_BLOCK_QUOTE:
            ctx->cur_flags |= 2;
            break;
        case MD_BLOCK_UL:
        case MD_BLOCK_OL:
            ctx->cur_indent += 10;
            break;
        case MD_BLOCK_LI:
            ctx->cur_flags |= 4;
            break;
        default:
            break;
    }
    return 0;
}

static int leave_block_cb(MD_BLOCKTYPE type, void* detail, void* userdata) {
    MdCtx* ctx = (MdCtx*)userdata;
    switch (type) {
        case MD_BLOCK_H:
            commit_run(ctx);
            emit_newline(ctx);
            emit_newline(ctx);  // blank line after heading
            ctx->cur_heading = 0;
            break;
        case MD_BLOCK_P:
        case MD_BLOCK_LI:
            commit_run(ctx);
            ctx->cur_flags &= ~4;
            break;
        case MD_BLOCK_CODE:
            commit_run(ctx);
            ctx->cur_flags &= ~1;
            break;
        case MD_BLOCK_QUOTE:
            commit_run(ctx);
            ctx->cur_flags &= ~2;
            break;
        case MD_BLOCK_UL:
        case MD_BLOCK_OL:
            ctx->cur_indent -= 10;
            break;
        default:
            break;
    }
    return 0;
}

static int enter_span_cb(MD_SPANTYPE type, void* detail, void* userdata) {
    MdCtx* ctx = (MdCtx*)userdata;
    switch (type) {
        case MD_SPAN_EM:         push_style(ctx, 2); break;
        case MD_SPAN_STRONG:     push_style(ctx, 1); break;
        case MD_SPAN_CODE:       commit_run(ctx); ctx->cur_flags |= 1; break;
        default: break;
    }
    return 0;
}

static int leave_span_cb(MD_SPANTYPE type, void* detail, void* userdata) {
    MdCtx* ctx = (MdCtx*)userdata;
    switch (type) {
        case MD_SPAN_EM:
        case MD_SPAN_STRONG:
            commit_run(ctx);
            pop_style(ctx);
            break;
        case MD_SPAN_CODE:
            commit_run(ctx);
            ctx->cur_flags &= ~1;
            break;
        default:
            break;
    }
    return 0;
}

static int text_cb(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) {
    MdCtx* ctx = (MdCtx*)userdata;
    switch (type) {
        case MD_TEXT_NORMAL:
        case MD_TEXT_ENTITY:
        case MD_TEXT_CODE:
            return append_text(ctx, text, size);
        case MD_TEXT_BR:
        case MD_TEXT_SOFTBR:
            commit_run(ctx);
            return emit_newline(ctx);
        case MD_TEXT_HTML:
            return 0;  // skip raw HTML
        default:
            return 0;
    }
}

int md4c_to_runs(const char* text, size_t len,
                  StyledRun* runs, int max_runs,
                  char* pool, int pool_cap)
{
    MdCtx ctx;
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
    ctx.style_sp    = 0;
    ctx.run_start   = -1;

    MD_PARSER parser;
    memset(&parser, 0, sizeof(parser));
    parser.abi_version = 0;
    parser.flags = MD_FLAG_COLLAPSEWHITESPACE;
    parser.enter_block = enter_block_cb;
    parser.leave_block = leave_block_cb;
    parser.enter_span  = enter_span_cb;
    parser.leave_span  = leave_span_cb;
    parser.text = text_cb;
    parser.debug_log = nullptr;
    parser.syntax = nullptr;

    int ret = md_parse(text, (MD_SIZE)len, &parser, &ctx);
    commit_run(&ctx);
    return (ret == 0) ? ctx.run_count : -1;
}
