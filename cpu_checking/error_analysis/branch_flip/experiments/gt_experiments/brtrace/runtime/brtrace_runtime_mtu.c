/* brtrace_runtime_mtu.c - multi-TU branch trace recorder.
 *
 * Record (little-endian, 12 bytes):
 *     uint32_t module_id
 *     uint32_t site_id
 *     int32_t  taken
 *
 * (module_id, site_id) is globally unique across all linked TUs, so traces
 * from a multi-file binary are unambiguous.
 *
 * Two independent streams:
 *     branches -> $BRTRACE_OUT      (default brtrace.out)
 *     selects  -> $BRTRACE_SEL_OUT  (default brtrace_sel.out)
 *
 * They are kept separate on purpose. The branch stream is byte-identical to
 * what it was before select instrumentation existed, so previously collected
 * traces stay valid and lock-step adjudication of the branch stream is
 * untouched. A run with no selects simply produces an empty select file.
 *
 * Build:
 *     clang -O2 -c brtrace_runtime_mtu.c -o brtrace_runtime_mtu.o
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct { uint32_t module_id; uint32_t site_id; int32_t taken; } brrec_t;

#define BRBUF_N 8192

/* ---------------------------------------------------------------- branches */

static FILE *g_fp = NULL;
static int   g_init = 0;
static brrec_t g_buf[BRBUF_N];
static size_t  g_n = 0;

static void brtrace_flush(void) {
    if (g_fp && g_n) { fwrite(g_buf, sizeof(brrec_t), g_n, g_fp); g_n = 0; }
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
    if (!g_fp) fprintf(stderr, "[brtrace] cannot open %s\n", path);
    atexit(brtrace_close);
    g_init = 1;
}

void __brtrace_log(uint32_t module_id, uint32_t site_id, int32_t taken) {
    if (!g_init) brtrace_init();
    if (!g_fp) return;
    if (g_n == BRBUF_N) brtrace_flush();
    g_buf[g_n].module_id = module_id;
    g_buf[g_n].site_id   = site_id;
    g_buf[g_n].taken     = taken;
    ++g_n;
}

/* ----------------------------------------------------------------- selects */
/*
 * Same 12-byte record layout, separate file, separate buffer, separate init.
 *
 * site_id here holds the SELECT id, which lives in its own numbering space --
 * a (module_id, id) pair from this stream is NOT comparable to one from the
 * branch stream. Keep the two files apart when diffing.
 *
 * Fires more often than the branch hook in numeric code (ternaries, folded
 * min/max), so the buffering matters more here.
 */

static FILE *g_sel_fp = NULL;
static int   g_sel_init = 0;
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
    if (!g_sel_fp) fprintf(stderr, "[brtrace] cannot open %s\n", path);
    atexit(brtrace_sel_close);
    g_sel_init = 1;
}

void __brtrace_log_select(uint32_t module_id, uint32_t sel_id, int32_t taken) {
    if (!g_sel_init) brtrace_sel_init();
    if (!g_sel_fp) return;
    if (g_sel_n == BRBUF_N) brtrace_sel_flush();
    g_sel_buf[g_sel_n].module_id = module_id;
    g_sel_buf[g_sel_n].site_id   = sel_id;
    g_sel_buf[g_sel_n].taken     = taken;
    ++g_sel_n;
}