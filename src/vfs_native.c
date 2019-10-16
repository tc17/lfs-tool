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

#include "vfs_native.h"

#include "macro.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

struct vfs_file {
    int fd;
};

static void *vfs_open(struct vfs *vfs, const char *pathname, int flags)
{
    void *result = NULL;

    struct vfs_file *file = NULL;

    CHECK_ERROR(vfs != NULL, NULL, "vfs == NULL");
    CHECK_ERROR(pathname != NULL, NULL, "pathname == NULL");

    struct vfs_file *file = malloc(sizeof(*file));
    CHECK_ERROR(file != NULL, NULL, "malloc() failed");

    file->fd = open(pathname, flags);
    CHECK_ERROR(file->fd >= 0, NULL, "open() failed: %s", strerror(errno));

done:
    if (result == NULL) {
        free(file);
    }
    return result;
}


static int vfs_close(struct vfs *vfs, void *fd)
{

}

static int32_t vfs_read(struct vfs *vfs, void *fd, void *buf, size_t count);
static int32_t vfs_write(struct vfs *vfs, void *fd, const void *buf, size_t count);
static int vfs_mount(struct vfs *vfs);
static int vfs_unmount(struct vfs *vfs);
static void *vfs_opendir(struct vfs *vfs, const char *path);
static int vfs_closedir(struct vfs *vfs, void *dir);
static struct vfs_dirent *vfs_readdir(struct vfs *vfs, void *dir);

struct vfs m_vfs_native = {

};
