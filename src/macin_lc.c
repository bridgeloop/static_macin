#include "macin_internal.h"
#include "macin_lc.h"

void *macin_lc(struct macin_core *core) {
	if (core == NULL) {
		return NULL;
	}
	
	// lc = load command
	// load commands directly follow the mach header
	uint64_t lc_off = sz_32_64(struct mach_header, is64(core));
	uint64_t sizeofcmds = core->header.sizeofcmds;

	if (!sizeofcmds) {
		err("no load commands");
	}

	void *load_commands = macin_read(core, lc_off, sizeofcmds);	
	if (load_commands == NULL) {
		err("failed to read load commands");
	}
	
	return load_commands;
	
	err:
	return NULL;
}

struct load_command *macin_lc_idx(struct macin_core *core, void *lcs, uint32_t index) {
	if (index >= core->header.ncmds) {
		return NULL;
	}

	uint64_t cached_idx = core->lc_cache_idx;
	uint64_t pos = core->lc_cache_pos;
	if (index < cached_idx) {
		cached_idx = 0;
		pos = 0;
	}
	for (uint32_t i = cached_idx; i <= index; ++i) {
		if ((pos + sizeof(struct load_command)) > core->header.sizeofcmds) {
			return NULL;
		}
		// SAFETY: macin_reinterpret is not needed here, because `lc`
		//         is within the bounds of `lcs` (which is trustworthy).
		//         the information held inside of `lc` is not trustworthy.
		struct load_command *lc = (struct load_command *)((uintptr_t)lcs + pos);
		// bail if `cmdsize` is malformed.
		if (lc->cmdsize == 0 || macin_lc_bad_span(core, lcs, lc)) {
			return NULL;
		}
		if (i == index) {
			return lc;
		}
		core->lc_cache_idx = i;
		core->lc_cache_pos = pos;
		pos += lc->cmdsize;
	}
	__builtin_unreachable();
}
bool macin_lc_bad_span(struct macin_core *core, void *lcs, struct load_command *lc) {
	return !(
		(uintptr_t)lc >= (uintptr_t)lcs &&
		(uintptr_t)lc->cmdsize < (uintptr_t)lcs + core->header.sizeofcmds
	);
}