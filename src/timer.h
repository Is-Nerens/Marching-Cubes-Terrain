#pragma once

#include <windows.h>
#include <x86intrin.h>
#include <stdio.h>
#include <stdint.h>

// -----------------------------
// RDTSC cycle counter
// -----------------------------
static unsigned long long _PERFORMANCE_start_cycles;

static inline void start_measurement() {
    _PERFORMANCE_start_cycles = __rdtsc();
}

static inline void end_measurement() {
    unsigned long long end_cycles = __rdtsc();
    printf("CPU cycles: %llu\n", end_cycles - _PERFORMANCE_start_cycles);
}

// -----------------------------
// High precision wall clock timer (microseconds)
// -----------------------------
static LARGE_INTEGER _PERFORMANCE_freq;
static LARGE_INTEGER _PERFORMANCE_start_time;
static int _PERFORMANCE_initialized = 0;

static inline void _PERFORMANCE_init() {
    if (!_PERFORMANCE_initialized) {
        QueryPerformanceFrequency(&_PERFORMANCE_freq);
        _PERFORMANCE_initialized = 1;
    }
}

static inline void timer_start() {
    _PERFORMANCE_init();
    QueryPerformanceCounter(&_PERFORMANCE_start_time);
}

static inline void timer_stop() {
    LARGE_INTEGER end_time;
    QueryPerformanceCounter(&end_time);
    double elapsed_us = (double)(end_time.QuadPart - _PERFORMANCE_start_time.QuadPart) * 1e6 /
                        (double)_PERFORMANCE_freq.QuadPart;
    printf("Elapsed time: %.3f us\n", elapsed_us);
}
