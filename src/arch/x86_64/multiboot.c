#include <stdbool.h>
#include "mystdio.h"
#include "page_manager.h"
#include "multiboot.h"

#define MMAP_TAG_TYPE 6
#define ELF_SYMBOL_TYPE 9
#define USABLE_MEM_TYPE 1

uint64_t kernel_start = ~(0x0);
uint64_t kernel_end = 0;

bool last_tag(struct MultibootTagHeader *tag_header){
    return tag_header->tag_type == 0 && tag_header->tag_size == 8;
}

int get_usable_memory_regions_from_tag(struct UsableMemoryRegion *regions, struct MultibootMMapTag *mem_map){
    uint32_t tag_count = mem_map->header.tag_size / mem_map->size_of_each_entry;
    struct MultibootMemInfoEntry *entries = (struct MultibootMemInfoEntry *) (mem_map + 1);
    int tag_index = 0;
    int regions_index = 0;

    while(tag_index <= tag_count && tag_index < MAX_MEMORY_REGIONS){
        if(entries[tag_index].type == USABLE_MEM_TYPE){
            regions[regions_index] = entries[tag_index].memory_region;
            printk("%lx, %lx\n", regions[regions_index].start_address, regions[regions_index].start_address + regions[regions_index].length);
            regions_index++;
        }
        tag_index++;
    }
    // "null" termination of regions list
    regions[regions_index].length = 0;
    regions[regions_index].start_address = 0;
    return regions_index + 1;
}

void parse_elf_symbols_file(void *tag){
    /* debugging function, used to print addresses used by the kernel elf file */
    struct MultibootElfTag *elf_tag_header = (struct MultibootElfTag *) tag;
    struct MultibootElfTagSectionHeader *elf_section = (struct MultibootElfTagSectionHeader *) (elf_tag_header + 1);
    for(int i = 0; i <  elf_tag_header->section_header_count; i++){
        if(elf_section[i].segment_address < kernel_start){
            kernel_start = elf_section[i].segment_address;
        }
        if(elf_section[i].segment_address + elf_section[i].segment_size > kernel_end){
            kernel_end = elf_section[i].segment_address + elf_section[i].segment_size;
        }
        printk("segment address %p, segment length %lu\n", (void *) elf_section[i].segment_address, elf_section[i].segment_size);
    }
}

bool is_in_use_by_kernel(uint64_t segment_start, uint64_t segment_end){
    if(kernel_start >= segment_start && kernel_start <= segment_end){
        return true;
    }
    if(segment_start >= kernel_start && segment_start <= kernel_end){
        return true;
    }
    if(kernel_end >= segment_start && kernel_end <= segment_end){
        return true;
    }
    return false; 
}

struct MultibootElfTag *parse_multiboot_tags(struct UsableMemoryRegion *memory_regions, void *tags, uint32_t total_tag_size, int *memory_regions_counts){
    /*
    gets all usable memory regions from tags
    gets pointer to elf tag struct
    */
    struct MultibootTagHeader *tag_header = (struct MultibootTagHeader *) tags;
    struct MultibootElfTag *elf_tag_header;
    uint64_t read_bytes = 0;  
    uint64_t next_tag_offset = 0;
    while(!(last_tag(tag_header))){// && read_bytes < total_tag_size){
        if(tag_header->tag_type == MMAP_TAG_TYPE){
            *memory_regions_counts = get_usable_memory_regions_from_tag(memory_regions, (struct MultibootMMapTag *) tag_header);
        }
        if(tag_header->tag_type == ELF_SYMBOL_TYPE){
            elf_tag_header = (struct MultibootElfTag *) tag_header;
            parse_elf_symbols_file((void * ) tag_header);
        }
        next_tag_offset = (tag_header->tag_size + 7) & ~7;
        read_bytes += next_tag_offset;
        tag_header = (struct MultibootTagHeader *) (((uint8_t *) tag_header) + next_tag_offset);
    }
    return elf_tag_header;
}