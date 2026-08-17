/* brtrace_runtime.c - records the ordered sequence of (site_id, taken).
 *
 * Link this into both the fp32 and fp64 builds. Each run appends binary
 * records to the file named by the BRTRACE_OUT env var (default: brtrace.out).
 *
 * Record format (little-endian, fixed 8 bytes):
 *     uint32_t site_id
 *     int32_t  taken
 *
 * Binary + fixed-width keeps the trace compact and makes the diff a simple
 * lock-step stream compare. A trailing flush happens atexit.
 *
 * Build:
 *     clang -O2 -c brtrace_runtime.c -o brtrace_runtime.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static FILE *g_fp = NULL;
static int   g_init_done = 0;

typedef struct { uint32_t site_id; int32_t taken; } brrec_t;

/* Buffer to avoid a syscall per branch. */
#define BRBUF_N 8192
static brrec_t g_buf[BRBUF_N];
static size_t  g_n = 0;

static void brtrace_flush(void) {
    if (g_fp && g_n) {
        fwrite(g_buf, sizeof(brrec_t), g_n, g_fp);
        g_n = 0;
    }
    if (g_fp) fflush(g_fp);
}

static void brtrace_close(void) {
    brtrace_flush();
    if (g_fp) { fclose(g_fp); g_fp = NULL; }
}

static void brtrace_init(void) {
    const char *path = getenv("BRTRACE_OUT");
    if (!path || !*path) path = "brtrace.out";
    g_fp = fopen(path, "wb");
    if (!g_fp) {
        fprintf(stderr, "[brtrace] cannot open %s for writing\n", path);
    }
    atexit(brtrace_close);
    g_init_done = 1;
}

/* Called by every instrumented branch. Hot path -> keep it tiny. */
void __brtrace_log(uint32_t site_id, int32_t taken) {
    if (!g_init_done) brtrace_init();
    if (!g_fp) return;
    if (g_n == BRBUF_N) brtrace_flush();
    g_buf[g_n].site_id = site_id;
    g_buf[g_n].taken   = taken;
    ++g_n;
}

/* ---------------------------------------------------------------------- *
 * Select stream.
 *
 * Separate file, separate buffer, separate init. The branch stream written to
 * BRTRACE_OUT is therefore byte-identical to what it was before selects were
 * instrumented, so previously collected traces stay valid and lock-step
 * adjudication of the branch stream is untouched.
 *
 * Same 8-byte record layout, written to $BRTRACE_SEL_OUT (default
 * brtrace_sel.out).
 * ---------------------------------------------------------------------- */

static FILE *g_sel_fp = NULL;
static int   g_sel_init_done = 0;
static brrec_t g_sel_buf[BRBUF_N];
static size_t  g_sel_n = 0;

static void brtrace_sel_flush(void) {
    if (g_sel_fp && g_sel_n) {
        fwrite(g_sel_buf, sizeof(brrec_t), g_sel_n, g_sel_fp);
        g_sel_n = 0;
    }
    if (g_sel_fp) fflush(g_sel_fp);
}

static void brtrace_sel_close(void) {
    brtrace_sel_flush();
    if (g_sel_fp) { fclose(g_sel_fp); g_sel_fp = NULL; }
}

static void brtrace_sel_init(void) {
    const char *path = getenv("BRTRACE_SEL_OUT");
    if (!path || !*path) path = "brtrace_sel.out";
    g_sel_fp = fopen(path, "wb");
    if (!g_sel_fp) {
        fprintf(stderr, "[brtrace] cannot open %s for writing\n", path);
    }
    atexit(brtrace_sel_close);
    g_sel_init_done = 1;
}

/* Called by every instrumented select. Fires more often than the branch hook
 * in numeric code, so the buffering matters more here. */
void __brtrace_log_select(uint32_t sel_id, int32_t taken) {
    if (!g_sel_init_done) brtrace_sel_init();
    if (!g_sel_fp) return;
    if (g_sel_n == BRBUF_N) brtrace_sel_flush();
    g_sel_buf[g_sel_n].site_id = sel_id;
    g_sel_buf[g_sel_n].taken   = taken;
    ++g_sel_n;
}