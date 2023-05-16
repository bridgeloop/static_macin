#ifndef macin_fat_h
#define macin_fat_h

#include "macin_types.h"

struct macin_fat *macin_fat_with_options(struct macin_fat_options options);
void macin_destroy_fat(struct macin_fat **obj);
struct macin_fat *macin_fat(char *path);

#endif
