#ifndef macin_load_commands_h
#define macin_load_commands_h

#include "macin_types.h"

void *macin_lc(struct macin_core *core);

// returns a safe `struct load_command *`, or NULL.
// its fields are not trustworthy.
struct load_command *macin_lc_idx(struct macin_core *core, void *lcs, uint32_t index);

bool macin_lc_bad_span(struct macin_core *core, void *lcs, struct load_command *lc);

#endif
