#ifndef macin_internal_h
#define macin_internal_h

#include "macin_types.h"
#include <mach-o/loader.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef NDEBUG

void *_macin_malloc(size_t a, char *str);
void *_macin_realloc(void *ptr, size_t a, char *str);
void *_macin_bound_malloc(size_t a, size_t b, char *str);
void *_macin_bound_realloc(void *ptr, size_t a, size_t b, char *str);
void _macin_free(void *ptr, char *str);

#define macin_malloc(a) _macin_malloc(a, #a)
#define macin_realloc(ptr, a) _macin_realloc(ptr, a, #a)
#define macin_bound_malloc(a, b) _macin_malloc(a, b, #a)
#define macin_bound_realloc(ptr, a, b) _macin_bound_realloc(ptr, a, b, #a)
#define macin_free(a) _macin_free(a, #a)

#else

#define macin_malloc malloc
#define macin_realloc realloc
#define macin_free free

static inline void *macin_bound_malloc(size_t a, size_t b) {
	if (a > b) {
		return NULL;
	}
	return malloc(a);
}
static inline void *macin_bound_realloc(void *ptr, size_t a, size_t b) {
	if (a > b) {
		return NULL;
	}
	return realloc(ptr, a);
}

#endif

void macin__to_section_64(void *s, uint8_t is_64bit, struct macin_section *l);

// these functions are safe
void *macin_reinterpret(struct macin_mmap *rg, void *address, uint32_t size);
void *macin_read_abs(struct macin_mmap *rg, off_t where, uint32_t size);
void *macin_read(struct macin_core *core, off_t where, uint32_t size);

#define PASTE2(a, b) a##b
#define PASTE(a, b) PASTE2(a, b)

#define streq !strcmp

#define err(message) do { \
	fputs("macin error: " message "\n", stderr); \
	goto err; \
} while (0)

#define is64(macin_obj) (macin_obj->header.magic == MH_MAGIC_64)
#define sz_32_64(what32, i64b) (i64b ? sizeof(PASTE(what32, _64)) : sizeof(what32))

#endif
