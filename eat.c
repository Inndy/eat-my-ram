#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "xoroshiro128plus.h"

#define KB (1LL<<10)
#define MB (1LL<<20)
#define GB (1LL<<30)
#define TB (1LL<<40)

#define BASE (256*MB)

void size2readable(char *buffer, size_t limit, size_t size)
{
    const char * const units[] = { "bytes", "kB", "MB", "GB", "TB", "PB", "EB" };
    double s = size;
    int i = 0;
    while(i < sizeof(units)/sizeof(units[0])) {
        if(s < 1023) {
            break;
        }
        s /= 1024;
        i++;
    }
    snprintf(buffer, limit, "%g %s", s, units[i]);
}

int xs128p_unsafe_auto_seed() {
    uint64_t a, b, c, s1, s2;
    do {
        a ^= getpid() + 777777777777;
        b ^= getppid() + 333333333333;
        c ^= time(NULL) + 999999999999;
        a *= 0xb505b7eb321f8c1d;
        b *= 0xa22f20c41ff61e3b;
        c *= 0x875c6fed3b3e8743;
        s1 = a ^ b - c;
        s2 = (a + c) * b;
    } while(!(s1 && s2 && a && b && c));
    return xs128p_seed(s1, s2);
}

// to keep memory fresh, dont be swaped
void work(uint64_t *memory, size_t length)
{
    xs128p_unsafe_auto_seed();
    printf("worker %d start to work!\n", getpid());
    size_t i;

    while(1) {
        for(i = 0; i < length; i++) {
            memory[i] = xs128p_next();
        }
        sleep(300);
    }
}

int main(int argc, const char * const argv[])
{
    if(argc < 2) {
        char buffer[64];
        size2readable(buffer, sizeof(buffer) - 1, BASE);

        printf("Usage: %s size-in-mb\n", argv[0]);
        printf("    ** size must be multiple of %s (%llu bytes)\n", buffer, BASE);
        return 1;
    }

    int i;
    char size_buffer[64];
    uint64_t *memory;
    size_t size, count, length;

    size = atoll(argv[1]);
    count = size * MB / BASE;
    length = BASE / sizeof(uint64_t);

    if(count * BASE != size) {
        size2readable(size_buffer, sizeof(size_buffer) - 1, count * BASE);
        printf("Warning: only %s will be ate!\n", size_buffer);
    }

    memory = (uint64_t*)malloc(BASE);

    if(!memory) {
        fprintf(stderr, "Can not allocate memory!\n");
        return 1;
    }

    for(i = 0; i < count; i++) {
        pid_t pid = fork();
        if(pid == 0) {
            work(memory, length);
        } else if (work > 0) {
            printf("Work #%.2d created, pid = %d\n", i, pid);
        } else {
            fprintf(stderr, "fork() failed at %d\n", i);
            return 2;
        }
    }

    printf("All process created, run `pkill %s -9` to end eat-my-ram\n", argv[0]);

    return 0;
}
