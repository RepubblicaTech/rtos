#include "ustar.h"

#include <memory/heap/liballoc.h>

#include <stdio.h>

#include <util/string.h>
#include <util/util.h>

// returns a custom structure with their files and their count
ustar_fs *ramfs_init(void *ramfs) {
    ustar_file_header *ustar_start = (ustar_file_header *)ramfs;

    if (strcmp(USTAR_FILE_IDENTIFIER, ustar_start->identifier) != 0) {
        debugf_warn("Identifier's not correct, found \"%6s\"\n",
                    ustar_start->identifier);
    }

    // start parsing all files
    void *ptr                 = ustar_start;
    ustar_file_header *header = (ustar_file_header *)ptr;
    size_t files_found        = 0;
    do {
        if (strcmp(USTAR_FILE_IDENTIFIER, header->identifier) == 0) {
            debugf_ok("Found entry %s\n", header->path);
            files_found++;
        }

        size_t entry_size = 0;
        switch (oct2bin(&header->file_type, 1)) {
        case USTAR_FILE_DIRECTORY: // size will be 0
            entry_size = 1;        // just set a random value to go on
            break;

        default:
            entry_size = (size_t)oct2bin(header->size, 12);
            break;
        }

        ptr    += (((entry_size + 511) / 512) + 1) * 512;
        header  = (ustar_file_header *)ptr;
    } while (strcmp(USTAR_FILE_IDENTIFIER, header->identifier) == 0);

    ustar_fs *ustar_structure = kmalloc(sizeof(ustar_fs));
    memset(ustar_structure, 0, sizeof(ustar_fs));
    ustar_structure->file_count = files_found;
    ustar_structure->files = kcalloc(files_found, sizeof(ustar_file_header));

    ptr = ustar_start;
    for (size_t i = 0; i < ustar_structure->file_count; i++) {
        ustar_file_header *header = (ustar_file_header *)ptr;

        if (strcmp(USTAR_FILE_IDENTIFIER, header->identifier) != 0)
            break; // we should be done with parsing

        ustar_structure->files[i] = header;

        size_t entry_size = 0;
        switch (oct2bin(&header->file_type, 1)) {
        case USTAR_FILE_DIRECTORY: // size will be 0
            entry_size = 1;        // just set a random value to go on
            break;

        default:
            entry_size = (size_t)oct2bin(header->size, 12);
            break;
        }

        ptr += (((entry_size + 511) / 512) + 1) * 512;
    }

    kfree(ustar_start);

    debugf_ok("Found %zu files\n", ustar_structure->file_count);

    return ustar_structure;
}

void *file_lookup(ustar_fs *fs, char *filename);
