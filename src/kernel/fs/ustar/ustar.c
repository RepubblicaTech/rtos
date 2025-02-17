#include "ustar.h"

#include <memory/heap/liballoc.h>

#include <stdio.h>

#include <util/string.h>
#include <util/util.h>

void *file_lookup(void *ramfs, char *filename);

// returns a custom structure with their files and their count
ustar_fs *ramfs_init(void *ramfs) {
    ustar_file_header *ustar_start = (ustar_file_header *)ramfs;

    if (strcmp(ustar_start->identifier, USTAR_FILE_IDENTIFIER) != 0) {
        debugf_warn("Identifier's not correct, found \"%6s\"\n",
                    ustar_start->identifier);
    }

    // start parsing all files
    void *ptr          = ustar_start;
    size_t files_found = 0;
    do {

        ustar_file_header *header = (ustar_file_header *)ptr;

        if (strcmp(ustar_start->identifier, USTAR_FILE_IDENTIFIER) == 0)
            files_found++;

        ptr += (((oct2bin(header->size, 11) + 511) / 512) + 1) * 512;
    } while (!memcmp(ptr + 257, USTAR_FILE_IDENTIFIER, 5));

    ustar_fs *ustar_structure = kmalloc(sizeof(ustar_fs));
    memset(ustar_structure, 0, sizeof(ustar_fs));
    ustar_structure->file_count = files_found;
    ustar_structure->files = kcalloc(files_found, sizeof(ustar_file_header));

    kfree(ustar_start);

    return ustar_structure;
}
