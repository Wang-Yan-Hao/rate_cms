
#include <unistd.h>
#include <assert.h>

#include "rate/rate.h"

void test_observe_rate() {
    Duration interval = {1000}; // 1 sec = 1000 ms
    Rate r;
    rate_new(&r, interval);
    const char *key = "ip0/port0";

    // second: 0
    assert(rate_observe(&r, key, 3) == 3);
	assert(rate_observe(&r, key, 2) == 5);
	assert(rate_rate(&r, key) == 0);

    // second: 1
    sleep(1);
	assert(rate_observe(&r, key, 4) == 4);
    assert(rate_rate(&r, key) == 5);

    // second: 2
    sleep(1);
    assert(rate_rate(&r, key) == 4);

    // second: 3
    sleep(1);
    assert(rate_rate(&r, key) == 0);

    printf("All tests passed! (test_observe_rate)\n");
}

int main() {
	test_observe_rate();
	return 0;
}