#include "macin_internal.h"
#include "macin_symbols.h"

#include "macin_lc.h"

#include <mach-o/nlist.h>

const uint8_t MAX_SYMBOL_NAME_SZ = 0xff;

bool macin_symbols(struct macin_core *core, void *lcs, struct macin_section *sections, uint32_t n_sects, struct macin_symbol_collection *collection) {
	uint32_t n_lazy_symbols = 0;

	struct {
		void *symtab;
		
		uint32_t nsyms;
		uint32_t strtab_off;
	} symtab_info;
	bzero(&(symtab_info), sizeof(symtab_info));
	symtab_info.symtab = NULL;
	
	struct {
		bool indirect_symtab;
		
		uint32_t *lazy_indexes;
		uint64_t nlazysyms;
		
		struct macin_section *lazy_symbols_section;
	} lazy_symtab_info;
	lazy_symtab_info.indirect_symtab = false;
	lazy_symtab_info.lazy_indexes = NULL;
	lazy_symtab_info.nlazysyms = 0;
	lazy_symtab_info.lazy_symbols_section = NULL;

	for (uint32_t i = 0; i < core->header.ncmds; ++i) {
		struct load_command *lc = macin_lc_idx(core, lcs, i);
		
		// symbol table
		if (lc->cmd == LC_SYMTAB) {
			if (symtab_info.symtab != NULL) {
				err("multiple symbol tables");
			}
			
			struct symtab_command *cmd = macin_reinterpret(&(core->mmap), lc, sizeof(struct symtab_command));
			uint32_t symtab_off = cmd->symoff;
			symtab_info.strtab_off = cmd->stroff;
			symtab_info.nsyms = cmd->nsyms;

			size_t nlist_sz = sz_32_64(struct nlist, is64(core));
			symtab_info.symtab = macin_read(core, symtab_off, symtab_info.nsyms * nlist_sz);
			if (symtab_info.symtab == NULL) {
				err("failed to read symtab");
			}
		}

		// dynamic symbol table
		else if (lc->cmd == LC_DYSYMTAB) {
			if (lazy_symtab_info.indirect_symtab) {
				err("multiple dynamic symbol tables");
			}
			lazy_symtab_info.indirect_symtab = true;

			struct dysymtab_command *cmd = macin_reinterpret(&(core->mmap), lc, sizeof(struct dysymtab_command));
			uint32_t n_indirect = cmd->nindirectsyms;
			uint32_t *indirect_symtab = macin_read(core, cmd->indirectsymoff, n_indirect * sizeof(uint32_t));
			if (indirect_symtab == NULL) {
				err("failed to read dysymtab");
			}
			for (uint32_t ii = 0; ii < n_sects; ++ii) {
				//printf("section type: %x\n", sections[ii].flags & 0xff);
				if (streq(sections[ii].sectname, "__la_symbol_ptr")) {
					lazy_symtab_info.lazy_symbols_section = sections + ii;

					uint32_t lazy_index = lazy_symtab_info.lazy_symbols_section->reserved1;
					uint32_t n_lazy = lazy_symtab_info.lazy_symbols_section->size / sizeof(uint32_t *);

					uint32_t sum;
					if (__builtin_add_overflow(lazy_index, n_lazy, &(sum))) {
						err("bad section");
					}
					if (sum > n_indirect) {
						err("bad section");
					}

					lazy_symtab_info.lazy_indexes = indirect_symtab + lazy_index;
					lazy_symtab_info.nlazysyms = n_lazy;
					break;
				}
			}
		}
	}

	collection->symbols = macin_malloc(sizeof(struct macin_symbol) * symtab_info.nsyms);
	collection->n_symbols = 0;

	#define symtab_field(i, f) (is64(core) ? ((struct nlist_64 *)symtab_info.symtab)[i].f : ((struct nlist *)symtab_info.symtab)[i].f)

	for (uint32_t idx = 0; idx < symtab_info.nsyms; idx++) {
		uint8_t symbol_type = symtab_field(idx, n_type);
		uint64_t symbol_address = symtab_field(idx, n_value);
		uint64_t symbol_section = symtab_field(idx, n_sect);
		if (symbol_section >= n_sects) {
			err("bad symbol");
		}
		uint32_t string_table_index = symtab_field(idx, n_un.n_strx);
		//uint16_t symbol_description = symtab_field(idx, n_desc);

		char *name = macin_read(core, symtab_info.strtab_off + string_table_index, MAX_SYMBOL_NAME_SZ);
		if (name == NULL) {
			continue;
		}

		bool pointer = false;

		if (!symbol_address) {
			int type = 0;
			for (int i = 0; !type && (i < lazy_symtab_info.nlazysyms); ++i) {
				if (idx == lazy_symtab_info.lazy_indexes[i]) {
					type = 1;
				}
			}
			if (!type) {
				#ifndef NDEBUG
				puts("to-do https://opensource.apple.com/source/xnu/xnu-4570.71.2/EXTERNAL_HEADERS/mach-o/loader.h.auto.html");
				#endif
				continue;
			}
			
			// lazy symbols
			if (type == 1) {
				pointer = true;
			} else {
				continue;
			}
		}

		struct macin_symbol *sym = &(collection->symbols[collection->n_symbols++]);

		sym->strtab_index = string_table_index;

		//
		sym->type = symbol_type;
		//lazy_sym->desc = symbol_description;
		//lazy_sym->is_pointer = 1; ?
		//

		if (pointer) {
			sym->pointer.file_offset = lazy_symtab_info.lazy_symbols_section->offset + (n_lazy_symbols * sizeof(uint64_t));
			sym->pointer.vm_address = lazy_symtab_info.lazy_symbols_section->addr + (n_lazy_symbols * sizeof(uint64_t));
			n_lazy_symbols += 1;
		} else {
			sym->vm_address = symbol_address;

			struct macin_section *section = &(sections[symbol_section]);
			sym->file_offset = symbol_address - section->addr + section->offset;
		}
	}

	collection->strtab_offset = symtab_info.strtab_off;

	return true;

	err:;
	macin_destroy_symbol_collection(collection);
	return false;
}

void macin_destroy_symbol_collection(struct macin_symbol_collection *collection) {
	if (collection->symbols != NULL) {
		macin_free(collection->symbols);
		collection->symbols = NULL;
	}
	collection->n_symbols = 0;
}

struct macin_symbol *macin_symbols_find_symbol(struct macin_core *core, struct macin_symbol_collection *collection, char *name) {
	struct macin_symbol *arr = collection->symbols;
	for (uint32_t i = 0; i < collection->n_symbols; ++i) {
		char *sym_name = macin_read(core, collection->strtab_offset + arr[i].strtab_index, MAX_SYMBOL_NAME_SZ);
		if (sym_name == NULL) {
			continue;
		}
		if (strncmp(sym_name, name, MAX_SYMBOL_NAME_SZ) == 0) {
			return &(arr[i]);
		}
	}
	fprintf(stderr, "macin_symbols_find_symbol: invalid symbol name - %s\n", name);
	return NULL;
}