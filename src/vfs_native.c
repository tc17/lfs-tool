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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "macro.h"
#include "util.h"


struct vfs_file {
    int fd;
};

struct vfs_dir {
    DIR *dir;
    char *dirname;
};

struct vfs_context {
    const char *path;
};

struct vfs_context m_context = {0};

static void *vfs_open(struct vfs *vfs, const char *pathname, int flags)
{
    void *result = NULL;

    struct vfs_file *file = NULL;
    char *path = NULL;

    CHECK_ERROR(vfs != NULL, NULL, "vfs == NULL");
    CHECK_ERROR(pathname != NULL, NULL, "pathname == NULL");

    struct vfs_context *context = vfs->opaque;
    CHECK_ERROR(context != NULL, NULL, "context == NULL");

    file = malloc(sizeof(*file));
    CHECK_ERROR(file != NULL, NULL, "malloc() failed");

    path = append_dir_alloc(context->path, pathname);
    CHECK_ERROR(path != NULL, NULL, "append_dir_alloc() failed");

#ifdef _WIN32
    flags |= O_BINARY;
#endif //_WIN32

    file->fd = open(path, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    CHECK_ERROR(file->fd >= 0, NULL, "open() failed: %s", strerror(errno));

    result = file;

done:
    free(path);

    if (result == NULL) {
        free(file);
    }
    return result;
}


static int vfs_close(struct vfs *vfs, void *fd)
{
    int result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");
    CHECK_ERROR(fd != NULL, -1, "fd == NULL");

    struct vfs_file *file = fd;

    int err = close(file->fd);
    CHECK_ERROR(err == 0, -1, "close() failed: %s", strerror(errno));

    free(file);

done:
    return result;
}

static int32_t vfs_read(struct vfs *vfs, void *fd, void *buf, size_t count)
{
    int32_t result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");
    CHECK_ERROR(fd != NULL, -1, "fd == NULL");
    CHECK_ERROR(buf != NULL, -1, "buf == NULL");

    struct vfs_file *file = fd;
    result = read(file->fd, buf, count);
    CHECK_ERROR(result >= 0, result, "read() failed: %s", strerror(errno));

done:
    return result;
}

static int32_t vfs_write(struct vfs *vfs, void *fd, const void *buf, size_t count)
{
    int32_t result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");
    CHECK_ERROR(fd != NULL, -1, "fd == NULL");
    CHECK_ERROR(buf != NULL, -1, "buf == NULL");

    struct vfs_file *file = fd;
    result = write(file->fd, buf, count);
    CHECK_ERROR(result >= 0, result, "write() failed: %s", strerror(errno));

done:
    return result;
}

static int vfs_mount(struct vfs *vfs)
{
    int result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");

done:
    return result;
}

static int vfs_unmount(struct vfs *vfs)
{
    int result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");

done:
    return result;
}

static void *vfs_opendir(struct vfs *vfs, const char *pathname)
{
    void *result = NULL;

    struct vfs_dir *vfs_dir = NULL;
    char *path = NULL;

    CHECK_ERROR(vfs != NULL, NULL, "vfs == NULL");
    CHECK_ERROR(pathname != NULL, NULL, "path == NULL");

    struct vfs_context *context = vfs->opaque;
    CHECK_ERROR(context != NULL, NULL, "context == NULL");

    vfs_dir = malloc(sizeof(*vfs_dir));
    CHECK_ERROR(vfs_dir != NULL, NULL, "malloc() failed");

    path = append_dir_alloc(context->path, pathname);
    CHECK_ERROR(path != NULL, NULL, "append_dir_alloc() failed");

    DIR *dir = opendir(path);
    CHECK_ERROR(dir != NULL, NULL, "opendir() failed: %s", strerror(errno));

    vfs_dir->dirname = path;
    vfs_dir->dir = dir;
    result = vfs_dir;

done:
    if (result == NULL) {
        free(path);
        free(vfs_dir);
    }
    return result;
}

static int vfs_closedir(struct vfs *vfs, void *dir)
{
    int result = 0;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");
    CHECK_ERROR(dir != NULL, -1, "dir == NULL");

    struct vfs_dir *vfs_dir = dir;

    free(vfs_dir->dirname);

    int err = closedir(vfs_dir->dir);
    CHECK_ERROR(err == 0, -1, "closedir() failed: %s", strerror(errno));

done:
    return result;
}

static struct vfs_dirent *vfs_readdir(struct vfs *vfs, void *dir)
{
    struct vfs_dirent *result = NULL;

    char *buf = NULL;

    CHECK_ERROR(vfs != NULL, NULL, "vfs == NULL");
    CHECK_ERROR(dir != NULL, NULL, "dir == NULL");

    struct vfs_dir *vfs_dir = dir;

    errno = 0;
    struct dirent *dirent = readdir(vfs_dir->dir);
    CHECK_ERROR(dirent != NULL || errno == 0, NULL, "readdir() failed: %s", strerror(errno));

    static struct vfs_dirent vfs_dirent = {0};

    if (dirent == NULL) {
        vfs_dirent.name[0] = '\0';
        vfs_dirent.type = VFS_TYPE_END;
    }
    else
    {
        buf = append_dir_alloc(vfs_dir->dirname, dirent->d_name);
        CHECK_ERROR(buf != NULL, NULL, "append_dir_alloc() failed");

        struct stat stat_ = {0};

        int err = stat(buf, &stat_);
        CHECK_ERROR(err == 0, NULL, "stat() failed: %s", strerror(errno));
        CHECK_ERROR(S_ISREG(stat_.st_mode) || S_ISDIR(stat_.st_mode), NULL, "unknown file type: 0x%x", stat_.st_mode);

        CHECK_ERROR(strlen(dirent->d_name) < sizeof(vfs_dirent.name), NULL, "vfs_dirent.name is too small");
        strncpy(vfs_dirent.name, dirent->d_name, sizeof(vfs_dirent.name) - 1);
        vfs_dirent.type = S_ISREG(stat_.st_mode) ? VFS_TYPE_FILE : VFS_TYPE_DIR;
    }

    result = &vfs_dirent;

done:
    free(buf);
    return result;
}

static int vfs_mkdir(struct vfs *vfs, const char *pathname)
{
    int result = 0;

    char *path = NULL;

    CHECK_ERROR(vfs != NULL, -1, "vfs == NULL");
    CHECK_ERROR(pathname != NULL, -1, "pathname == NULL");

    struct vfs_context *context = vfs->opaque;
    CHECK_ERROR(context != NULL, -1, "context == NULL");

    path = append_dir_alloc(context->path, pathname);

#ifdef _WIN32
    int err = mkdir(path);
#else
    int err = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
#endif
    CHECK_ERROR(err == 0 || errno == EEXIST, -1, "mkdir() failed: %s", strerror(errno));

done:
    free(path);

    return result;
}

struct vfs m_vfs_native = {
    .open = vfs_open,
    .close = vfs_close,
    .read = vfs_read,
    .write = vfs_write,
    .mount = vfs_mount,
    .unmount = vfs_unmount,
    .opendir = vfs_opendir,
    .closedir = vfs_closedir,
    .readdir = vfs_readdir,
    .mkdir = vfs_mkdir
};


//TODO: replace with init/fini

struct vfs *vfs_native_get(const char *path)
{
    struct vfs *result = NULL;

    CHECK_ERROR(path != NULL, NULL, "path == NULL");

    m_context.path = path;
    m_vfs_native.opaque = &m_context;

    result = &m_vfs_native;
done:
    return result;
}
