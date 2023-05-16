#ifndef macin_combine_h
#define macin_combine_h

#include "macin_types.h"

struct macin *macin(char *path);
struct macin *macin_with_options(struct macin_options options);
void macin_destroy(struct macin **macin);

#endif
