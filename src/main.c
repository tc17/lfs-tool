/**
 * Copyright 2019 Sergey Tyultyaev
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "vfs_lfs.h"
#include "vfs_native.h"
#include "macro.h"
#include "util.h"

static uint8_t m_buffer[4096];

typedef enum {
    ACTION_NONE = 0,
    ACTION_EXTRACT,
    ACTION_CREATE
} action_t;

struct options {
    const char *directory;
    const char *image;
    action_t action;
    size_t name_max;
    size_t io_size;
    size_t block_size;
    size_t block_count;
};

static void usage(const char *name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "   %s [-n <max name length>] [-s <io size>] [-b <block size>] [-a <number of blocks>] -i <lfs image> -d <directory> (-x | -c)\n", name);
    fprintf(stderr, "   \n");
    fprintf(stderr, "   -n <max name length>   Maximum file name length.\n");
    fprintf(stderr, "   -s <io size>           IO size [default: 512].\n");
    fprintf(stderr, "   -b <block size>        Block size [default: 4096].\n");
    fprintf(stderr, "   -a <number of blocks>  Number of blocks [default: 4059].\n");
    fprintf(stderr, "   -i <lfs image>         Path to lfs image.\n");
    fprintf(stderr, "   -d <directory>         Path to root directory.\n");
    fprintf(stderr, "   -x                     Extract files from image.\n");
    fprintf(stderr, "   -c                     Create image.\n");
    exit(EXIT_FAILURE);
}

static int process_file(struct vfs *vfs, struct vfs *target_vfs, const char *path)
{
    int result = 0;

    void *in = NULL;
    void *out = NULL;

    INFO("process: %s", path);

    out = target_vfs->open(target_vfs, path, O_CREAT | O_TRUNC | O_WRONLY);
    CHECK_ERROR(out != NULL, -1, "target_vfs->open() failed");

    in = vfs->open(vfs, path, O_RDONLY);
    CHECK_ERROR(in != NULL, -1, "vfs->open() failed");

    int32_t rb = 0;
    while ((rb = vfs->read(vfs, in, m_buffer, sizeof(m_buffer))) >= 0) {
        int32_t wb = target_vfs->write(target_vfs, out, m_buffer, rb);
        CHECK_ERROR(wb == rb, -1, "target_vfs->write() failed");
        if (rb != sizeof(m_buffer)) {
            break;
        }
    }

    CHECK_ERROR(rb >= 0, -1, "vfs->read() failed: %d", rb);

done:
    if (in != NULL) {
        int err = vfs->close(vfs, in);
        if (err != 0) {
            ERROR("vfs->close() failed: %d", err);
        }
    }
    if (out != NULL) {
        int err = target_vfs->close(target_vfs, out);
        if (err != 0) {
            ERROR("target_vfs()->close() failed: %d", err);
        }
    }

    return result;
}

static int traversal(struct vfs *vfs, struct vfs *target_vfs, const char *dir)
{
    int result = 0;

    char *path = NULL;
    void *vfs_dir = NULL;

    int err = target_vfs->mkdir(target_vfs, dir);
    CHECK_ERROR(err == 0, -1, "target_vfs->mkdir() failed: %d", err);

    vfs_dir = vfs->opendir(vfs, dir);
    CHECK_ERROR(vfs_dir != NULL, -1, "vfs->opendir() failed");

    INFO("traverse %s", dir);

    struct vfs_dirent *dirent = NULL;
    while ((dirent = vfs->readdir(vfs, vfs_dir)) != NULL)
    {
        if (dirent->type == VFS_TYPE_END) {
            break;
        }

        path = append_dir_alloc(dir, dirent->name);
        CHECK_ERROR(path != NULL, -1, "append_dir_alloc() failed");

        INFO("name: %s, type: %s", path, dirent->type == VFS_TYPE_FILE ? "FILE" : "DIR");

        if (dirent->type == VFS_TYPE_FILE)
        {
            int err = process_file(vfs, target_vfs, path);
            CHECK_ERROR(err == 0, -1, "process_file(.., %s) failed: %d", path, err);
        }
        else if (strcmp(dirent->name, ".") && strcmp(dirent->name, ".."))
        {
            int err = traversal(vfs, target_vfs, path);
            CHECK_ERROR(err == 0, -1, "traversal(.., %s) failed: %d", path, err);
        }

        free(path);
        path = NULL;
    };

    CHECK_ERROR(dirent != NULL, -1, "vfs->readdir() failed");

done:
    free(path);

    if (vfs_dir != NULL) {
        int err = vfs->closedir(vfs, vfs_dir);
        if (err != 0) {
            ERROR("vfs->closedir() failed: %d", err);
        }
    }
    return result;
}

static int string_to_size(const char *str, size_t *size)
{
    int result = 0;

    CHECK_ERROR(str != NULL, -1, "str == NULL");
    CHECK_ERROR(size != NULL, -1, "size == NULL");

    char *endptr = NULL;
    errno = 0;

    unsigned long value = strtoul(str, &endptr, 10);
    CHECK_ERROR(endptr != str && errno == 0, -1, "invalid number: %s", str);

    CHECK_ERROR(value <= SIZE_MAX, -1, "conversion failed, size_t is too small");
    *size = (size_t)value;

done:
    return result;
}

int main(int argc, char **argv)
{
    int result = EXIT_SUCCESS;

    struct options options = {0};
    struct vfs *vfs_lfs = NULL;
    struct vfs *vfs_native = NULL;

    int opt = 0;
    while ((opt = getopt(argc, argv, "i:d:n:s:b:a:cxh?")) != -1) {
        switch (opt) {
            case 'i':
                options.image = optarg;
                break;
            case 'd':
                options.directory = optarg;
                break;
            case 'c': {
                CHECK_ERROR(options.action == ACTION_NONE, 1, "REQUIRED -c OR -x");
                options.action = ACTION_CREATE;
            } break;
            case 'x': {
                CHECK_ERROR(options.action == ACTION_NONE, 1, "REQUIRED -x OR -c");
                options.action = ACTION_EXTRACT;
            } break;
            case 'n': {
                CHECK_ERROR(string_to_size(optarg, &options.name_max) == 0, 1, "string_to_size() failed");
            } break;
            case 's': {
                CHECK_ERROR(string_to_size(optarg, &options.io_size) == 0, 1, "string_to_size() failed");
            } break;
            case 'b': {
                CHECK_ERROR(string_to_size(optarg, &options.block_size) == 0, 1, "string_to_size() failed");
            } break;
            case 'a': {
                CHECK_ERROR(string_to_size(optarg, &options.block_count) == 0, 1, "string_to_size() failed");
            } break;
            case 'h':
            /* FALLTHROUGH */
            case '?':
            /* FALLTHROUGH */
            default:
                usage(argv[0]);
                break;
        }
    }

    CHECK_ERROR(optind == argc, 1, "Invalid argument count");
    CHECK_ERROR(options.image != NULL, 1, "-i required");
    CHECK_ERROR(options.directory != NULL, 1, "-d required");

    vfs_native = vfs_native_get(options.directory);

    switch (options.action) {
        case ACTION_EXTRACT: {
            vfs_lfs = vfs_lfs_get(options.image, false, options.name_max, options.io_size, options.block_size,
                                  options.block_count);

            int err = vfs_lfs->mount(vfs_lfs);
            CHECK_ERROR(err == 0, 2, "vfs->mount() failed: %d", err);

            traversal(vfs_lfs, vfs_native, "/");
        } break;
        case ACTION_CREATE: {
            vfs_lfs = vfs_lfs_get(options.image, true, options.name_max, options.io_size, options.block_size,
                                  options.block_count);
            CHECK_ERROR(vfs_lfs != NULL, 2, "vfs_lfs_get() failed");

            int err = vfs_lfs->mount(vfs_lfs);
            CHECK_ERROR(err == 0, 2, "vfs->mount() failed: %d", err);

            traversal(vfs_native, vfs_lfs, "/");
        } break;
        case ACTION_NONE:
            ERROR("REQUIRED -x OR -c");
            usage(argv[0]);
            break;
    }

done:
    if (vfs_lfs != NULL) {
        int err = vfs_lfs->unmount(vfs_lfs);
        if (err != 0) {
            ERROR("vfs->unmount: %d", err);
        }
    }

    if (result != EXIT_SUCCESS) {
        if (result == 1) {
            usage(argv[0]);
        }
        result = EXIT_FAILURE;
    }

    return result;
}
