#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "lfs.h"

#define BLOCK_SIZE 4096

#define CHECK_ERROR(expr, code, msg, ...) ({                        \
    if (!(expr))                                                    \
    {                                                               \
        fprintf(stderr, "[%d] %s: ", __LINE__, __func__);          \
        fprintf(stderr, #expr " failed: " msg "\n", ##__VA_ARGS__); \
        result = code;                                              \
        goto done;                                                  \
    }                                                               \
})

#define ERROR(msg, ...) ({                            \
    fprintf(stderr, "[%d] %s: ", __LINE__, __func__); \
    fprintf(stderr, msg "\n", ##__VA_ARGS__);         \
})

#define INFO(msg, ...) ({                             \
    fprintf(stdout, "[%d] %s: ", __LINE__, __func__); \
    fprintf(stdout, msg "\n", ##__VA_ARGS__);         \
})

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

static struct lfs_config lfs_config = {
    .read = fs_read,
    .prog = fs_prog,
    .erase = fs_erase,
    .sync = fs_sync,
    .read_size = BLOCK_SIZE,
    .prog_size = BLOCK_SIZE,
    .block_size = BLOCK_SIZE,
    .cache_size = BLOCK_SIZE,
    .lookahead_size = BLOCK_SIZE,
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

    INFO("read block: %u, off: %u", block, off);
    INFO("read offset: %u, size: %u", offset, size);

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


static int traversal(lfs_t *lfs)
{
    int result = 0;

    lfs_dir_t lfs_dir = {0};
    bool opened = false;

    int err = lfs_dir_open(lfs, &lfs_dir, "/");
    CHECK_ERROR(err == 0, err, "lfs_dir_open() failed: %d", err);

    do
    {
        struct lfs_info info = {0};
        err = lfs_dir_read(lfs, &lfs_dir, &info);
        CHECK_ERROR(err >= 0, -1, "lfs_dir_read() failed: %d", err);

        printf("name: %s, size: %u, type: %s\n", info.name, info.size, info.type == LFS_TYPE_REG ? "FILE" : "DIR");
    } while (err != 0);

done:
    if (opened) {
        int err = lfs_dir_close(lfs, &lfs_dir);
        if (err < 0) {
            ERROR("lfs_dir_close() failed: %d", err);
        }
    }
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
    lfs_config.block_count = 512;


    int err = lfs_mount(&lfs, &lfs_config);
    CHECK_ERROR(err == 0, EXIT_FAILURE, "lfs_mount() failed: %d", err);
    mounted = true;

    traversal(&lfs);

done:
    if (mounted) {
        int err = lfs_unmount(&lfs);
        if (err != 0) {
            ERROR("lfs_unmount(): %d", err);
        }
    }
    return result;
}
