#include "macin_internal.h"
#include "macin_core.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// returns useful information about a mach-o file
bool macin_core(struct macin_core_options options, struct macin_core *core) {
	int fd = -1;

	// setup
	core->lc_cache_idx = 0;
	core->lc_cache_pos = 0;
	if (options.mmap.region) {
		core->mmap = options.mmap;
		core->called_mmap = false;
	} else {
		fd = open(options.path, O_RDONLY);
		if (fd == -1) {
			err("open failed");
		}
		core->mmap.rg_sz = lseek(fd, 0, SEEK_END);
		if (core->mmap.rg_sz == -1) {
			err("lseek failed");
		}
		core->mmap.region = mmap(NULL, core->mmap.rg_sz, PROT_READ, MAP_PRIVATE, fd, 0);
		close(fd);
		fd = -1;
		if (core->mmap.region == NULL) {
			err("mmap failed");
		}

		core->called_mmap = true;
	}
	core->header_offset = options.offset;
	
	// get info from header
	struct mach_header_64 *header = macin_read(core, 0, sizeof(struct mach_header_64));
	if (header == NULL) {
		err("couldn't read header");
	}

	// "magic number", used for determining byte order and 32-bit/64-bit
	uint32_t *magic = &(header->magic);

	core->header = (struct macin_core_header){
		.magic = *magic,
		.ncmds = header->ncmds,
		.sizeofcmds = header->sizeofcmds,
	};

	// doesn't support opposite endian
	if (*magic == MH_CIGAM || *magic == MH_CIGAM_64) {
		err("opposide endian not supported");
	}
	
	// determine if it's for a 32-bit or 64-bit architecture
	uint8_t is_64bit = *magic == MH_MAGIC_64;
	if (*magic != MH_MAGIC && !is_64bit) /* "if *magic isn't 32-bit and *magic isn't 64-bit..." */ {
		err("bad magic"); // "...then the file is invalid."
	}
	
	return true;
	
	err:;
	if (fd != -1) {
		close(fd);
	}
	macin_destroy_core(core);
	return false;
}

void macin_destroy_core(struct macin_core *core) {
	if (core->mmap.region != NULL && core->called_mmap) {
		munmap(core->mmap.region, core->mmap.rg_sz);
		core->called_mmap = false;
	}

	return;
}
