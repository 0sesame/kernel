#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint-gcc.h>
#include "page_manager.h"

struct MultibootHeader{
    uint32_t total_tag_size;
    uint32_t reserved;
};

struct MultibootTagHeader{
    uint32_t tag_type;
    uint32_t tag_size;
};

struct MultibootMemInfoEntry{
    struct UsableMemoryRegion memory_region;
    uint32_t type; // type 1 free for use
    uint32_t reserved;
};

struct MultibootMMapTag{
    struct MultibootTagHeader header;
    uint32_t size_of_each_entry;
    uint32_t each_entry_version;
};

struct MultibootElfTag{
    struct MultibootTagHeader header;
    uint32_t section_header_count;
    uint32_t section_header_entry_size;
    uint32_t index_of_section_with_string_table;
};

struct MultibootElfTagSectionHeader{
    uint32_t section_name_index;
    uint32_t section_type;
    uint64_t flags;
    uint64_t segment_address;
    uint64_t segment_offset;
    uint64_t segment_size;
    uint32_t table_index_link;
    uint32_t extra_info;
    uint64_t address_alignment;
    uint64_t fixed_entry_size;
};

bool is_in_use_by_kernel(uint64_t segment_start, uint64_t segment_end);
struct MultibootElfTag *parse_multiboot_tags(struct UsableMemoryRegion *, void *, uint32_t, int *);

#endif