#ifndef macin_sections_h
#define macin_sections_h

#include "macin_types.h"

struct macin_section *macin_sections(struct macin_core *core, void *lcs, uint32_t *n_sects);
void macin_destroy_sections(struct macin_section **sections, uint32_t *n_sects);

#endif
