/*
 * Source: https://github.com/cloudflare/pingora/blob/main/pingora-limits/src/rate.rs
 */
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include <rte_cycles.h>

#include "cms/count_min_sketch.h"
#include "rate.h"

const unsigned long HASHES = 4;
const unsigned long SLOTS = 1024;

/* Utility function, return double ms second */
double get_time_in_ms(void);

/* Private function */
CountMinSketch *rate_current(Rate *rt, bool red_or_blue);
CountMinSketch *rate_previous(Rate *rt, bool red_or_blue);
bool red_or_blue(Rate *rt);
double maybe_reset(Rate *rt);

double get_time_in_ms() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (time.tv_sec * 1000.0) + (time.tv_usec / 1000.0);
}

int rate_new(Rate *rt, Duration interval) {
	return rate_new_with_estimator_config(rt, interval, HASHES, SLOTS);
}

int rate_new_with_estimator_config(Rate *rt, Duration interval, unsigned long hashes,
								   unsigned long slots) {
	cms_init(&rt->red_slot, hashes, slots);
	cms_init(&rt->blue_slot, hashes, slots);
	rte_atomic32_set(&rt->red_or_blue, 1); // Defuat set true
	rt->start.instant = get_time_in_ms();
	rt->reset_interval_ms = interval.duration;
	rte_atomic64_set(&rt->last_reset_time, 0); // Defuat set true
	rt->interval = interval;
}

CountMinSketch *rate_current(Rate *rt, bool red_or_blue) {
	if (red_or_blue)
		return &rt->red_slot;
	else
		return &rt->blue_slot;
}

CountMinSketch *rate_previous(Rate *rt, bool red_or_blue) {
	if (red_or_blue)
		return &rt->blue_slot;
	else
		return &rt->red_slot;
}

bool red_or_blue(Rate *rt) {
	return rte_atomic32_read(&rt->red_or_blue) != 0;
}

// return the per second rate estimation.
double rate_rate(Rate *rt, char *key) {
	double past_ms = maybe_reset(rt);
	if (past_ms >= rt->reset_interval_ms * 2) {
		// already missed 2 intervals, no data, just report 0 as short cut
		return 0.0;
	}

    CountMinSketch *prev_slot = rate_previous(rt, rte_atomic32_read(&rt->red_or_blue));
	int key_count = cms_check(prev_slot, key);
	return (key_count / (double)rt->reset_interval_ms) * 1000.0;
}

int32_t rate_observe(Rate *rt, char *key, unsigned int events) {
	maybe_reset(rt);
    CountMinSketch *current_slot = rate_current(rt, rte_atomic32_read(&rt->red_or_blue));
	return cms_add_inc(current_slot, key, events);
}

double maybe_reset(Rate *rt) {
	double now = get_time_in_ms();	
	double last_reset = rte_atomic64_read(&rt->last_reset_time) + rt->start.instant;
	double past_ms = now - last_reset;
	if (past_ms < rt->reset_interval_ms) {
		// no need to reset
		return past_ms;
	}
	
	double last_reset_2 = rte_atomic64_read(&rt->last_reset_time) + rt->start.instant;
	if (last_reset == last_reset_2) {
		// first clear the previous slo
		bool red_or_blue = rte_atomic32_read(&rt->red_or_blue);
		cms_clear(rate_previous(rt, red_or_blue));
		// then flip the flag to tell other to use the reset slot
		rte_atomic32_set(&rt->red_or_blue, !red_or_blue);
		// if current time is beyond 2 intervals, the data stored in the previous slot
        // is also stale, we should clear that too
		if (now - last_reset >= rt->reset_interval_ms * 2) {
			cms_clear(rate_current(rt, red_or_blue));
		}

		// Update the last_reset_time
		double new_last_reset = rte_atomic64_read(&rt->last_reset_time) + rt->reset_interval_ms;
		rte_atomic64_set(&rt->last_reset_time, rt->reset_interval_ms); 
	}

	/*
	Err(new) => {
		// another thread beats us to it
		assert!(new >= now - 1000); // double check that the new timestamp looks right
	}
	*/

	return past_ms;
}