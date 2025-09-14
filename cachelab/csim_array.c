#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int S = 0, E = 0, B = 0;
int hit_count = 0, miss_count = 0, eviction_count = 0;
int v = 0;
static int totalSet;
static int global_timestamp = 0;

// Array
typedef struct line {
    int valid;
    int tag;
    int timestamp;
} line;

typedef struct {
    line* lines;
    int next_to_evict;
} set;

static set* cache;

// ./csim -v -s 4 -E 1 -b 4 -t traces/yi.trace
void parseInput(int argc, char *argv[], FILE** tracefile) {
    int option;
    while((option = getopt(argc, argv, "vs:E:b:t:")) != -1) {
        switch (option) {
            case 'v':
                v = 1;
                break;
            case 's':
                S = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                B = atoi(optarg);
                break;
            case 't':
                *tracefile = fopen(optarg, "r");
                break;            
        }
    }
    totalSet = 1 << S;
}

void update(size_t address) {
    size_t set_pos = address >> B & (totalSet - 1);
    size_t tag = address >> (B + S);

    set* target_set = &cache[set_pos];

    // Check for cache hit
    for (int i = 0; i < E; i++) {
        if (target_set->lines[i].valid && target_set->lines[i].tag == tag) {
            hit_count++;
            // Update timestamp for LRU
            target_set->lines[i].timestamp = global_timestamp++;
            if (v) {
                printf(" hit");
            }
            return;
        }
    }
    
    // Cache miss
    miss_count++;
    if (v) {
        printf(" miss");
    }

    int line_to_use = -1;
    
    // Find empty line first
    for (int i = 0; i < E; i++) {
        if (!target_set->lines[i].valid) {
            line_to_use = i;
            break;
        }
    }
    
    // If no empty line, find LRU line
    if (line_to_use == -1) {
        eviction_count++;
        if (v) {
            printf(" eviction");
        }
        
        // Find the line with smallest timestamp (least recently used)
        int lru_timestamp = target_set->lines[0].timestamp;
        line_to_use = 0;
        for (int i = 1; i < E; i++) {
            if (target_set->lines[i].timestamp < lru_timestamp) {
                lru_timestamp = target_set->lines[i].timestamp;
                line_to_use = i;
            }
        }
    }
    
    // Update the selected line
    target_set->lines[line_to_use].valid = 1;
    target_set->lines[line_to_use].tag = tag;
    target_set->lines[line_to_use].timestamp = global_timestamp++;
}

void simulate(FILE* tracefile) {
    char op;
    size_t address;
    int size;
    
    // Initialize cache
    cache = malloc(totalSet * sizeof(*cache));
    for (int i = 0; i < totalSet; i++) {
        cache[i].lines = malloc(sizeof(line) * E); // E lines
        cache[i].next_to_evict = 0;
        for (int j = 0; j < E; j++) {
            cache[i].lines[j].valid = 0;
            cache[i].lines[j].tag = -1;
            cache[i].lines[j].timestamp = 0;
        }
    }
    
    // S 00602264,1
    while (fscanf(tracefile, " %c %lx,%d", &op, &address, &size) > 0) {
        
        if (v) {
            printf("%c %lx,%d", op, address, size);
        }
        /**
        The operation field denotes the type of memory access: “I” denotes an instruction load, “L” a data load,
        “S” a data store, and “M” a data modify (i.e., a data load followed by a data store). There is never a space
        before each “I”. There is always a space before each “M”, “L”, and “S”. The address field specifies a 64-bit
        hexadecimal memory address. The size field specifies the number of bytes accessed by the operation. 
        */
        switch (op) {
            case 'I':
                break;
            case 'L':
            case 'S':
                update(address);
                break;
            case 'M':
                update(address);
                update(address);
                break;
        }

        if (v) {
            printf("\n");
        }
    }
    // free
    for (int i = 0; i < totalSet; i++) {
        free(cache[i].lines);
    }
    free(cache);
    fclose(tracefile);
}

int main(int argc, char *argv[])
{
    FILE* tracefile;
    parseInput(argc, argv, &tracefile);
    simulate(tracefile);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
