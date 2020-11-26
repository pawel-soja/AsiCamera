//#define _GNU_SOURCE
#include "symtab_memory.h"
#include <link.h>

#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static ElfW(Shdr*) find_shdr(ElfW(Ehdr*) ehdr, const char *name)
{
    if (ehdr == NULL || name == NULL)
        return NULL;

    ElfW(Shdr*)  shdr = (ElfW(Shdr*)) ((const char*)ehdr + ehdr->e_shoff);
    const char * str  = (const char*)ehdr + shdr[ehdr->e_shstrndx].sh_offset;

    for(int i=0; i<ehdr->e_shnum; ++i)
    {
        if (strcmp(str + shdr[i].sh_name, name) == 0)
        {
            return &shdr[i];
        }
    }
    return NULL;
}

static ElfW(Sym*) find_sym(ElfW(Ehdr*) ehdr, ElfW(Shdr*) symbols, ElfW(Shdr*) strings, const char *name)
{
    if (ehdr == NULL || symbols == NULL || strings == NULL || name == NULL)
        return NULL;

    ElfW(Sym*) symtab = (ElfW(Sym*))((const char*)ehdr + symbols->sh_offset);
    int count = symbols->sh_size / sizeof(ElfW(Sym));

    const char *strtab = (const char*)ehdr + strings->sh_offset;

    for(int i=0; i<count; ++i)
    {
        if (strcmp(strtab + symtab[i].st_name, name) == 0)
        {
            return &symtab[i];
        }
    }

    return NULL;
}

struct symtab_args
{
    const char **names;
    void **addresses;
};

static int dl_iterate_phdr_callback(struct dl_phdr_info *info, size_t info_size, void *vargs) 
{
    struct symtab_args *args = (struct symtab_args*)vargs;

    int fd = open(*info->dlpi_name ? info->dlpi_name : "/proc/self/exe", O_RDONLY);
    struct stat statbuf;
    int err = fstat(fd, &statbuf);
    char *ptr = (char*)mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);

    // The ELF file header
    ElfW(Ehdr*) ehdr = (ElfW(Ehdr*)) (ptr);
    ElfW(Shdr*) shsymtab = find_shdr(ehdr, ".symtab");
    ElfW(Shdr*) shstrtab = find_shdr(ehdr, ".strtab");

    for(; *args->names != NULL; ++args->names, ++args->addresses)
    {
        ElfW(Sym*) sym = find_sym(ehdr, shsymtab, shstrtab, *args->names);
        if (sym == NULL)
        {
            fprintf(stderr, "symtab_memory: symbol '%s' not found\n", *args->names);
            *args->addresses = NULL;
            continue;
        }
        *args->addresses = (void*)(info->dlpi_addr + sym->st_value);
    }
    err = munmap(ptr, statbuf.st_size);
    close(fd);

    return 1;
}

void symtab_memory(const char **names, void **addresses)
{
    struct symtab_args args = {
        .names = names,
        .addresses = addresses
    };
    dl_iterate_phdr(dl_iterate_phdr_callback, &args);
}
