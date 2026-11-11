#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

static pid_t parse_pid(const char *text) {
    char *end = NULL;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || !text[0] || *end || value <= 0 || value > INT_MAX) {
        fprintf(stderr, "PID invalido: %s\n", text);
        exit(EXIT_FAILURE);
    }
    return (pid_t)value;
}

/* /proc/PID/stat: el campo comm puede contener espacios y parentesis. */
static int last_cpu(pid_t pid) {
    char path[64], line[8192];
    snprintf(path, sizeof(path), "/proc/%ld/stat", (long)pid);
    FILE *fp = fopen(path, "r");
    if (!fp) { perror(path); return -1; }
    if (!fgets(line, sizeof(line), fp)) { perror("fgets"); fclose(fp); return -1; }
    fclose(fp);
    char *after_comm = strrchr(line, ')');
    if (!after_comm) return -1;
    char *save = NULL;
    char *token = strtok_r(after_comm + 1, " \t\n", &save);
    /* El primer token es el campo 3 (state); processor es el campo 39. */
    for (int field = 3; token && field <= 39; ++field) {
        if (field == 39) {
            char *end = NULL;
            long cpu = strtol(token, &end, 10);
            return (end && *end == '\0' && cpu >= 0 && cpu <= INT_MAX) ? (int)cpu : -1;
        }
        token = strtok_r(NULL, " \t\n", &save);
    }
    return -1;
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Uso: %s [PID]\n", argv[0]);
        return EXIT_FAILURE;
    }
    pid_t pid = (argc == 2) ? parse_pid(argv[1]) : getpid();
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if (sched_getaffinity(pid, sizeof(allowed), &allowed) < 0) {
        perror("sched_getaffinity");
        return EXIT_FAILURE;
    }
    printf("PID=%ld CPU_ultima=%d CPU_permitidas=", (long)pid, last_cpu(pid));
    int printed = 0;
    for (int cpu = 0; cpu < CPU_SETSIZE; ++cpu) {
        if (CPU_ISSET(cpu, &allowed)) {
            printf("%s%d", printed ? "," : "", cpu);
            printed = 1;
        }
    }
    printf(" total_permitidas=%d\n", CPU_COUNT(&allowed));
    return EXIT_SUCCESS;
}
