#ifndef macin_core_h
#define macin_core_h

#include "macin_types.h"

bool macin_core(struct macin_core_options options, struct macin_core *core);
void macin_destroy_core(struct macin_core *core);

#endif
