/* nsan_bf_runtime.c -- runtime half of the nsan branch-flip instrumentation.
 *
 *   __nsan_bf_tick(mod, site, kind)   called before every instrumented fcmp;
 *                                     advances the site's occurrence index k
 *                                     and records it as the pending site.
 *   __wrap___nsan_fcmp_fail_*(...)    intercepts nsan's flip report through
 *                                     -Wl,--wrap and emits one event carrying
 *                                     the pending (mod, site, k).
 *
 * nsan calls the fail hook straight from the fcmp's block, so the pending
 * site is always current; stock compiler-rt is enough. The upstream handler
 * is not chained by default (it unwinds and symbolizes every flip); set
 * NSAN_BF_CHAIN=1 to call through.
 *
 * Output (NSAN_BF_LOG, else stderr), same shape as FPChecker's:
 *   #NSAN_EVENT mod=<id> site=<id> k=<occ> kind=<branch|select|oos>
 *               verdict=FLIP pred=<n> native=<0|1> shadow=<0|1>
 *   #NSAN_SITE  <mod> <site> <kind> <executions> <flagged>
 * nsan has no abstention, so verdict is always FLIP. */

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NSANBF_TAB_BITS 20
#define NSANBF_TAB_SIZE (1u << NSANBF_TAB_BITS)
#define NSANBF_TAB_MASK (NSANBF_TAB_SIZE - 1u)

typedef struct {
  uint32_t mod;
  int32_t site;
  uint32_t kind;
  uint64_t execs;
  uint64_t flagged;
  uint8_t used;
} SiteSlot;

static SiteSlot g_tab[NSANBF_TAB_SIZE];
static uint64_t g_overflow;
static uint64_t g_events;
static uint64_t g_oos_events;
static uint64_t g_unticked;

/* Pending site: set by the tick, consumed by the fail hook. */
static __thread uint32_t t_mod;
static __thread int32_t t_site = -2; /* -2 = nothing pending */
static __thread uint32_t t_kind;
static __thread uint64_t t_k;

static FILE *g_log;
static int g_chain;
static int g_inited;
static int g_max_events; /* 0 = unlimited */

static uint32_t slot_of(uint32_t mod, int32_t site, uint32_t kind) {
  uint64_t h = (uint64_t)mod * 1099511628211ull;
  h ^= (uint64_t)(uint32_t)site * 16777619ull;
  h ^= (uint64_t)kind * 2654435761ull;
  h ^= h >> 29;
  uint32_t i = (uint32_t)(h & NSANBF_TAB_MASK);
  for (uint32_t probe = 0; probe < 64; ++probe) {
    SiteSlot *s = &g_tab[i];
    if (!s->used) {
      s->used = 1;
      s->mod = mod;
      s->site = site;
      s->kind = kind;
      return i;
    }
    if (s->mod == mod && s->site == site && s->kind == kind)
      return i;
    i = (i + 1) & NSANBF_TAB_MASK;
  }
  ++g_overflow;
  return NSANBF_TAB_SIZE; /* sentinel */
}

static const char *kind_name(uint32_t k) {
  switch (k) {
  case 0: return "branch";
  case 1: return "select";
  default: return "oos";
  }
}

static void nsanbf_dump(void);

static void nsanbf_init(void) {
  if (g_inited)
    return;
  g_inited = 1;

  const char *path = getenv("NSAN_BF_LOG");
  if (path && *path) {
    g_log = fopen(path, "w");
    if (!g_log) {
      fprintf(stderr, "[NSanBF] cannot open NSAN_BF_LOG=%s, using stderr\n",
              path);
    }
  }
  if (!g_log)
    g_log = stderr;

  const char *chain = getenv("NSAN_BF_CHAIN");
  g_chain = chain && *chain && *chain != '0';

  const char *cap = getenv("NSAN_BF_MAX_EVENTS");
  g_max_events = cap ? atoi(cap) : 0;

  atexit(nsanbf_dump);
}

void __nsan_bf_tick(uint32_t mod, uint32_t site_u, uint32_t kind) {
  int32_t site = (int32_t)site_u;
  if (!g_inited)
    nsanbf_init();

  uint32_t i = slot_of(mod, site, kind);
  uint64_t k;
  if (i < NSANBF_TAB_SIZE) {
    /* 0-based, matching brtrace. */
    k = g_tab[i].execs++;
  } else {
    k = 0;
  }

  t_mod = mod;
  t_site = site;
  t_kind = kind;
  t_k = k;
}

static void record(int predicate, int native, int shadow) {
  if (!g_inited)
    nsanbf_init();

  if (t_site == -2) {
    /* Hook fired with no preceding tick: plugin and nsan pass disagree. */
    ++g_unticked;
    return;
  }

  ++g_events;
  if (t_kind == 2)
    ++g_oos_events;

  uint32_t i = slot_of(t_mod, t_site, t_kind);
  if (i < NSANBF_TAB_SIZE)
    g_tab[i].flagged++;

  if (g_max_events && (int)g_events > g_max_events)
    return;

  fprintf(g_log,
          "#NSAN_EVENT mod=%u site=%d k=%llu kind=%s verdict=FLIP "
          "pred=%d native=%d shadow=%d\n",
          t_mod, t_site, (unsigned long long)t_k, kind_name(t_kind), predicate,
          native ? 1 : 0, shadow ? 1 : 0);
}

static void dump_counts(void) {
  for (uint32_t i = 0; i < NSANBF_TAB_SIZE; ++i) {
    if (!g_tab[i].used)
      continue;
    fprintf(g_log, "#NSAN_SITE %u\t%d\t%s\t%llu\t%llu\n", g_tab[i].mod,
            g_tab[i].site, kind_name(g_tab[i].kind),
            (unsigned long long)g_tab[i].execs,
            (unsigned long long)g_tab[i].flagged);
  }
}

static void nsanbf_dump(void) {
  if (!g_log)
    return;
  dump_counts();
  fprintf(g_log, "#NSAN_TOTALS events=%llu out_of_scope=%llu unticked=%llu "
                 "overflow=%llu\n",
          (unsigned long long)g_events, (unsigned long long)g_oos_events,
          (unsigned long long)g_unticked, (unsigned long long)g_overflow);
  if (g_unticked)
    fprintf(g_log,
            "#NSAN_WARN %llu flip(s) had no pending site. The plugin and the "
            "nsan pass disagree about which fcmps are instrumented; do not "
            "score this run.\n",
            (unsigned long long)g_unticked);
  if (g_overflow)
    fprintf(g_log, "#NSAN_WARN site table overflow (%llu); raise "
                   "NSANBF_TAB_BITS and rebuild.\n",
            (unsigned long long)g_overflow);
  fflush(g_log);
  if (g_log != stderr)
    fclose(g_log);
  g_log = NULL;
}

/* Signatures must match compiler-rt/lib/nsan/nsan.cpp exactly. */

void __real___nsan_fcmp_fail_float_d(float, float, double, double, int, _Bool,
                                     _Bool);
void __real___nsan_fcmp_fail_double_q(double, double, __float128, __float128,
                                      int, _Bool, _Bool);
void __real___nsan_fcmp_fail_double_l(double, double, long double, long double,
                                      int, _Bool, _Bool);
void __real___nsan_fcmp_fail_longdouble_q(long double, long double, __float128,
                                          __float128, int, _Bool, _Bool);

void __wrap___nsan_fcmp_fail_float_d(float l, float r, double ls, double rs,
                                     int pred, _Bool res, _Bool sres) {
  record(pred, res, sres);
  if (g_chain)
    __real___nsan_fcmp_fail_float_d(l, r, ls, rs, pred, res, sres);
}

void __wrap___nsan_fcmp_fail_double_q(double l, double r, __float128 ls,
                                      __float128 rs, int pred, _Bool res,
                                      _Bool sres) {
  record(pred, res, sres);
  if (g_chain)
    __real___nsan_fcmp_fail_double_q(l, r, ls, rs, pred, res, sres);
}

void __wrap___nsan_fcmp_fail_double_l(double l, double r, long double ls,
                                      long double rs, int pred, _Bool res,
                                      _Bool sres) {
  record(pred, res, sres);
  if (g_chain)
    __real___nsan_fcmp_fail_double_l(l, r, ls, rs, pred, res, sres);
}

void __wrap___nsan_fcmp_fail_longdouble_q(long double l, long double r,
                                          __float128 ls, __float128 rs,
                                          int pred, _Bool res, _Bool sres) {
  record(pred, res, sres);
  if (g_chain)
    __real___nsan_fcmp_fail_longdouble_q(l, r, ls, rs, pred, res, sres);
}
