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

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "vfs_lfs.h"
#include "vfs_native.h"
#include "macro.h"
#include "util.h"

static const char *m_extract_path = "extracted/";
static uint8_t m_buffer[4096];

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s <lfs image>\n", name);
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
        int err = target_vfs->close(vfs, out);
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

int main(int argc, char **argv)
{
    int result = EXIT_SUCCESS;

    bool mounted = false;

    if (argc != 2) {
        usage(argv[0]);
        result = EXIT_FAILURE;
        goto done;
    }

    const char *filename = argv[1];

    struct vfs *vfs_lfs = vfs_lfs_get(filename);
    struct vfs *vfs_native = vfs_native_get(m_extract_path);

    int err = vfs_lfs->mount(vfs_lfs);
    CHECK_ERROR(err == 0, EXIT_FAILURE, "vfs->mount() failed: %d", err);
    mounted = true;

    traversal(vfs_lfs, vfs_native, "/");

done:
    if (mounted) {
        int err = vfs_lfs->unmount(vfs_lfs);
        if (err != 0) {
            ERROR("vfs->unmount: %d", err);
        }
    }
    return result;
}
