/* brtrace_runtime_mtu.c - branch trace recorder.
 *
 * 12-byte little-endian records: uint32 module_id, uint32 site_id, int32 taken.
 * Branches go to $BRTRACE_OUT (default brtrace.out), selects to
 * $BRTRACE_SEL_OUT (default brtrace_sel.out); the two id spaces are disjoint.
 *
 * Build: clang -O2 -c brtrace_runtime_mtu.c -o brtrace_runtime_mtu.o
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
