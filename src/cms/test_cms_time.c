#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "count_min_sketch.h"

// Numbers of doing function
#define NUM_OBSERVATIONS 10000
#define KEY_LEN 16
#define NUM_RUNS 10

// Configurable width and column lists
const int widths[] = {1000, 10000, 100000};
const int columns[] = {10, 100, 1000};
const int num_widths = sizeof(widths) / sizeof(widths[0]);
const int num_columns = sizeof(columns) / sizeof(columns[0]);

void generate_random_key(char *key, size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < length - 1; i++) {
        key[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    key[length - 1] = '\0';
}

double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

void test_cms(int width, int column) {
    CountMinSketch cms;
    cms_init(&cms, width, column);

    srand(time(NULL));
    char keys[NUM_OBSERVATIONS][KEY_LEN];
    for (int i = 0; i < NUM_OBSERVATIONS; i++) {
        generate_random_key(keys[i], KEY_LEN);
    }

    double total_time = 0.0;
    for (int run = 0; run < NUM_RUNS; run++) {
        double start = get_time_sec();
        for (int i = 0; i < NUM_OBSERVATIONS; i++) {
            cms_add(&cms, keys[i]);
        }
        double end = get_time_sec();

        total_time += end - start;
    }

    double avg_time = total_time / NUM_RUNS;
    printf("Width: %d, Columns/Hash: %d\ncms_add(): %.9f seconds\n", width, column, avg_time);

	int res = 0, true = 0, false = 0;
    total_time = 0.0;
    for (int run = 0; run < NUM_RUNS; run++) {
        double start = get_time_sec();
        for (int i = 0; i < NUM_OBSERVATIONS; i++) {
            res = cms_check(&cms, keys[i]);
			
			// cms_check return should be the maximum of the actually count (NUM_RUNS)
			if (res >= NUM_RUNS)
				true += 1;
			else
				false += 1;
        }
        double end = get_time_sec();

        total_time += end - start;
    }
	
    avg_time = total_time / NUM_RUNS;
    printf("cms_check(): %.9f seconds\n", avg_time);
	double accuracy = (double)true/(true+false);
    //printf("Accuracy is %.9f\n\n", accuracy);
    printf("True: %d, False: %d\n", true, false);

    printf("-----------------------------\n");
    cms_destroy(&cms);
}

int main() {
    for (int i = 0; i < num_widths; i++) {
        for (int j = 0; j < num_columns; j++) {
            test_cms(widths[i], columns[j]);
        }
    }
    return 0;
}
