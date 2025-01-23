#include <stdint.h>
#include <stdbool.h>
#define _POSIX_C_SOURCE 199309L
#include <time.h>


#include "count_min_sketch.h"

typedef struct {
	struct timespec start;
} Duration;

typedef struct {
	struct timespec start;
} Instant;

typedef struct {
	int prev_samples;
	int curr_samples;
	Duration interval;
	double current_interval_fraction;
} RateComponenets;

typedef struct {
	CountMinSketch red_slot;
	CountMinSketch blue_slot;
	bool red_or_blue; // TODO atomic
	Instant start;

	unsigned long long reset_interval_ms;
	unsigned long long last_reset_time; // TODO atomic
	Duration interval;
} Rate; 

const unsigned long HASHES = 4;
const unsigned long SLOTS = 1024;

int rate_new(Rate *rt, Duration interval) {
	return rate_new_with_estimator_config(rt, interval, HASHES, SLOTS);
}

int rate_new_with_estimator_config(Rate *rt, Duration interval, unsigned long hashes,
								   unsigned long slots) {
	cms_init(&rt->red_slot, hashes, slots);
	cms_init(&rt->blue_slot, hashes, slots);
	rt->red_or_blue = true;
	// start: Instant::now(),
	// reset_interval_ms: interval.as_millis() as u64, // should be small not to overflow
	rt->last_reset_time = 0;
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

// TODO, rust 有時做可以 atomic 存取 bool value
bool red_or_blue(Rate *rt) {

}

double rate_rate(Rate *rt) {

}

double rate_observe(Rate *rt, char *key, unsigned int val) {
	maybe_reset(rt);
	cms_add_inc(rate_current(rt), key, val);
}

double maybe_reset(Rate *rt) {
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);

	now = now - rt->start;
	double last_reset = rt->last_reset_time; // TODO need to atomic
	double past_ms = now - rt->last_reset_time;

	if (past_ms < rt->reset_interval_ms) {
		return past_ms;
	}

	if (last_reset == rt->last_reset_time) {
		cms_clear(rate_previous(rt, rt->red_or_blue));
		rt->red_or_blue = !rt->red_or_blue;
		if (now - last_reset >= rt->reset_interval_ms * 2) {
			cms_clear(rate_current(rt, rt->red_or_blue));
		}	
	}

	/*
	Err(new) => {
		// another thread beats us to it
		assert!(new >= now - 1000); // double check that the new timestamp looks right
	}
	*/

	return past_ms;
}
