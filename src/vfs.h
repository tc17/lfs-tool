#pragma once

#include <stdint.h>

#define VFS_MAX_NAME_LEN 512

typedef enum {
    VFS_TYPE_END = 0,
    VFS_TYPE_FILE,
    VFS_TYPE_DIR
} vfs_dirent_type_t;

struct vfs_dirent {
    char name[VFS_MAX_NAME_LEN];
    vfs_dirent_type_t type;
};

struct vfs
{
    void *opaque;
    void *(*open)(struct vfs *vfs, const char *pathname, int flags);
    int (*close)(struct vfs *vfs, void *fd);
    int32_t (*read)(struct vfs *vfs, void *fd, void *buf, size_t count);
    int32_t (*write)(struct vfs *vfs, void *fd, const void *buf, size_t count);
    int (*mount)(struct vfs *vfs);
    int (*unmount)(struct vfs *vfs);
    void *(*opendir)(struct vfs *vfs, const char *path);
    int (*closedir)(struct vfs *vfs, void *dir);
    struct vfs_dirent *(*readdir)(struct vfs *vfs, void *dir);
};
