#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double seconds(clockid_t id) {
    struct timespec ts;
    if (clock_gettime(id, &ts) != 0) { perror("clock_gettime"); exit(1); }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}
static unsigned long long parse_count(const char *s) {
    char *end = NULL;
    errno = 0;
    unsigned long long n = strtoull(s, &end, 10);
    if (errno || !end || *end || n == 0) {
        fprintf(stderr, "Cantidad invalida: %s\n", s);
        exit(2);
    }
    return n;
}
int main(int argc, char **argv) {
    if (argc != 3 || (strcmp(argv[1], "--iterations") && strcmp(argv[1], "--seconds"))) {
        fprintf(stderr, "Uso: %s --iterations N | --seconds N (1..30)\n", argv[0]);
        return 2;
    }
    unsigned long long amount = parse_count(argv[2]);
    if (!strcmp(argv[1], "--seconds") && amount > 30) {
        fprintf(stderr, "Duracion maxima: 30 segundos\n");
        return 2;
    }
    const int timed = !strcmp(argv[1], "--seconds");
    double start_wall = seconds(CLOCK_MONOTONIC);
    double start_cpu = seconds(CLOCK_PROCESS_CPUTIME_ID);
    volatile uint64_t acc = 0x123456789abcdefULL;
    unsigned long long iterations = 0;
    const unsigned long long batch = 100000ULL;
    do {
        unsigned long long steps = batch;
        if (!timed && amount - iterations < batch) steps = amount - iterations;
        for (unsigned long long k = 0; k < steps; ++k) {
            acc ^= acc << 13; acc ^= acc >> 7; acc ^= acc << 17;
        }
        iterations += steps;
    } while ((timed && seconds(CLOCK_MONOTONIC) - start_wall < (double)amount) ||
             (!timed && iterations < amount));
    double wall = seconds(CLOCK_MONOTONIC) - start_wall;
    double cpu = seconds(CLOCK_PROCESS_CPUTIME_ID) - start_cpu;
    printf("PID=%ld CPU_final=%d iteraciones=%llu tiempo_real=%.3f s tiempo_cpu=%.3f s checksum=%llu\n",
           (long)getpid(), sched_getcpu(), iterations, wall, cpu,
           (unsigned long long)acc);
    return 0;
}
