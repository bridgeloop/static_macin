#include "macin_internal.h"
#include "macin_fat.h"

#include "macin_combine.h"

#include <arpa/inet.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <mach-o/fat.h>

#ifndef ntohll
#define ntohll(x) (1 == ntohl(1) ? (x) : (((uint64_t)ntohl((x) & 0xffffffff)) << 32) | ntohl((uint32_t)((x) >> 32)))
#endif

struct macin_fat *macin_fat_with_options(struct macin_fat_options options) {
	struct macin_fat *ret = NULL;
	
	int fd = open(options.path, O_RDONLY);
	struct macin_mmap m = {
		.region = NULL,
		.rg_sz = 0,
	};
	if (fd == -1) {
		err("open failed");
	}
	m.rg_sz = lseek(fd, 0, SEEK_END);
	if (m.rg_sz == -1) {
		err("lseek failed");
	}
	m.region = mmap(NULL, m.rg_sz, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	fd = -1;
	if (m.region == NULL) {
		err("mmap failed");
	}

	bool is_64bit;
	
	struct fat_header *header = macin_read_abs(&(m), 0, sizeof(struct fat_header));
	if (header == NULL) {
		err("failed to read fat header");
	}

	uint32_t magic = ntohl(header->magic);
	if (magic == FAT_MAGIC) {
		is_64bit = false;
	} else if (magic == FAT_MAGIC_64) {
		is_64bit = true;
	} else {
		err("bad magic");
	}

	uint32_t nfat_arch = ntohl(header->nfat_arch);
	ret = macin_malloc(sizeof(struct macin_fat) + (sizeof(struct macin *) * nfat_arch));
	if (ret == NULL) {
		err("out of memory");
	}
	ret->array_length = 0;
	
	size_t fat_arch_sz = sz_32_64(struct fat_arch, is_64bit);
	for (uint32_t i = 0; i < nfat_arch; ++i) {
		struct fat_arch_64 *fat_arch =
			macin_read_abs(&(m), sizeof(struct fat_header) + (i * fat_arch_sz), sizeof(struct fat_arch_64));
		if (fat_arch == NULL) {
			err("failed to read fat_arch");
		}
		struct macin_options opts = {
			.core_options = {
				.offset = is_64bit ? ntohll(fat_arch->offset) : ntohl(((struct fat_arch *)fat_arch)->offset),
				.mmap = m,
			},

			.retain_symbols = true,
		};
		
		ret->array[ret->array_length] = macin_with_options(opts);
		if (ret->array[ret->array_length++] == NULL) {
			err("macin failed");
		}
	}
	
	return ret;
	
	err:;

	if (fd != -1) {
		close(fd);
	}
	if (m.region != NULL) {
		munmap(m.region, m.rg_sz);
	}
	macin_destroy_fat(&(ret));

	return NULL;
}

void macin_destroy_fat(struct macin_fat **m_obj) {
	struct macin_fat *obj = *m_obj;
	*m_obj = NULL;
	if (obj == NULL) {
		return;
	}

	for (int i = 0; i < obj->array_length && obj->array[i] != NULL; ++i) {
		macin_destroy(&(obj->array[i]));
	}

	macin_free(obj);
	return;
}

struct macin_fat *macin_fat(char *path) {
	struct macin_fat_options opts = { 0 };
	opts.path = path;
	
	return macin_fat_with_options(opts);
}
