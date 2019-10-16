// Copyright 2019 Sergey Tyultyaev
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
    int (*mkdir)(struct vfs *vfs, const char *pathname);
};
