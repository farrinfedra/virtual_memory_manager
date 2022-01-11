/**
 * virtmem.c
 */

#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define TLB_SIZE 16
#define PAGE_MASK 0xFFC00
#define VIRTUAL_PAGES 1024
#define PHYSICAL_PAGES 256 //frames

#define PAGE_SIZE 1024
#define OFFSET_BITS 10
#define OFFSET_MASK 1023

#define MEMORY_SIZE PHYSICAL_PAGES * PAGE_SIZE

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 10

struct tlbentry {
    unsigned char logical;
    unsigned char physical;
    int valid_bit;
};
struct pagetable_entry {
    unsigned char physical;
    int valid_bit;
};

// TLB is kept track of as a circular array, with the oldest element being overwritten once the TLB is full.
struct tlbentry tlb[TLB_SIZE];
struct pagetable_entry page_table[VIRTUAL_PAGES];
// number of inserts into TLB that have been completed. Use as tlbindex % TLB_SIZE for the index of the next TLB line to use.
int tlbindex = 0;

// pagetable[logical_page] is the physical page number for logical page. Value is -1 if that logical page isn't yet in the table.

signed char main_memory[MEMORY_SIZE];

// Pointer to memory mapped backing file
signed char *backing;

int max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

/* Returns the physical address from TLB or -1 if not present. */
int search_tlb(unsigned char logical_page) {
    for (int i =0; i< TLB_SIZE; i++){
        if (tlb[i].logical == logical_page && tlb[i].valid_bit == 1){
            return tlb[i].physical;
        }
    }
    return -1;
}

/* Update valid bit of overwritten entry at tlb. */
void update_tlb(unsigned char physical_page) {
    for (int i =0; i< TLB_SIZE; i++) {
        if (tlb[i].physical == physical_page) {
            tlb[i].valid_bit = 0;
            return;
        }
    }
}
/* Update valid bit of overwritten entry at pagetable. */
void update_pagetable(unsigned char physical_page) {
    for (int i =0; i< VIRTUAL_PAGES; i++) {
        if (page_table[i].physical == physical_page) {
            page_table[i].valid_bit = 0;
            return;
        }
    }
}

/* Adds the specified mapping to the TLB, replacing the oldest mapping (FIFO replacement). */
void add_to_tlb(unsigned char logical, unsigned char physical) {
    tlb[tlbindex % TLB_SIZE].logical = logical;
    tlb[tlbindex % TLB_SIZE].physical = physical;
    tlb[tlbindex % TLB_SIZE].valid_bit = 1;
    tlbindex++;
}

int fifo_index = -1;
int FIFO(){

    fifo_index++;
    return fifo_index % PHYSICAL_PAGES;
}

int LRU(){
    return 2;
}

int main(int argc, const char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage ./virtmem backingstore input -p 0/1\n");
        exit(1);
    }

    int (*page_replacement_policy)(); //returns the selected index of the pagetable to override.

    if (atoi(argv[4]) == 0) {
        page_replacement_policy = &FIFO;
    } else if (atoi(argv[4]) == 1) {
        page_replacement_policy = &LRU;
    } else {
        fprintf(stderr, "Page replacement policy should be 0 or 1\n");
        exit(1);
    }

    const char *backing_filename = argv[1];
    int backing_fd = open(backing_filename, O_RDONLY);
    backing = mmap(0, MEMORY_SIZE, PROT_READ, MAP_PRIVATE, backing_fd, 0);

    const char *input_filename = argv[2];
    FILE *input_fp = fopen(input_filename, "r");

    // Fill page table entries with 0 for initially empty table.

    for (int i = 0; i < VIRTUAL_PAGES; i++) {
        page_table[i].valid_bit = 0;
        page_table[i].physical = -1;
    }

    // Fill page tlb entries with -1 for initially empty table.
    for (int i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid_bit = 0;
    }

    // Character buffer for reading lines of input file.
    char buffer[BUFFER_SIZE];

    // Data we need to keep track of to compute stats at end.
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;

    // Number of the next unallocated physical page in main memory
    int free_page = 0;

    while (fgets(buffer, BUFFER_SIZE, input_fp) != NULL) {
        total_addresses++;
        int logical_address = atoi(buffer);

        // Calculate the page offset and logical page number from logical_address */
        int offset = logical_address & OFFSET_MASK; //take last 10 bits
        int logical_page = (logical_address  & PAGE_MASK) >> OFFSET_BITS; //take first 10 bits


        int physical_page = search_tlb(logical_page);
        // TLB hit
        if (physical_page != -1) {
            tlb_hits++;
            // TLB miss
        } else {

            if (page_table[logical_page].valid_bit == 1){
                physical_page = page_table[logical_page].physical;
            }else{
                physical_page = -1;
            }

            // Page fault
            if (physical_page == -1) {
                page_faults++;
                printf("Accessing logical: %d\n", logical_page);

                if (free_page < PHYSICAL_PAGES) {
                    physical_page = free_page;
                    free_page++;
                } else {
                    physical_page = page_replacement_policy();
                    /* Update valid bit of overwritten entry at tlb. */
                    update_tlb(physical_page);
                    /* Update valid bit of overwritten entry at pagetable */
                    update_pagetable(physical_page);
                }

                memcpy(main_memory + physical_page * PAGE_SIZE, backing + logical_page * PAGE_SIZE, PAGE_SIZE);
                //update page table
                page_table[logical_page].physical = physical_page;
                page_table[logical_page].valid_bit = 1;
            }

            add_to_tlb(logical_page, physical_page);

        }

        int physical_address = (physical_page << OFFSET_BITS) | offset;
        signed char value = main_memory[physical_page * PAGE_SIZE + offset];

        printf("Virtual address: %d Physical address: %d Value: %d\n", logical_address, physical_address, value);
    }
    printf("Number of Translated Addresses = %d\n", total_addresses);
    printf("Page Faults = %d\n", page_faults);
    printf("Page Fault Rate = %.3f\n", page_faults / (1. * total_addresses));
    printf("TLB Hits = %d\n", tlb_hits);
    printf("TLB Hit Rate = %.3f\n", tlb_hits / (1. * total_addresses));

    return 0;
}
