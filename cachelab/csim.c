#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/**
Cache simulator with LRU replacement policy using doubly linked list.
*/

int S = 0, E = 0, B = 0;
int hit_count = 0, miss_count = 0, eviction_count = 0;
int v = 0;
static int totalSet;

// Cache line structure for doubly linked list
typedef struct cache_line {
    int valid;
    unsigned long tag;
    struct cache_line* prev;
    struct cache_line* next;
} cache_line_t;

// Cache set structure
typedef struct {
    cache_line_t* head;  // dummy head node
    cache_line_t* tail;  // dummy tail node
    int size;            // current number of valid lines
} cache_set_t;

static cache_set_t* cache;

// LinkList Tools - Remove a node from the list
void removeNode(cache_line_t* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

// Add a node right after head (most recently used position)
void addToHead(cache_set_t* set, cache_line_t* node) {
    node->next = set->head->next;
    node->prev = set->head;
    set->head->next->prev = node;
    set->head->next = node;
}

// Move a node to head (mark as most recently used)
void moveToHead(cache_set_t* set, cache_line_t* node) {
    removeNode(node);
    addToHead(set, node);
}

// Remove the last node (least recently used)
cache_line_t* removeTail(cache_set_t* set) {
    cache_line_t* last = set->tail->prev;
    removeNode(last);
    return last;
}

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
    size_t set_pos = (address >> B) & (totalSet - 1);
    size_t tag = address >> (B + S);

    cache_set_t* target_set = &cache[set_pos];

    // Check for cache hit
    cache_line_t* cur_line = target_set->head->next;
    while (cur_line != target_set->tail) {
        if (cur_line->valid && cur_line->tag == tag) {
            // Cache hit
            hit_count++;
            if (v) {
                printf(" hit");
            }
            // Move to head (most recently used)
            moveToHead(target_set, cur_line);
            return;
        }
        cur_line = cur_line->next;
    }
    
    // Cache miss
    miss_count++;
    if (v) {
        printf(" miss");
    }
    
    // Create new cache line
    cache_line_t* newline = malloc(sizeof(cache_line_t));
    newline->tag = tag;
    newline->valid = 1;
    
    // Add to head (most recently used position)
    addToHead(target_set, newline);
    target_set->size++;

    // Check if eviction is needed
    if (target_set->size > E) {
        // Remove least recently used (tail)
        cache_line_t* evicted = removeTail(target_set);
        free(evicted);
        target_set->size--;
        eviction_count++;
        if (v) {
            printf(" eviction");
        }
    }
}

void simulate(FILE* tracefile) {
    char op;
    size_t address;
    int size;
    
    // Initialize cache
    cache = malloc(totalSet * sizeof(cache_set_t));
    for (int i = 0; i < totalSet; i++) {
        cache_set_t* cur_set = &cache[i];
        
        // Initialize dummy head and tail nodes
        cur_set->head = malloc(sizeof(cache_line_t));
        cur_set->tail = malloc(sizeof(cache_line_t));
        
        // Connect head and tail
        cur_set->head->next = cur_set->tail;
        cur_set->tail->prev = cur_set->head;
        cur_set->head->prev = NULL;
        cur_set->tail->next = NULL;
        
        cur_set->size = 0;
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
    
    // Free memory
    for (int i = 0; i < totalSet; i++) {
        cache_line_t* cur = cache[i].head;
        while (cur != NULL) {
            cache_line_t* next = cur->next;
            free(cur);
            cur = next;
        }
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