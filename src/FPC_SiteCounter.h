#ifndef SRC_FPC_SITECOUNTER_H_
#define SRC_FPC_SITECOUNTER_H_

/* FPC_SiteCounter.h -- per-site occurrence indices and per-event verdicts.
 *
 * Gives every branch-controlling comparison a 0-based occurrence index k, so a
 * detection at (mod, site, k) can be joined against the oracle: in-window iff
 * k < E_S, a flip iff k is in the flip set. The counter ticks on every
 * execution; only non-silent verdicts are written. Per-site totals are dumped
 * at exit. Single-threaded by design (lock-step adjudication is anyway). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Power of two; sized for a whole link at <5% load. */
#define _FPC_SITE_TAB_BITS_ 14
#define _FPC_SITE_TAB_SIZE_ (1u << _FPC_SITE_TAB_BITS_)
#define _FPC_SITE_TAB_MASK_ (_FPC_SITE_TAB_SIZE_ - 1u)

typedef struct {
  uint64_t key;       /* (module_id << 32) | (uint32)site_id; 0 = empty  */
  uint64_t count;     /* executions of this site so far, this run        */
  uint64_t flagged;   /* reported UNSTABLE  (interval rule)              */
  uint64_t declined;  /* abstained: non-finite shadow (interval rule)    */
  /* Shadow-rule tallies; same entry, so both rules share one k. */
  uint64_t flagged_s;   /* reported SFLIP     (shadow rule)              */
} _FPC_SITE_ENT_T;

/* Not static: one instance per link. */
/* Branch-flip decision rule:
 *   _FPC_RULE_INTERVAL : |g| <= wa + wb + eta       (FPChecker, Sec 3.4)
 *   _FPC_RULE_SHADOW   : low_cond != shadow_cond    (Chevalier et al., CC'21)
 * Both read the same shadow state and the same k, so "both" is a paired
 * ablation of the decision rule alone. */
#define _FPC_RULE_INTERVAL 0x1
#define _FPC_RULE_SHADOW   0x2
int _FPC_BF_MODE_ = _FPC_RULE_INTERVAL;

/* Bit 3 marks an LLVM unordered predicate (true on NaN); the low three bits
 * are the comparison, unchanged. */
#define _FPC_PRED_UNORD_ 0x8
#define _FPC_PRED_BASE_(p) ((p) & 0x7)

_FPC_SITE_ENT_T _FPC_SITE_TAB_[_FPC_SITE_TAB_SIZE_];
int _FPC_SITE_TAB_INIT_ = 0;
uint64_t _FPC_SITE_TAB_USED_ = 0;
uint64_t _FPC_SITE_TAB_OVERFLOW_ = 0;

/* Top bit set so a live key is never 0 (empty). site_id -1 becomes 0xFFFFFFFF. */
static inline uint64_t _FPC_SITE_KEY_(uint32_t mod, int32_t site) {
  return (1ULL << 63) | ((uint64_t)mod << 32) | (uint64_t)(uint32_t)site;
}

static inline uint32_t _FPC_SITE_HASH_(uint64_t k) {
  /* splitmix64 finalizer, so consecutive site ids do not cluster. */
  k ^= k >> 33;
  k *= 0xff51afd7ed558ccdULL;
  k ^= k >> 33;
  k *= 0xc4ceb9fe1a85ec53ULL;
  k ^= k >> 33;
  return (uint32_t)(k & _FPC_SITE_TAB_MASK_);
}

static inline _FPC_SITE_ENT_T *_FPC_SITE_FIND_(uint32_t mod, int32_t site) {
  if (!_FPC_SITE_TAB_INIT_) {
    memset(_FPC_SITE_TAB_, 0, sizeof(_FPC_SITE_TAB_));
    _FPC_SITE_TAB_INIT_ = 1;
  }
  uint64_t key = _FPC_SITE_KEY_(mod, site);
  uint32_t i = _FPC_SITE_HASH_(key);
  for (uint32_t probe = 0; probe < _FPC_SITE_TAB_SIZE_; ++probe) {
    _FPC_SITE_ENT_T *e = &_FPC_SITE_TAB_[(i + probe) & _FPC_SITE_TAB_MASK_];
    if (e->key == key)
      return e;
    if (e->key == 0) {
      e->key = key;
      _FPC_SITE_TAB_USED_++;
      return e;
    }
  }
  /* Full: return NULL and count it rather than reuse a slot. */
  _FPC_SITE_TAB_OVERFLOW_++;
  return NULL;
}

/* Returns this execution's 0-based occurrence index, then advances. Call
 * exactly once per execution, before any verdict or early return. */
static inline uint64_t _FPC_SITE_TICK_(uint32_t mod, int32_t site) {
  _FPC_SITE_ENT_T *e = _FPC_SITE_FIND_(mod, site);
  if (!e)
    return (uint64_t)-1;
  return e->count++;
}

static inline void _FPC_SITE_MARK_(uint32_t mod, int32_t site, int declined) {
  _FPC_SITE_ENT_T *e = _FPC_SITE_FIND_(mod, site);
  if (!e)
    return;
  if (declined)
    e->declined++;
  else
    e->flagged++;
}

/* As above, for the shadow rule. */
static inline void _FPC_SITE_MARK_S_(uint32_t mod, int32_t site) {
  _FPC_SITE_ENT_T *e = _FPC_SITE_FIND_(mod, site);
  if (!e)
    return;
  e->flagged_s++;
}

/* One line per non-silent verdict:
 *   #FPC_EVENT mod=<id> site=<id> k=<occurrence> verdict=<UNSTABLE|DECLINED|...>
 * DECLINED is an abstention, not a stability verdict. */
static inline void _FPC_SITE_EMIT_(FILE *out, uint32_t mod, int32_t site,
                                   uint64_t k, const char *verdict,
                                   const char *file, int line,
                                   const char *func, const char *cause) {
  if (!out)
    return;
  fprintf(out, "#FPC_EVENT mod=%u site=%d k=%llu verdict=%s at %s:%d in %s",
          mod, site, (unsigned long long)k, verdict,
          file ? file : "Unknown", line, func ? func : "Unknown");
  if (cause && *cause)
    fprintf(out, " cause=%s", cause);
  fputc('\n', out);
}

/* Per-site totals, written at exit. `executions` is the full-run count T. */
static inline void _FPC_SITE_DUMP_(FILE *out) {
  if (!out || !_FPC_SITE_TAB_INIT_)
    return;
  fprintf(out, "#FPC_SITES module_id\tsite_id\texecutions\tflagged\tdeclined\n");
  for (uint32_t i = 0; i < _FPC_SITE_TAB_SIZE_; ++i) {
    _FPC_SITE_ENT_T *e = &_FPC_SITE_TAB_[i];
    if (e->key == 0)
      continue;
    uint32_t mod = (uint32_t)((e->key >> 32) & 0x7FFFFFFFu);
    int32_t site = (int32_t)(uint32_t)(e->key & 0xFFFFFFFFu);
    fprintf(out, "#FPC_SITE %u\t%d\t%llu\t%llu\t%llu\n",
            mod, site, (unsigned long long)e->count,
            (unsigned long long)e->flagged,
            (unsigned long long)e->declined);
  }
  /* Shadow-rule totals, only when that rule ran. */
  if (_FPC_BF_MODE_ & _FPC_RULE_SHADOW) {
    fprintf(out, "#FPC_SITES_S module_id\tsite_id\texecutions\tflagged\n");
    for (uint32_t i = 0; i < _FPC_SITE_TAB_SIZE_; ++i) {
      _FPC_SITE_ENT_T *e = &_FPC_SITE_TAB_[i];
      if (e->key == 0)
        continue;
      uint32_t mod = (uint32_t)((e->key >> 32) & 0x7FFFFFFFu);
      int32_t site = (int32_t)(uint32_t)(e->key & 0xFFFFFFFFu);
      fprintf(out, "#FPC_SITE_S %u\t%d\t%llu\t%llu\n",
              mod, site, (unsigned long long)e->count,
              (unsigned long long)e->flagged_s);
    }
  }
  fprintf(out, "#FPC_SITES total=%llu overflow=%llu\n",
          (unsigned long long)_FPC_SITE_TAB_USED_,
          (unsigned long long)_FPC_SITE_TAB_OVERFLOW_);
  if (_FPC_SITE_TAB_OVERFLOW_)
    fprintf(out, "#FPC_SITES WARNING: the site table filled up; %llu "
                 "execution(s) were not counted and occurrence indices are "
                 "NOT reliable. Raise _FPC_SITE_TAB_BITS_ and rerun.\n",
            (unsigned long long)_FPC_SITE_TAB_OVERFLOW_);
}

/* Same table as _FPC_SITE_DUMP_, as a JSON array. */
static inline void _FPC_SITE_DUMP_JSON_(FILE *out) {
  int first = 1;
  fprintf(out, "[");
  if (_FPC_SITE_TAB_INIT_) {
    for (uint32_t i = 0; i < _FPC_SITE_TAB_SIZE_; ++i) {
      _FPC_SITE_ENT_T *e = &_FPC_SITE_TAB_[i];
      if (e->key == 0)
        continue;
      uint32_t mod = (uint32_t)((e->key >> 32) & 0x7FFFFFFFu);
      int32_t site = (int32_t)(uint32_t)(e->key & 0xFFFFFFFFu);
      fprintf(out,
              "%s\n    {\"module_id\": %u, \"site_id\": %d, \"executions\": %llu, "
              "\"flagged\": %llu, \"declined\": %llu, "
              "\"flagged_shadow\": %llu}",
              first ? "" : ",", mod, site, (unsigned long long)e->count,
              (unsigned long long)e->flagged, (unsigned long long)e->declined,
              (unsigned long long)e->flagged_s);
      first = 0;
    }
  }
  fprintf(out, "\n  ]");
}

#endif /* SRC_FPC_SITECOUNTER_H_ */
