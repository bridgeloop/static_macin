#include "macin_internal.h"

#ifndef NDEBUG

static size_t allocations = 0;
void *_macin_malloc(size_t a, char *str) {
	void *ptr = malloc(a);
	if (ptr != NULL) {
		allocations += 1;
		#ifndef NDEBUG
		printf("malloc: %s\n", str);
		#endif
	}
	return ptr;
}
void *_macin_realloc(void *ptr, size_t a, char *str) {
	void *new_ptr = realloc(ptr, a);
	if (ptr == NULL && new_ptr != NULL) {
		allocations += 1;
		#ifndef NDEBUG
		printf("realloc: %s\n", str);
		#endif
	}
	return new_ptr;
}
void _macin_free(void *ptr, char *str) {
	if (ptr == NULL) {
		abort();
	}
	allocations -= 1;
	#ifndef NDEBUG
	printf("free: %s\n", str);
	#endif
	free(ptr);
	return;
}
void *_macin_bound_malloc(size_t a, size_t b, char *str) {
	if (a > b) {
		return NULL;
	}
	return _macin_malloc(a, str);
}
void *_macin_bound_realloc(void *ptr, size_t a, size_t b, char *str) {
	if (a > b) {
		return NULL;
	}
	return _macin_realloc(ptr, a, str);
}
void macin_exit(int status) {
	if (allocations) {
		fprintf(stderr, "%zu memory leak(s) detected\n", allocations);
		abort();
	}
	exit(status);
}

#endif

void macin__to_section_64(void *s, uint8_t is_64bit, struct macin_section *l) {
	#define get_field(field) (is_64bit ? (((struct section_64 *)s)->field) : (((struct section *)s)->field))
	
	strncpy(l->sectname, get_field(sectname), 16);
	l->addr = get_field(addr);
	l->size = get_field(size);
	l->offset = get_field(offset);
	l->reserved1 = get_field(reserved1);

	#undef get_field
}

void *macin_reinterpret(struct macin_mmap *rg, void *address, uint32_t size) {
	if (address < rg->region || address + size > rg->region + rg->rg_sz) {
		return NULL;
	}
	return address;
}
void *macin_read_abs(struct macin_mmap *rg, off_t where, uint32_t size) {
	if (where < 0) {
		return NULL;
	}
	return macin_reinterpret(
		rg,
		(void *)((uintptr_t)rg->region + where),
		size
	);
}
void *macin_read(struct macin_core *core, off_t where, uint32_t size) {
	return macin_read_abs(
		&(core->mmap),
		core->header_offset + where,
		size
	);
}