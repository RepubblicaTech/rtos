#include "ustar.h"

#include <memory/heap/liballoc.h>

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

        size_t entry_size = (size_t)oct2bin(header->size, 11);

        ptr    += (((entry_size + 511) / 512) + 1) * 512;
        header  = (ustar_file_header *)ptr;
    } while (strcmp(USTAR_FILE_IDENTIFIER, header->identifier) == 0);

    ustar_fs *ustar_structure = kmalloc(sizeof(ustar_fs));
    memset(ustar_structure, 0, sizeof(ustar_fs));
    ustar_structure->file_count = files_found;
    ustar_structure->files = kmalloc(files_found * sizeof(ustar_file_header));
    memset(ustar_structure->files, 0, sizeof(ustar_file_header) * files_found);

    ptr = ustar_start;
    for (size_t i = 0; i < ustar_structure->file_count; i++) {
        ustar_file_header *header = (ustar_file_header *)ptr;

        if (strcmp(USTAR_FILE_IDENTIFIER, header->identifier) != 0)
            continue; // we should be done with parsing

        ustar_structure->files[i] = header;

        size_t entry_size = (size_t)oct2bin(header->size, 11);

        ptr += (((entry_size + 511) / 512) + 1) * 512;
    }

    debugf_ok("Found %zu files\n", ustar_structure->file_count);

    return ustar_structure;
}

// returns a pointer to the start of the file
ustar_file **file_lookup(ustar_fs *fs, char *filename) {
    ustar_file **found_files = kmalloc(sizeof(ustar_file));
    ustar_file_header *file_header;
    size_t found = 0;
    for (size_t i = 0; i < fs->file_count; i++) {
        file_header = fs->files[i];

        if (strstr(file_header->path, filename) == NULL) {
            continue;
        } else {
            // we found a file, we're going to save it to our array
            found++;
            krealloc(found_files, found);

            int ustar_entry_type = oct2bin(&file_header->file_type, 1);
            switch (ustar_entry_type) {
                // we might need handling special files (eg. directories)

            default:
                void *file_ptr =
                    ((void *)file_header) +
                    USTAR_SECTOR_ALIGN; // don't use sizeof(file header)...
                size_t file_size = (size_t)oct2bin(file_header->size, 11);

                ustar_file *file = kmalloc(sizeof(ustar_file));
                file->path       = file_header->path;
                file->size       = file_size;
                file->start      = file_ptr;

                found_files[found - 1] = file;
            }
        }
    }

    return found_files;
}
