#ifndef macin_types_h
#define macin_types_h

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

// macin //

#define MACIN_NUMBER_FIND_FAILURE ((uint64_t)(-1))

struct macin_mmap {
	void *region;
	size_t rg_sz;
};
struct macin_core_options {
	uint64_t offset;

	char *path;
	struct macin_mmap mmap;
};

struct macin_fat_options {
	char *path;
};

struct macin_core_header {
	uint32_t magic;
	uint32_t ncmds;
	uint32_t sizeofcmds;
};
struct macin_core {
	struct macin_mmap mmap;
	bool called_mmap;

	uint64_t header_offset;
	struct macin_core_header header;

	uint64_t lc_cache_idx;
	uint64_t lc_cache_pos;
};

struct macin_section {
	char sectname[16];
	uint64_t addr;
	uint64_t size;
	uint32_t offset;
	uint32_t reserved1;
};

struct macin_symbol {
	uint8_t type;
	uint32_t strtab_index;
	union {
		struct {
			uint64_t file_offset;
			uint64_t vm_address;
		} pointer;
		struct {
			uint64_t vm_address;
			uint64_t file_offset;
		};
	};
	//uint16_t desc;
};

struct macin_symbol_collection {
	uint64_t strtab_offset;
	struct macin_symbol *symbols;
	uint32_t n_symbols;
};

struct macin_options {
	struct macin_core_options core_options;
	bool retain_symbols;
};

struct macin {
	struct macin_core core;
	struct macin_section *sections;
	uint32_t n_sects;
	struct macin_symbol_collection sym_coll;
};

struct macin_fat {
	size_t array_length;
	struct macin *array[];
};

#endif
