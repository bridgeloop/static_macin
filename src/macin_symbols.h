#ifndef macin_symbols_h
#define macin_symbols_h

#include "macin_types.h"

bool macin_symbols(struct macin_core *core, void *lcs, struct macin_section *sections, uint32_t n_sects, struct macin_symbol_collection *collection);
void macin_destroy_symbol_collection(struct macin_symbol_collection *collection);

struct macin_symbol *macin_symbols_find_symbol(struct macin_core *core, struct macin_symbol_collection *collection, char *name);

#endif
