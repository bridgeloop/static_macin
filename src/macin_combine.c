#include "macin_internal.h"
#include "macin_combine.h"

#include "macin_core.h"
#include "macin_lc.h"
#include "macin_sections.h"
#include "macin_symbols.h"

struct macin *macin_with_options(struct macin_options options) {
	struct macin *macin = macin_malloc(sizeof(struct macin));
	if (macin == NULL) {
		err("allocation failed");
	}
	struct macin_core *core = &(macin->core);
	macin->n_sects = 0;
	macin->sections = NULL;
	macin->sym_coll.n_symbols = 0;
	macin->sym_coll.symbols = NULL;

	if (!macin_core(options.core_options, core)) {
		err("macin_core failed");
	}

	void *load_commands = macin_lc(core);
	if (load_commands == NULL) {
		err("macin_lc failed");
	}

	macin->sections = macin_sections(core, load_commands, &(macin->n_sects));
	if (macin->sections == NULL) {
		err("macin_segments failed");
	}
	
	if (options.retain_symbols) {
		if (!macin_symbols(core, load_commands, macin->sections, macin->n_sects, &(macin->sym_coll))) {
			err("macin_symbols failed");
		}
	}
	
	return macin;
	
	err:;
	
	macin_destroy(&(macin));
	return NULL;
}

void macin_destroy(struct macin **macin) {
	struct macin *obj = *macin;
	if (obj == NULL) {
		return;
	}

	macin_destroy_sections(&(obj->sections), &(obj->n_sects));
	macin_destroy_symbol_collection(&(obj->sym_coll));
	macin_destroy_core(&(obj->core));

	macin_free(obj);
	*macin = NULL;

	return;
}

struct macin *macin(char *path) {
	struct macin_options opts = {
		.core_options = {
			.offset = 0,
			.path = path,
			.mmap = { .region = NULL, },
		},

		.retain_symbols = true,
	};
	
	return macin_with_options(opts);
}