#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>

// Declaration of functions added
static void fully_associative();
static void direct_mapped();
static int fa_cache(int start, int end, int fifo);
static void dm_cache(uint32_t set);

typedef enum {dm, fa} cache_map_t;
typedef enum {uc, sc} cache_org_t;
typedef enum {instruction, data} access_t;

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;
mem_access_t access;

typedef struct {
    uint64_t accesses;
    uint64_t hits;
} cache_stat_t;

// Declaration of cache and global variables
const uint8_t block_offset_bits = 6;

struct block {                      
    bool valid;                     // Making a struct for the cache
    uint32_t tag;                   // containing a valid bit and the tag
};

struct block *ptr = NULL;           // Pointer to point at the address of the cache
int fifo_i, fifo_d, fifo_uc = 0;    // First in first out counter for different fully associative caches
int ways, setbits, data_cache_offset = 0;
uint32_t tagbits = 0;

uint32_t cache_size; 
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;

cache_stat_t cache_statistics;      // Variable used for cache statistics

/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */

mem_access_t read_transaction(FILE *ptr_file) {
    char type;
    if (fscanf(ptr_file, "%c %x\n", &type, &access.address) == 2) {
        if (type != 'I' && type != 'D') {
            printf("Unkown access type\n");
            exit(0);
        }
        access.accesstype = (type == 'I') ? instruction : data;
        return access;
    }

    /* If there are no more entries in the file,  
     * return an address 0 that will terminate the infinite loop in main
     */
    access.address = 0;
    return access;
}

int main(int argc, char** argv)
{
    {
        // Reset statistics:
        memset(&cache_statistics, 0, sizeof(cache_stat_t));

        /* Read command-line parameters and initialize:
        * cache_size, cache_mapping and cache_org variables
        */

        if ( argc != 4 ) { /* argc should be 2 for correct execution */
            printf("Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] [cache organization: uc|sc]\n");
            return EXIT_FAILURE;
        }
        /* argv[0] is program name, parameters start with argv[1] */

        /* Set cache size */
        cache_size = atoi(argv[1]);

        /* Set Cache Mapping */
        if (strcmp(argv[2], "dm") == 0) {
            cache_mapping = dm;
        } else if (strcmp(argv[2], "fa") == 0) {
            cache_mapping = fa;
        } else {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Set Cache Organization */
        if (strcmp(argv[3], "uc") == 0) {
            cache_org = uc;
        } else if (strcmp(argv[3], "sc") == 0) {
            cache_org = sc;
        } else {
            printf("Unknown cache organization\n");
            exit(0);
        }
    }
 
    ways = cache_size/block_size;                       // Determine the number of ways in fa
    setbits = log2(cache_size/block_size);              // Determine the number of set bits
    if (cache_mapping == fa){                           // Only one set in fa
        setbits = 0;                                    // therefore it should be 0 setbits
        fifo_d = ways/2;                                // Init the upper fifo counter
    }
    if ((cache_org == sc) && (cache_mapping == dm))     // Sc in dm gives half the set range
        setbits--;                                      // Remove one number of setbits                                                  
    data_cache_offset = pow(2,(setbits));               // An offset equal to the number of sets
    tagbits = 32 - setbits - block_offset_bits;         // Determining the number of tagbits

    // Make pointer point at the start of the cache, and allocating memory for the cache
    ptr = (struct block*)malloc((cache_size/block_size) * sizeof(struct block));
    
    for (uint32_t i = 0; i < (cache_size/block_size); i++)   {
        (ptr + i) -> valid = false;                     // Setting all valid bits
    }

    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen("mem_trace2.txt","r");
    if (!ptr_file) {
        printf("Unable to open the trace file\n");
        exit(1);
    }
    
    /* Loop until whole trace file has been read */
    mem_access_t access;
    while(true) {
        access = read_transaction(ptr_file);
        //If no transactions left, break out of loop
        if (access.address == 0)
            break;

        // Check what type of cache mapping
        switch (cache_mapping)
        {
            case fa:
                fully_associative();
                break;
            case dm:
                direct_mapped();
                break;
            
            default:                    // This should never happen
                assert(true);
                break;
        }    
    cache_statistics.accesses++;        // Add one after each access
    }

    /* Print the statistics */
    printf("\nCache Statistics\n");
    printf("-----------------\n\n");
    printf("Accesses: %ld\n", cache_statistics.accesses);
    printf("Hits:     %ld\n", cache_statistics.hits);
    printf("Hit Rate: %.4f\n", (double) cache_statistics.hits / cache_statistics.accesses);

    /* Close the trace file */
    fclose(ptr_file);
    // Free memory allocated with malloc
    free(ptr);
}

static void fully_associative()
{   // check if split or unified cache
    switch (cache_org)
    {
    case sc:
        if (access.accesstype == instruction)
            fifo_i = fa_cache(0,ways/2,fifo_i);     // Only half the total cache size for instructions cache
        else
            fifo_d = fa_cache(ways/2,ways,fifo_d);  // Second half of the cache for the data cache
        break;    
    case uc:
        fifo_uc = fa_cache(0,ways,fifo_uc);
        break;

    default:                                        // This should never happen
        assert(true);
        break;
    }
}

static void direct_mapped()
{
    uint32_t set = 0;
    if (setbits)
    {   // Shifting to isolate set/index bits of the address
        set = access.address >> block_offset_bits;  
        set = set << (block_offset_bits + tagbits);       
        set = set >> (block_offset_bits + tagbits);
    }
    // check if split or unified cache
    switch (cache_org)
    {   
        case sc:
            if (access.accesstype == instruction)
                dm_cache(set);                      // Only half the total cache size for instructions cache
            else
                dm_cache(set + data_cache_offset);  // Second half of the cache for the data cache
            break;
        case uc:
            dm_cache(set);
            break;
    
        default:                                    // This should never happen
            assert(true);
            break;
    }
}

static int fa_cache(int start, int end, int fifo)
{
    bool hit = false;                       // A variable to monitor whether it is a hit or a miss
    for (int i = start; i < end; i++)      // Looping through the cache looking for a match
        {
            // Checking if the tag of the cache matches the tag bits of the address, and if it is valid
            if ((ptr + i) -> tag == access.address >> (32 - tagbits) && (ptr + i) -> valid)
            {
                cache_statistics.hits++;    // Adding a hit to the statistics
                hit = true;
                break;                      // Exit the for loop
            }
        }
    if (!hit)
    {   // When cache miss, load the new tag into the cache
        (ptr + fifo) -> tag = access.address >> (32 - tagbits);
        (ptr + fifo) -> valid = true;       // Setting the cache tag valid
        fifo++;                             // Adding to the fifo counter
        if (fifo == end)        
            fifo = start;                   // Reset first in first out counter
    }
    return fifo;
}

static void dm_cache(uint32_t set)
{   // Checking if the tag of the cache matches the tag bits of the address, and if it is valid
    if ((ptr + set) -> tag == access.address >> (32 - tagbits) && (ptr + set) -> valid)
        cache_statistics.hits++;    // Adding a hit to the statistics
    else{ // When cache miss, load the new tag into the cache
        (ptr + set) -> tag = access.address >> (32 - tagbits);
        (ptr + set) -> valid = true;        // Setting the cache tag valid
    }
}