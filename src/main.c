#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "vfs_lfs.h"
#include "macro.h"

static const char *m_extract_path = "extracted/";
static uint8_t m_buffer[4096];

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s <lfs image>\n", name);
}

static int mk_extract_dir(const char *path)
{
    int result = 0;

    size_t extract_path_size = strlen(m_extract_path) + strlen(path) + 1;
    char * extract_path = malloc(extract_path_size);

    strcpy_s(extract_path, extract_path_size, m_extract_path);
    strcat_s(extract_path, extract_path_size, path);

    int err = mkdir(extract_path);
    CHECK_ERROR(err == 0 || errno == EEXIST, -1, "mkdir() failed: %s", strerror(errno));

done:
    free(extract_path);
    return result;
}

static int extract_file(struct vfs *vfs, const char *dir, const char *file)
{
    int result = 0;

    char *path = NULL;
    char *extracted_path = NULL;
    void *vfs_file = NULL;
    bool file_opened = false;
    int out = -1;

    size_t path_size = strlen(dir) + strlen("/") + strlen(file) + 1;
    path = malloc(path_size);

    strcpy_s(path, path_size, dir);
    strcat_s(path, path_size, "/");
    strcat_s(path, path_size, file);

    size_t extracted_path_size = path_size + strlen(m_extract_path);
    extracted_path = malloc(extracted_path_size);

    strcpy_s(extracted_path, extracted_path_size, m_extract_path);
    strcat_s(extracted_path, extracted_path_size, path);

    INFO("extract: %s -> %s", path, extracted_path);

    out = open(extracted_path, O_CREAT | O_TRUNC | O_WRONLY);
    CHECK_ERROR(out >= 0, -1, "open() failed: %s", strerror(errno));

    vfs_file = vfs->open(vfs, path, O_RDONLY);
    CHECK_ERROR(vfs_file != NULL, -1, "vfs->open() failed");
    file_opened = true;

    int32_t rb = 0;
    while ((rb = vfs->read(vfs, vfs_file, m_buffer, sizeof(m_buffer))) >= 0) {
        ssize_t wb = write(out, m_buffer, rb);
        CHECK_ERROR(wb == rb, -1, "write() failed: %s", strerror(errno));
        if (rb != sizeof(m_buffer)) {
            break;
        }
    }

    CHECK_ERROR(rb >= 0, -1, "vfs->read() failed: %d", rb);

done:
    if (file_opened) {
        int err = vfs->close(vfs, vfs_file);
        if (err != 0) {
            ERROR("vfs->close() failed: %d", err);
        }
    }
    if (out >= 0) {
        if (close(out) != 0) {
            ERROR("close(): %s", strerror(errno));
        }
    }
    free(path);
    free(extracted_path);

    return result;
}

static int traversal(struct vfs *vfs, const char *dir, const char *subdir)
{
    int result = 0;

    bool opened = false;
    char *path = NULL;

    if (subdir) {
        size_t path_size = strlen(dir) + strlen("/") + strlen(subdir) + 1;
        path = malloc(path_size);
        CHECK_ERROR(path != NULL, -1, "malloc() failed");

        strcpy_s(path, path_size, dir);
        if (dir[strlen(dir) - 1] != '/') {
            strcat_s(path, path_size, "/");
        }
        strcat_s(path, path_size, subdir);
        dir = path;
    }

    int err = mk_extract_dir(dir);
    CHECK_ERROR(err == 0, -1, "mk_extract_dir() failed: %d", err);

    void *vfs_dir = vfs->opendir(vfs, dir);
    CHECK_ERROR(vfs_dir != NULL, -1, "vfs->opendir() failed");
    opened = true;

    INFO("traverse %s", dir);

    struct vfs_dirent *dirent = NULL;
    while ((dirent = vfs->readdir(vfs, vfs_dir)) != NULL)
    {
        if (dirent->type == VFS_TYPE_END) {
            break;
        }

        printf("name: %s/%s, type: %s\n", dir, dirent->name, dirent->type == VFS_TYPE_FILE ? "FILE" : "DIR");
        if (dirent->type == VFS_TYPE_FILE)
        {
            int err = extract_file(vfs, dir, dirent->name);
            CHECK_ERROR(err == 0, -1, "extract_file(.., %s, %s) failed: %d", dir, dirent->name, err);
        }
        else if (strcmp(dirent->name, ".") && strcmp(dirent->name, ".."))
        {
            int err = traversal(vfs, dir, dirent->name);
            CHECK_ERROR(err == 0, -1, "traversal(.., %s, %s) failed: %d", dir, dirent->name, err);
        }
    };

    CHECK_ERROR(dirent != NULL, -1, "vfs->readdir() failed");

done:
    if (opened) {
        int err = vfs->closedir(vfs, vfs_dir);
        if (err != 0) {
            ERROR("vfs->closedir() failed: %d", err);
        }
    }
    free(path);
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

    int err = vfs_lfs->mount(vfs_lfs);
    CHECK_ERROR(err == 0, EXIT_FAILURE, "vfs->mount() failed: %d", err);
    mounted = true;

    traversal(vfs_lfs, "/", NULL);

done:
    if (mounted) {
        int err = vfs_lfs->unmount(vfs_lfs);
        if (err != 0) {
            ERROR("vfs->unmount: %d", err);
        }
    }
    return result;
}
