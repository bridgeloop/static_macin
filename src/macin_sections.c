#include "macin_internal.h"
#include "macin_sections.h"

#include "macin_lc.h"

struct macin_section *macin_sections(struct macin_core *core, void *lcs, uint32_t *total_nsects) {
	struct macin_section *sections = NULL;
	uint32_t ncmds = core->header.ncmds;

	size_t sg_sz, sc_sz;
	if (is64(core)) {
		sg_sz = sizeof(struct segment_command_64);
		sc_sz = sizeof(struct section_64);
	} else {
		sg_sz = sizeof(struct segment_command);
		sc_sz = sizeof(struct section);
	}

	for (uint32_t i = 0; i < core->header.ncmds; ++i) {
		struct load_command *lc = macin_lc_idx(core, lcs, i);
		if (lc == NULL) {
			err("invalid mach-o file");
		}
		uint32_t type = lc->cmd;

		if (type == LC_SEGMENT) {
			if (is64(core)) {
				err("incompatible segment type");
			}
		} else if (type == LC_SEGMENT_64) {
			if (!is64(core)) {
				err("incompatible segment type");
			}
		} else {
			continue;
		}

		void *region = macin_reinterpret(&(core->mmap), lc, sg_sz);
		if (region == NULL) {
			err("bad segment");
		}
		#define get_field(field) ( \
			is64(core) \
				? (((struct segment_command_64 *)region)->field) \
				: (((struct segment_command *)region)->field) \
		)

		uint64_t nsects = get_field(nsects);
		// sanity check!
		// these values are not trusted and
		// they are not used for security purposes.
		if ((uintptr_t)region + sg_sz + (nsects * sc_sz) != (uintptr_t)lc + lc->cmdsize) {
			err("bad segment");
		}
		*total_nsects += nsects;

		struct macin_section *new = macin_realloc(sections, *total_nsects * sizeof(struct macin_section));
		if (new == NULL) {
			err("failed to realloc sections");
		}
		sections = new;

		struct macin_section *base = sections + (*total_nsects - nsects);
		for (uint32_t ii = 0; ii < nsects; ++ii) {
			void *sect = macin_reinterpret(
				&(core->mmap),
				(void *)((uintptr_t)region + sg_sz + (ii * sc_sz)),
				sc_sz
			);
			if (sect == NULL) {
				err("bad section");
			}
			macin__to_section_64(
				sect,
				is64(core),
				base + ii
			);
		}
	}
	
	return sections;
	
err:;
	macin_destroy_sections(&(sections), total_nsects);
	return NULL;
}

void macin_destroy_sections(struct macin_section **sections, uint32_t *n_sects) {
	struct macin_section *s = *sections;
	*sections = NULL;
	*n_sects = 0;
	if (s != NULL) {
		macin_free(s);
	}

	return;
}