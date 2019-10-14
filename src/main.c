#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "lfs.h"

#define BLOCK_SIZE 4096
#define IO_SIZE 256

struct context
{
    FILE *file;
};

static int fs_read(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, void *buffer, lfs_size_t size);
static int fs_prog(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, const void *buffer, lfs_size_t size);
static int fs_erase(const struct lfs_config *c, lfs_block_t block);
static int fs_sync(const struct lfs_config *c);

static const char *m_extract_path = "extracted/";
static uint8_t m_buffer[BLOCK_SIZE];

static struct lfs_config lfs_config = {
    .read = fs_read,
    .prog = fs_prog,
    .erase = fs_erase,
    .sync = fs_sync,
    .read_size = IO_SIZE,
    .prog_size = IO_SIZE,
    .block_size = BLOCK_SIZE,
    .cache_size = IO_SIZE,
    .lookahead_size = IO_SIZE,
    .block_cycles = -1,
};

static int fs_read(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, void *buffer, lfs_size_t size)
{
    int result = 0;
    struct context *context = c->context;

    size_t offset = c->block_size * block + off;

    int err = fseek(context->file, offset, SEEK_SET);
    CHECK_ERROR(err == 0, -1, "fseek() failed: %d", err);

    size_t bytes = fread(buffer, 1, size, context->file);
    CHECK_ERROR(bytes == size, -1, "fread() failed");

    //INFO("read block: %u, off: %u", block, off);
    //INFO("read offset: %u, size: %u", offset, size);

done:
    return result;
}

static int fs_prog(const struct lfs_config *c, lfs_block_t block,
                   lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int result = 0;
    struct context *context = c->context;

    size_t offset = c->block_size * block + off;

    int err = fseek(context->file, offset, SEEK_SET);
    CHECK_ERROR(err == 0, -1, "fseek() failed: %d", err);

    size_t bytes = fwrite(buffer, 1, size, context->file);
    CHECK_ERROR(bytes == size, -1, "fwrite() failed");

done:
    return result;
}

static int fs_erase(const struct lfs_config *c, lfs_block_t block)
{
    return 0;
}

static int fs_sync(const struct lfs_config *c)
{
    struct context *context = c->context;
    return fflush(context->file) != EOF ? 0 : -1;
}

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

static int extract_file(lfs_t *lfs, const char *dir, const char *file)
{
    int result = 0;

    char *path = NULL;
    char *extracted_path = NULL;
    lfs_file_t lfs_file = {0};
    bool lfs_file_opened = false;
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

    int err = lfs_file_open(lfs, &lfs_file, path, LFS_O_RDONLY);
    CHECK_ERROR(err >= 0, -1, "lfs_file_open() failed: %d", err);
    lfs_file_opened = true;

    int32_t rb = 0;
    while ((rb = lfs_file_read(lfs, &lfs_file, m_buffer, sizeof(m_buffer))) >= 0 ) {
        ssize_t wb = write(out, m_buffer, rb);
        CHECK_ERROR(wb == rb, -1, "write() failed: %s", strerror(errno));
        if (rb != sizeof(m_buffer)) {
            break;
        }
    }

    CHECK_ERROR(rb >= 0, -1, "lfs_file_read() failed: %d", rb);

done:
    if (lfs_file_opened) {
        int err = lfs_file_close(lfs, &lfs_file);
        if (err != 0) {
            ERROR("lfs_file_close() failed: %d", err);
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

static int traversal(lfs_t *lfs, const char *dir, const char *subdir)
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

    lfs_dir_t lfs_dir = {0};

    err = lfs_dir_open(lfs, &lfs_dir, dir);
    CHECK_ERROR(err == 0, err, "lfs_dir_open() failed: %d", err);
    opened = true;

    INFO("traverse %s", dir);

    struct lfs_info info = {0};
    while ((err = lfs_dir_read(lfs, &lfs_dir, &info)) > 0)
    {
        printf("name: %s/%s, size: %u, type: %s\n", dir, info.name, info.size, info.type == LFS_TYPE_REG ? "FILE" : "DIR");
        if (info.type == LFS_TYPE_REG)
        {
            int err = extract_file(lfs, dir, info.name);
            CHECK_ERROR(err == 0, -1, "extract_file(.., %s, %s) failed: %d", dir, info.name, err);
        }
        else if (info.type == LFS_TYPE_DIR && strcmp(info.name, ".") && strcmp(info.name, ".."))
        {
            int err = traversal(lfs, dir, info.name);
            CHECK_ERROR(err == 0, -1, "traversal(.., %s, %s) failed: %d", dir, info.name, err);
        }
    };

    CHECK_ERROR(err == 0, -1, "lfs_dir_read() failed: %d", err);

done:
    if (opened) {
        int err = lfs_dir_close(lfs, &lfs_dir);
        if (err < 0) {
            ERROR("lfs_dir_close() failed: %d", err);
        }
    }
    free(path);
    return result;
}

int main(int argc, char **argv)
{
    int result = EXIT_SUCCESS;

    lfs_t lfs = {0};
    bool mounted = false;

    if (argc != 2) {
        usage(argv[0]);
        result = EXIT_FAILURE;
        goto done;
    }

    const char *filename = argv[1];
    struct context context = {0};
    context.file = fopen(filename, "rb");
    CHECK_ERROR(context.file != NULL, EXIT_FAILURE, "fopen() failed: %s", strerror(errno));
    lfs_config.context = &context;
    lfs_config.block_count = 4059;


    int err = lfs_mount(&lfs, &lfs_config);
    CHECK_ERROR(err == 0, EXIT_FAILURE, "lfs_mount() failed: %d", err);
    mounted = true;

    traversal(&lfs, "/", NULL);

done:
    if (mounted) {
        int err = lfs_unmount(&lfs);
        if (err != 0) {
            ERROR("lfs_unmount(): %d", err);
        }
    }
    return result;
}
