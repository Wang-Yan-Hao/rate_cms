#ifndef RATE_H
#define RATE_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

#include <rte_cycles.h>

#include "count_min_sketch.h"

typedef struct {
    double duration;
} Duration;

typedef struct {
    double instant;
} Instant;

typedef struct {
    int64_t prev_samples; // isize
    int64_t curr_samples;
    Duration interval;
    double current_interval_fraction; // f64
} RateComponents;

typedef struct {
    CountMinSketch red_slot;
    CountMinSketch blue_slot;
    rte_atomic32_t red_or_blue; // AtomicBool
    Instant start;
    // Use u64 below instead of Instant because we want atomic operation
    double reset_interval_ms;
    rte_atomic64_t last_reset_time;
    Duration interval;
} Rate;

/* Public function */
int rate_new(Rate *rt, Duration interval);
int rate_new_with_estimator_config(Rate *rt, Duration interval, unsigned long hashes, unsigned long slots);
double rate_rate(Rate *rt, char *key);
int32_t rate_observe(Rate *rt, char *key, unsigned int events);

#endif // RATE_H
